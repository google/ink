// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/rendering/webgpu/webgpu_renderer.h"

#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/color_function.h"
#include "ink/color/color.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/rendering/common/stroke_attribute_helpers.h"
#include "ink/rendering/webgpu/embedded_shaders.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/strokes/stroke.h"

namespace ink::rendering::webgpu {
namespace {

using ::ink::strokes_internal::StrokeVertex;

constexpr uint32_t kTextureMappingModeTiling = 0;
constexpr uint32_t kTextureMappingModeStamping = 1;
constexpr uint32_t kMaxTextureLayers = 4;

std::array<float, 16> AffineTransformToMat4(const AffineTransform& t) {
  return {
      t.M00(), t.M01(), 0.0f, 0.0f,  // Column 0
      t.M10(), t.M11(), 0.0f, 0.0f,  // Column 1
      0.0f,    0.0f,    1.0f, 0.0f,  // Column 2
      t.M20(), t.M21(), 0.0f, 1.0f,  // Column 3
  };
}

Color EvaluateCoatColor(const Brush& brush, const BrushPaint& paint) {
  return ColorFunction::ApplyAll(paint.color_functions, brush.GetColor());
}

std::array<float, 4> MakeColorOklabUniform(const Color& color) {
  Color::OklabFloat oklab = color.AsOklab();
  return {oklab.ok_L, oklab.ok_a, oklab.ok_b, oklab.alpha};
}

std::array<float, 4> GetMeshUnpackingTransform(const Mesh& mesh,
                                               int attribute_index) {
  if (attribute_index < 0) return {0.0f, 0.0f, 0.0f, 0.0f};
  const MeshAttributeCodingParams& params =
      mesh.VertexAttributeUnpackingParams(attribute_index);
  if (params.components.Size() < 2) return {0.0f, 0.0f, 0.0f, 0.0f};
  return {
      params.components[0].offset,
      params.components[0].scale,
      params.components[1].offset,
      params.components[1].scale,
  };
}

wgpu::AddressMode ToWgpuAddressMode(BrushPaint::TextureWrap wrap) {
  switch (wrap) {
    case BrushPaint::TextureWrap::kRepeat:
      return wgpu::AddressMode::Repeat;
    case BrushPaint::TextureWrap::kMirror:
      return wgpu::AddressMode::MirrorRepeat;
    case BrushPaint::TextureWrap::kClamp:
      return wgpu::AddressMode::ClampToEdge;
  }
  return wgpu::AddressMode::Repeat;
}

wgpu::Buffer CreateAndUploadBuffer(const wgpu::Device& device,
                                   wgpu::BufferUsage usage,
                                   const void* raw_data, size_t raw_size,
                                   size_t min_size) {
  size_t aligned_size =
      std::max(min_size, (raw_size + 3u) & ~static_cast<size_t>(3u));
  wgpu::BufferDescriptor desc{};
  desc.usage = usage | wgpu::BufferUsage::CopyDst;
  desc.size = aligned_size;
  wgpu::Buffer buffer = device.CreateBuffer(&desc);
  if (raw_size > 0) {
    if (raw_size == aligned_size) {
      device.GetQueue().WriteBuffer(buffer, 0, raw_data, raw_size);
    } else {
      std::vector<std::byte> padded(aligned_size, std::byte{0});
      std::memcpy(padded.data(), raw_data, raw_size);
      device.GetQueue().WriteBuffer(buffer, 0, padded.data(), aligned_size);
    }
  }
  return buffer;
}

}  // namespace

void WebGpuTextureStore::SetTexture(absl::string_view id,
                                    wgpu::Texture texture) {
  textures_[id] = std::move(texture);
}

absl::StatusOr<wgpu::Texture> WebGpuTextureStore::SetRgba8Texture(
    absl::string_view id, const wgpu::Device& device, uint32_t width,
    uint32_t height, absl::Span<const uint8_t> rgba_data) {
  if (!device) {
    return absl::InvalidArgumentError("WebGPU device is null.");
  }
  const size_t expected_size = static_cast<size_t>(width) * height * 4;
  if (rgba_data.size() < expected_size) {
    return absl::InvalidArgumentError(absl::StrCat(
        "RGBA texture data size (", rgba_data.size(),
        ") is smaller than width * height * 4 (", expected_size, ")."));
  }

  wgpu::TextureDescriptor tex_desc{};
  tex_desc.dimension = wgpu::TextureDimension::e2D;
  tex_desc.size = {width, height, 1};
  tex_desc.format = wgpu::TextureFormat::RGBA8Unorm;
  tex_desc.usage =
      wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
  wgpu::Texture texture = device.CreateTexture(&tex_desc);
  if (!texture) {
    return absl::InternalError("Failed to create WebGPU texture.");
  }

  wgpu::TexelCopyTextureInfo destination{};
  destination.texture = texture;
  destination.mipLevel = 0;
  destination.origin = {0, 0, 0};
  destination.aspect = wgpu::TextureAspect::All;

  wgpu::TexelCopyBufferLayout layout{};
  layout.offset = 0;
  layout.bytesPerRow = width * 4;
  layout.rowsPerImage = height;

  wgpu::Extent3D copy_size = {width, height, 1};
  device.GetQueue().WriteTexture(&destination, rgba_data.data(), expected_size,
                                 &layout, &copy_size);

  textures_[id] = texture;
  return texture;
}

wgpu::Texture WebGpuTextureStore::GetTexture(absl::string_view id) const {
  auto it = textures_.find(id);
  if (it == textures_.end()) return nullptr;
  return it->second;
}

absl::StatusOr<WebGpuRenderer> WebGpuRenderer::Create(
    wgpu::Device device,
    std::shared_ptr<const WebGpuTextureStore> texture_store,
    wgpu::TextureFormat color_format,
    wgpu::TextureFormat depth_stencil_format) {
  if (!device) {
    return absl::InvalidArgumentError("WebGPU device must not be null.");
  }

  wgpu::ShaderSourceWGSL wgsl_desc{};
  wgsl_desc.code = kWgslShaderSource;
  wgpu::ShaderModuleDescriptor shader_desc{};
  shader_desc.nextInChain = &wgsl_desc;
  wgpu::ShaderModule shader_module = device.CreateShaderModule(&shader_desc);
  if (!shader_module) {
    return absl::InternalError("Failed to compile StrokeShader.wgsl.");
  }

  // Create explicit bind group layout matching @group(0) in StrokeShader.wgsl:
  // binding 0: uniform buffer
  // binding 1: storage buffer (rawVertexData)
  // bindings 2..5: 2D float textures
  // bindings 6..9: filtering samplers
  std::array<wgpu::BindGroupLayoutEntry, 10> bgl_entries{};
  bgl_entries[0].binding = 0;
  bgl_entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  bgl_entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
  bgl_entries[0].buffer.minBindingSize = sizeof(Uniforms);

  bgl_entries[1].binding = 1;
  bgl_entries[1].visibility = wgpu::ShaderStage::Vertex;
  bgl_entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
  bgl_entries[1].buffer.minBindingSize = 4;

  for (uint32_t i = 0; i < kMaxTextureLayers; ++i) {
    bgl_entries[2 + i].binding = 2 + i;
    bgl_entries[2 + i].visibility = wgpu::ShaderStage::Fragment;
    bgl_entries[2 + i].texture.sampleType = wgpu::TextureSampleType::Float;
    bgl_entries[2 + i].texture.viewDimension = wgpu::TextureViewDimension::e2D;
    bgl_entries[2 + i].texture.multisampled = false;

    bgl_entries[6 + i].binding = 6 + i;
    bgl_entries[6 + i].visibility = wgpu::ShaderStage::Fragment;
    bgl_entries[6 + i].sampler.type = wgpu::SamplerBindingType::Filtering;
  }

  wgpu::BindGroupLayoutDescriptor bgl_desc{};
  bgl_desc.entryCount = bgl_entries.size();
  bgl_desc.entries = bgl_entries.data();
  wgpu::BindGroupLayout bind_group_layout =
      device.CreateBindGroupLayout(&bgl_desc);
  if (!bind_group_layout) {
    return absl::InternalError("Failed to create WebGPU bind group layout.");
  }

  wgpu::PipelineLayoutDescriptor layout_desc{};
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = &bind_group_layout;
  wgpu::PipelineLayout pipeline_layout =
      device.CreatePipelineLayout(&layout_desc);

  // Premultiplied alpha blending: out = src * 1 + dst * (1 - src.a).
  wgpu::BlendState blend_state{};
  blend_state.color.operation = wgpu::BlendOperation::Add;
  blend_state.color.srcFactor = wgpu::BlendFactor::One;
  blend_state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
  blend_state.alpha.operation = wgpu::BlendOperation::Add;
  blend_state.alpha.srcFactor = wgpu::BlendFactor::One;
  blend_state.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;

  wgpu::ColorTargetState color_target{};
  color_target.format = color_format;
  color_target.blend = &blend_state;
  color_target.writeMask = wgpu::ColorWriteMask::All;

  wgpu::VertexState vertex_state{};
  vertex_state.module = shader_module;
  vertex_state.entryPoint = "vertexMain";
  vertex_state.bufferCount = 0;
  vertex_state.buffers = nullptr;

  wgpu::FragmentState fragment_state{};
  fragment_state.module = shader_module;
  fragment_state.entryPoint = "fragmentMain";
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;

  // Normal depth-stencil state (SelfOverlap::kAny / kAccumulate).
  wgpu::DepthStencilState ds_normal{};
  ds_normal.format = depth_stencil_format;
  ds_normal.depthWriteEnabled = false;
  ds_normal.depthCompare = wgpu::CompareFunction::Always;
  ds_normal.stencilFront.compare = wgpu::CompareFunction::Always;
  ds_normal.stencilFront.passOp = wgpu::StencilOperation::Keep;
  ds_normal.stencilFront.failOp = wgpu::StencilOperation::Keep;
  ds_normal.stencilFront.depthFailOp = wgpu::StencilOperation::Keep;
  ds_normal.stencilBack = ds_normal.stencilFront;
  ds_normal.stencilReadMask = 0xFF;
  ds_normal.stencilWriteMask = 0x00;

  wgpu::RenderPipelineDescriptor pipeline_desc{};
  pipeline_desc.layout = pipeline_layout;
  pipeline_desc.vertex = vertex_state;
  pipeline_desc.fragment = &fragment_state;
  pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
  if (depth_stencil_format != wgpu::TextureFormat::Undefined) {
    pipeline_desc.depthStencil = &ds_normal;
  }

  wgpu::RenderPipeline pipeline_normal =
      device.CreateRenderPipeline(&pipeline_desc);
  if (!pipeline_normal) {
    return absl::InternalError(
        "Failed to create normal WebGPU render pipeline.");
  }

  // Discard depth-stencil state (SelfOverlap::kDiscard).
  wgpu::DepthStencilState ds_discard = ds_normal;
  ds_discard.stencilFront.compare = wgpu::CompareFunction::Equal;
  ds_discard.stencilFront.passOp = wgpu::StencilOperation::IncrementClamp;
  ds_discard.stencilBack = ds_discard.stencilFront;
  ds_discard.stencilReadMask = 0xFF;
  ds_discard.stencilWriteMask = 0xFF;

  if (depth_stencil_format != wgpu::TextureFormat::Undefined) {
    pipeline_desc.depthStencil = &ds_discard;
  }
  wgpu::RenderPipeline pipeline_discard =
      device.CreateRenderPipeline(&pipeline_desc);
  if (!pipeline_discard) {
    return absl::InternalError(
        "Failed to create discard WebGPU render pipeline.");
  }

  // Create a 1x1 white RGBA8Unorm dummy texture for unused texture slots.
  wgpu::TextureDescriptor dummy_tex_desc{};
  dummy_tex_desc.dimension = wgpu::TextureDimension::e2D;
  dummy_tex_desc.size = {1, 1, 1};
  dummy_tex_desc.format = wgpu::TextureFormat::RGBA8Unorm;
  dummy_tex_desc.usage =
      wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
  wgpu::Texture dummy_texture = device.CreateTexture(&dummy_tex_desc);
  const std::array<uint8_t, 4> white_pixel = {255, 255, 255, 255};
  wgpu::TexelCopyTextureInfo dst{};
  dst.texture = dummy_texture;
  dst.aspect = wgpu::TextureAspect::All;
  wgpu::TexelCopyBufferLayout layout{};
  layout.bytesPerRow = 4;
  layout.rowsPerImage = 1;
  wgpu::Extent3D extent = {1, 1, 1};
  device.GetQueue().WriteTexture(&dst, white_pixel.data(), white_pixel.size(),
                                 &layout, &extent);
  wgpu::TextureView dummy_texture_view = dummy_texture.CreateView();

  wgpu::SamplerDescriptor sampler_desc{};
  sampler_desc.minFilter = wgpu::FilterMode::Linear;
  sampler_desc.magFilter = wgpu::FilterMode::Linear;
  sampler_desc.addressModeU = wgpu::AddressMode::Repeat;
  sampler_desc.addressModeV = wgpu::AddressMode::Repeat;
  wgpu::Sampler default_sampler = device.CreateSampler(&sampler_desc);

  return WebGpuRenderer(std::move(device), std::move(texture_store),
                        color_format, depth_stencil_format,
                        std::move(bind_group_layout),
                        std::move(pipeline_normal), std::move(pipeline_discard),
                        std::move(dummy_texture), std::move(dummy_texture_view),
                        std::move(default_sampler));
}

WebGpuRenderer::WebGpuRenderer(
    wgpu::Device device,
    std::shared_ptr<const WebGpuTextureStore> texture_store,
    wgpu::TextureFormat color_format, wgpu::TextureFormat depth_stencil_format,
    wgpu::BindGroupLayout bind_group_layout,
    wgpu::RenderPipeline pipeline_normal, wgpu::RenderPipeline pipeline_discard,
    wgpu::Texture dummy_texture, wgpu::TextureView dummy_texture_view,
    wgpu::Sampler default_sampler)
    : device_(std::move(device)),
      texture_store_(std::move(texture_store)),
      color_format_(color_format),
      depth_stencil_format_(depth_stencil_format),
      bind_group_layout_(std::move(bind_group_layout)),
      pipeline_normal_(std::move(pipeline_normal)),
      pipeline_discard_(std::move(pipeline_discard)),
      dummy_texture_(std::move(dummy_texture)),
      dummy_texture_view_(std::move(dummy_texture_view)),
      default_sampler_(std::move(default_sampler)) {}

absl::Status WebGpuRenderer::EncodeMeshDraw(
    const Uniforms& uniforms, const BrushPaint& paint,
    absl::Span<const std::byte> raw_vertex_data,
    absl::Span<const std::byte> raw_index_data, size_t index_stride,
    uint32_t index_count, wgpu::RenderPassEncoder& render_pass) {
  if (index_count == 0 || raw_vertex_data.empty() || raw_index_data.empty()) {
    return absl::OkStatus();
  }
  if (index_stride != 2 && index_stride != 4) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported index stride: ", index_stride));
  }

  Uniforms final_uniforms = uniforms;

  std::array<wgpu::TextureView, kMaxTextureLayers> tex_views = {
      dummy_texture_view_,
      dummy_texture_view_,
      dummy_texture_view_,
      dummy_texture_view_,
  };
  std::array<wgpu::Sampler, kMaxTextureLayers> samplers = {
      default_sampler_,
      default_sampler_,
      default_sampler_,
      default_sampler_,
  };

  uint32_t active_layers = 0;
  if (texture_store_ != nullptr) {
    for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
      if (active_layers >= kMaxTextureLayers) {
        ABSL_LOG(WARNING) << "Exceeded maximum number of texture layers.";
        break;
      }

      std::string client_id;
      BrushPaint::TextureWrap wrap_x = BrushPaint::TextureWrap::kRepeat;
      BrushPaint::TextureWrap wrap_y = BrushPaint::TextureWrap::kRepeat;
      ShaderTextureLayer layer_params{};

      if (std::holds_alternative<BrushPaint::TilingTexture>(layer)) {
        const auto& tiling = std::get<BrushPaint::TilingTexture>(layer);
        client_id = tiling.client_texture_id;
        wrap_x = tiling.wrap_x;
        wrap_y = tiling.wrap_y;
        layer_params = ShaderTextureLayer{
            .mapping_mode = kTextureMappingModeTiling,
            .blend_mode = static_cast<uint32_t>(tiling.blend_mode),
            .tiling_origin = static_cast<uint32_t>(tiling.origin),
            .tiling_size_unit = static_cast<uint32_t>(tiling.size_unit),
            .tiling_size = {tiling.size.x, tiling.size.y},
            .tiling_offset = {tiling.offset.x, tiling.offset.y},
            .tiling_rotation = tiling.rotation.ValueInRadians(),
        };
      } else if (std::holds_alternative<BrushPaint::StampingTexture>(layer)) {
        const auto& stamping = std::get<BrushPaint::StampingTexture>(layer);
        client_id = stamping.client_texture_id;
        wrap_x = BrushPaint::TextureWrap::kClamp;
        wrap_y = BrushPaint::TextureWrap::kClamp;
        layer_params = ShaderTextureLayer{
            .mapping_mode = kTextureMappingModeStamping,
            .blend_mode = static_cast<uint32_t>(stamping.blend_mode),
            .tiling_origin = 0,
            .tiling_size_unit = 0,
            .tiling_size = {0.0f, 0.0f},
            .tiling_offset = {0.0f, 0.0f},
            .tiling_rotation = 0.0f,
        };
      } else {
        continue;
      }

      if (client_id.empty()) continue;
      wgpu::Texture tex = texture_store_->GetTexture(client_id);
      if (!tex) {
        ABSL_LOG(WARNING) << "Texture not found in WebGpuTextureStore for "
                          << client_id;
        continue;
      }

      wgpu::SamplerDescriptor samp_desc{};
      samp_desc.minFilter = wgpu::FilterMode::Linear;
      samp_desc.magFilter = wgpu::FilterMode::Linear;
      samp_desc.addressModeU = ToWgpuAddressMode(wrap_x);
      samp_desc.addressModeV = ToWgpuAddressMode(wrap_y);

      final_uniforms.layers[active_layers] = layer_params;
      tex_views[active_layers] = tex.CreateView();
      samplers[active_layers] = device_.CreateSampler(&samp_desc);
      ++active_layers;
    }
  }
  final_uniforms.texture_layer_count = active_layers;

  wgpu::Buffer uniform_buffer = CreateAndUploadBuffer(
      device_, wgpu::BufferUsage::Uniform, &final_uniforms, sizeof(Uniforms),
      sizeof(Uniforms));
  wgpu::Buffer vertex_buffer = CreateAndUploadBuffer(
      device_, wgpu::BufferUsage::Storage, raw_vertex_data.data(),
      raw_vertex_data.size(), /*min_size=*/16);
  wgpu::Buffer index_buffer = CreateAndUploadBuffer(
      device_, wgpu::BufferUsage::Index, raw_index_data.data(),
      raw_index_data.size(), /*min_size=*/4);

  std::array<wgpu::BindGroupEntry, 10> bg_entries{};
  bg_entries[0].binding = 0;
  bg_entries[0].buffer = uniform_buffer;
  bg_entries[0].offset = 0;
  bg_entries[0].size = sizeof(Uniforms);

  bg_entries[1].binding = 1;
  bg_entries[1].buffer = vertex_buffer;
  bg_entries[1].offset = 0;
  bg_entries[1].size = vertex_buffer.GetSize();

  for (uint32_t i = 0; i < kMaxTextureLayers; ++i) {
    bg_entries[2 + i].binding = 2 + i;
    bg_entries[2 + i].textureView = tex_views[i];

    bg_entries[6 + i].binding = 6 + i;
    bg_entries[6 + i].sampler = samplers[i];
  }

  wgpu::BindGroupDescriptor bg_desc{};
  bg_desc.layout = bind_group_layout_;
  bg_desc.entryCount = bg_entries.size();
  bg_desc.entries = bg_entries.data();
  wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);

  if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard &&
      depth_stencil_format_ != wgpu::TextureFormat::Undefined) {
    render_pass.SetPipeline(pipeline_discard_);
    render_pass.SetStencilReference(0);
  } else {
    render_pass.SetPipeline(pipeline_normal_);
  }

  render_pass.SetBindGroup(0, bind_group);
  wgpu::IndexFormat index_format = (index_stride == 2)
                                       ? wgpu::IndexFormat::Uint16
                                       : wgpu::IndexFormat::Uint32;
  render_pass.SetIndexBuffer(index_buffer, index_format, 0,
                             index_buffer.GetSize());
  render_pass.DrawIndexed(index_count);
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::Draw(const InProgressStroke& stroke,
                                  const AffineTransform& object_to_canvas,
                                  const AffineTransform& canvas_to_ndc,
                                  wgpu::RenderPassEncoder& render_pass) {
  const Brush* brush = stroke.GetBrush();
  if (brush == nullptr || stroke.InputCount() == 0) {
    return absl::OkStatus();
  }

  const StrokeInputBatch& inputs = stroke.GetInputs();
  Point first_pos = inputs.Get(0).position;
  Point last_pos = inputs.Get(stroke.InputCount() - 1).position;

  uint32_t coat_count = stroke.BrushCoatCount();
  absl::Span<const BrushCoat> coats = brush->GetCoats();

  for (uint32_t coat_index = 0; coat_index < coat_count; ++coat_index) {
    if (coat_index >= coats.size()) break;
    if (coats[coat_index].paint_preferences.empty()) continue;
    const BrushPaint& paint = coats[coat_index].paint_preferences[0];

    const MutableMesh& mesh = stroke.GetMesh(coat_index);
    if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;

    const MeshFormat& format = stroke.GetMeshFormat(coat_index);
    StrokeVertex::FormatAttributeIndices attribute_indices =
        StrokeVertex::kFullFormatAttributeIndices;
    StrokeAttributeTypesAndOffsets offsets =
        GetInProgressStrokeAttributeTypesAndOffsets(format, attribute_indices);
    if (offsets.position_and_opacity_shift.offset < 0) continue;

    uint32_t position_offset =
        static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
    int32_t hcl_offset =
        offsets.hcl_shift.has_value() ? offsets.hcl_shift->offset : -1;
    int32_t side_offset = offsets.side_derivative_and_label.offset;
    int32_t forward_offset = offsets.forward_derivative_and_label.offset;
    int32_t surface_offset =
        offsets.surface_uv_and_paint_animation_offset.has_value()
            ? offsets.surface_uv_and_paint_animation_offset->offset
            : -1;

    Color coat_color = EvaluateCoatColor(*brush, paint);
    Uniforms uniforms{
        .model_transform = AffineTransformToMat4(AffineTransform::Identity()),
        .view_transform = AffineTransformToMat4(object_to_canvas),
        .projection_transform = AffineTransformToMat4(canvas_to_ndc),
        .color_oklab = MakeColorOklabUniform(coat_color),
        .position_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
        .side_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
        .forward_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
        .vertex_stride = -static_cast<int32_t>(mesh.VertexStride()),
        .position_and_opacity_shift_offset = position_offset,
        .hcl_shift_offset = hcl_offset,
        .side_derivative_and_label_offset = side_offset,
        .forward_derivative_and_label_offset = forward_offset,
        .surface_uv_and_animation_offset_offset = surface_offset,
        .brush_size = brush->GetSize(),
        .first_input_pos = {first_pos.x, first_pos.y},
        .last_input_pos = {last_pos.x, last_pos.y},
    };

    if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
      Uniforms no_aa_uniforms = uniforms;
      no_aa_uniforms.side_derivative_and_label_offset = -1;
      no_aa_uniforms.forward_derivative_and_label_offset = -1;
      absl::Status status = EncodeMeshDraw(
          no_aa_uniforms, paint, mesh.RawVertexData(), mesh.RawIndexData(),
          mesh.IndexStride(), static_cast<uint32_t>(mesh.TriangleCount()) * 3,
          render_pass);
      if (!status.ok()) return status;
    }

    absl::Status status = EncodeMeshDraw(
        uniforms, paint, mesh.RawVertexData(), mesh.RawIndexData(),
        mesh.IndexStride(), static_cast<uint32_t>(mesh.TriangleCount()) * 3,
        render_pass);
    if (!status.ok()) return status;
  }
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::Draw(const Stroke& stroke,
                                  const AffineTransform& object_to_canvas,
                                  const AffineTransform& canvas_to_ndc,
                                  wgpu::RenderPassEncoder& render_pass) {
  if (stroke.GetInputs().Size() == 0) {
    return absl::OkStatus();
  }

  const PartitionedMesh& shape = stroke.GetShape();
  uint32_t group_count = shape.RenderGroupCount();
  if (group_count == 0) {
    return absl::OkStatus();
  }

  const Brush& brush = stroke.GetBrush();
  absl::Span<const BrushCoat> coats = brush.GetCoats();
  const StrokeInputBatch& inputs = stroke.GetInputs();
  Point first_pos = inputs.Get(0).position;
  Point last_pos = inputs.Get(inputs.Size() - 1).position;

  for (uint32_t coat_index = 0; coat_index < group_count; ++coat_index) {
    if (coat_index >= coats.size()) break;
    if (coats[coat_index].paint_preferences.empty()) continue;
    const BrushPaint& paint = coats[coat_index].paint_preferences[0];

    const MeshFormat& format = shape.RenderGroupFormat(coat_index);
    StrokeVertex::FormatAttributeIndices attribute_indices =
        StrokeVertex::FindAttributeIndices(format);
    absl::StatusOr<StrokeAttributeTypesAndOffsets> offsets_or =
        GetValidatedStrokeAttributeTypesAndOffsets(format, attribute_indices);
    if (!offsets_or.ok()) return offsets_or.status();
    StrokeAttributeTypesAndOffsets offsets = *std::move(offsets_or);
    if (offsets.position_and_opacity_shift.offset < 0) continue;

    uint32_t position_offset =
        static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
    int32_t hcl_offset =
        offsets.hcl_shift.has_value() ? offsets.hcl_shift->offset : -1;
    int32_t side_offset = offsets.side_derivative_and_label.offset;
    int32_t forward_offset = offsets.forward_derivative_and_label.offset;
    int32_t surface_offset =
        offsets.surface_uv_and_paint_animation_offset.has_value()
            ? offsets.surface_uv_and_paint_animation_offset->offset
            : -1;

    int32_t vertex_stride = static_cast<int32_t>(format.PackedVertexStride());
    Color coat_color = EvaluateCoatColor(brush, paint);
    std::array<float, 4> color_oklab = MakeColorOklabUniform(coat_color);

    absl::Span<const Mesh> meshes = shape.RenderGroupMeshes(coat_index);
    if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
      for (const Mesh& mesh : meshes) {
        if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;

        Uniforms no_aa_uniforms{
            .model_transform =
                AffineTransformToMat4(AffineTransform::Identity()),
            .view_transform = AffineTransformToMat4(object_to_canvas),
            .projection_transform = AffineTransformToMat4(canvas_to_ndc),
            .color_oklab = color_oklab,
            .position_unpacking_transform =
                GetMeshUnpackingTransform(mesh, attribute_indices.position),
            .side_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
            .forward_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
            .vertex_stride = vertex_stride,
            .position_and_opacity_shift_offset = position_offset,
            .hcl_shift_offset = hcl_offset,
            .side_derivative_and_label_offset = -1,
            .forward_derivative_and_label_offset = -1,
            .surface_uv_and_animation_offset_offset = surface_offset,
            .brush_size = brush.GetSize(),
            .first_input_pos = {first_pos.x, first_pos.y},
            .last_input_pos = {last_pos.x, last_pos.y},
        };
        absl::Status status = EncodeMeshDraw(
            no_aa_uniforms, paint, mesh.RawVertexData(), mesh.RawIndexData(),
            mesh.IndexStride(), static_cast<uint32_t>(mesh.TriangleCount()) * 3,
            render_pass);
        if (!status.ok()) return status;
      }
    }

    for (const Mesh& mesh : meshes) {
      if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;

      Uniforms uniforms{
          .model_transform = AffineTransformToMat4(AffineTransform::Identity()),
          .view_transform = AffineTransformToMat4(object_to_canvas),
          .projection_transform = AffineTransformToMat4(canvas_to_ndc),
          .color_oklab = color_oklab,
          .position_unpacking_transform =
              GetMeshUnpackingTransform(mesh, attribute_indices.position),
          .side_derivative_unpacking_transform = GetMeshUnpackingTransform(
              mesh, attribute_indices.side_derivative),
          .forward_derivative_unpacking_transform = GetMeshUnpackingTransform(
              mesh, attribute_indices.forward_derivative),
          .vertex_stride = vertex_stride,
          .position_and_opacity_shift_offset = position_offset,
          .hcl_shift_offset = hcl_offset,
          .side_derivative_and_label_offset = side_offset,
          .forward_derivative_and_label_offset = forward_offset,
          .surface_uv_and_animation_offset_offset = surface_offset,
          .brush_size = brush.GetSize(),
          .first_input_pos = {first_pos.x, first_pos.y},
          .last_input_pos = {last_pos.x, last_pos.y},
      };

      absl::Status status = EncodeMeshDraw(
          uniforms, paint, mesh.RawVertexData(), mesh.RawIndexData(),
          mesh.IndexStride(), static_cast<uint32_t>(mesh.TriangleCount()) * 3,
          render_pass);
      if (!status.ok()) return status;
    }
  }
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::BeginRenderPass(wgpu::Texture target_texture,
                                             const Color& clear_color) {
  if (!target_texture) {
    return absl::InvalidArgumentError("Target texture must not be null.");
  }

  active_command_encoder_ = device_.CreateCommandEncoder();
  wgpu::TextureView color_view = target_texture.CreateView();
  Color::RgbaFloat rgba = clear_color.AsFloat(Color::Format::kGammaEncoded);

  wgpu::RenderPassColorAttachment color_attachment{};
  color_attachment.view = color_view;
  color_attachment.loadOp = wgpu::LoadOp::Clear;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.clearValue = {rgba.r, rgba.g, rgba.b, rgba.a};

  wgpu::RenderPassDepthStencilAttachment ds_attachment{};
  wgpu::RenderPassDescriptor rp_desc{};
  rp_desc.colorAttachmentCount = 1;
  rp_desc.colorAttachments = &color_attachment;

  if (depth_stencil_format_ != wgpu::TextureFormat::Undefined) {
    wgpu::TextureDescriptor ds_tex_desc{};
    ds_tex_desc.dimension = wgpu::TextureDimension::e2D;
    ds_tex_desc.size = {target_texture.GetWidth(), target_texture.GetHeight(),
                        1};
    ds_tex_desc.format = depth_stencil_format_;
    ds_tex_desc.usage = wgpu::TextureUsage::RenderAttachment;
    active_depth_stencil_texture_ = device_.CreateTexture(&ds_tex_desc);
    active_depth_stencil_view_ = active_depth_stencil_texture_.CreateView();

    ds_attachment.view = active_depth_stencil_view_;
    ds_attachment.depthClearValue = 1.0f;
    if (depth_stencil_format_ != wgpu::TextureFormat::Stencil8) {
      ds_attachment.depthLoadOp = wgpu::LoadOp::Clear;
      ds_attachment.depthStoreOp = wgpu::StoreOp::Discard;
    }
    ds_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
    ds_attachment.stencilStoreOp = wgpu::StoreOp::Store;
    ds_attachment.stencilClearValue = 0;
    rp_desc.depthStencilAttachment = &ds_attachment;
  }

  active_render_pass_ = active_command_encoder_.BeginRenderPass(&rp_desc);
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::DrawInActivePass(
    const InProgressStroke& stroke, const AffineTransform& object_to_canvas,
    const AffineTransform& canvas_to_ndc) {
  if (!active_render_pass_) {
    return absl::FailedPreconditionError(
        "BeginRenderPass must be called before DrawInActivePass.");
  }
  return Draw(stroke, object_to_canvas, canvas_to_ndc, active_render_pass_);
}

absl::Status WebGpuRenderer::DrawInActivePass(
    const Stroke& stroke, const AffineTransform& object_to_canvas,
    const AffineTransform& canvas_to_ndc) {
  if (!active_render_pass_) {
    return absl::FailedPreconditionError(
        "BeginRenderPass must be called before DrawInActivePass.");
  }
  return Draw(stroke, object_to_canvas, canvas_to_ndc, active_render_pass_);
}

absl::Status WebGpuRenderer::EndRenderPass() {
  if (!active_render_pass_) {
    return absl::FailedPreconditionError(
        "BeginRenderPass must be called before EndRenderPass.");
  }
  active_render_pass_.End();
  wgpu::CommandBuffer cmd = active_command_encoder_.Finish();
  device_.GetQueue().Submit(1, &cmd);

  active_render_pass_ = nullptr;
  active_depth_stencil_view_ = nullptr;
  active_depth_stencil_texture_ = nullptr;
  active_command_encoder_ = nullptr;
  return absl::OkStatus();
}

}  // namespace ink::rendering::webgpu
