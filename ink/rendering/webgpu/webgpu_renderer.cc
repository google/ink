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
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/status_macros.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
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
#include "ink/strokes/stroke_paint_animation_clock.h"

namespace ink::rendering::webgpu {
namespace {

using ::ink::strokes_internal::StrokeVertex;

// LINT.IfChange(texture_mapping_modes)
constexpr uint32_t kTextureMappingModeTiling = 0;
constexpr uint32_t kTextureMappingModeStamping = 1;
// LINT.ThenChange(StrokeShader.wgsl:texture_mapping_modes)

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
                                   const void* absl_nonnull raw_data,
                                   size_t raw_size, size_t min_size) {
  size_t aligned_size =
      std::max(min_size, (raw_size + 3u) & ~static_cast<size_t>(3u));
  wgpu::BufferDescriptor desc{
      .usage = usage | wgpu::BufferUsage::CopyDst,
      .size = aligned_size,
  };
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

wgpu::Texture WebGpuTextureStore::GetTexture(absl::string_view id) const {
  auto it = textures_.find(id);
  if (it == textures_.end()) return nullptr;
  return it->second;
}

absl::StatusOr<WebGpuRenderer> WebGpuRenderer::Create(
    wgpu::Device device,
    absl_nullable std::shared_ptr<const WebGpuTextureStore> texture_store,
    wgpu::TextureFormat color_format, wgpu::TextureFormat depth_stencil_format,
    std::optional<int> sample_count) {
  if (device == nullptr) {
    return absl::InvalidArgumentError("WebGPU device must not be null.");
  }
  if (sample_count.has_value() && *sample_count != 1 && *sample_count != 4) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported MSAA sample count: ", *sample_count));
  }
  uint32_t effective_sample_count =
      sample_count.has_value() ? static_cast<uint32_t>(*sample_count) : 1;

  wgpu::ShaderSourceWGSL wgsl_desc({.code = kWgslShaderSource});
  wgpu::ShaderModuleDescriptor shader_desc{.nextInChain = &wgsl_desc};
  wgpu::ShaderModule shader_module = device.CreateShaderModule(&shader_desc);
  if (shader_module == nullptr) {
    return absl::InternalError("Failed to compile StrokeShader.wgsl.");
  }

  // Create explicit bind group layout matching @group(0) in StrokeShader.wgsl:
  // binding 0: uniform buffer
  // binding 1: storage buffer (rawVertexData)
  // bindings 2..5: 2D float textures
  // bindings 6..9: filtering samplers
  std::array<wgpu::BindGroupLayoutEntry, 2 + kMaxTextureLayers * 2> bgl_entries{
      {
          {
              .binding = 0,
              .visibility =
                  wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
              .buffer =
                  {
                      .type = wgpu::BufferBindingType::Uniform,
                      .minBindingSize = sizeof(Uniforms),
                  },
          },
          {
              .binding = 1,
              .visibility = wgpu::ShaderStage::Vertex,
              .buffer =
                  {
                      .type = wgpu::BufferBindingType::ReadOnlyStorage,
                      .minBindingSize = 4,
                  },
          },
      }};

  for (uint32_t i = 0; i < kMaxTextureLayers; ++i) {
    bgl_entries[2 + i] = wgpu::BindGroupLayoutEntry{
        .binding = 2 + i,
        .visibility = wgpu::ShaderStage::Fragment,
        .texture =
            {
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D,
                .multisampled = false,
            },
    };
    bgl_entries[2 + kMaxTextureLayers + i] = wgpu::BindGroupLayoutEntry{
        .binding = 2 + kMaxTextureLayers + i,
        .visibility = wgpu::ShaderStage::Fragment,
        .sampler = {.type = wgpu::SamplerBindingType::Filtering},
    };
  }

  wgpu::BindGroupLayoutDescriptor bgl_desc{
      .entryCount = bgl_entries.size(),
      .entries = bgl_entries.data(),
  };
  wgpu::BindGroupLayout bind_group_layout =
      device.CreateBindGroupLayout(&bgl_desc);
  if (bind_group_layout == nullptr) {
    return absl::InternalError("Failed to create WebGPU bind group layout.");
  }

  wgpu::PipelineLayoutDescriptor layout_desc{
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &bind_group_layout,
  };
  wgpu::PipelineLayout pipeline_layout =
      device.CreatePipelineLayout(&layout_desc);

  // Premultiplied alpha blending: out = src * 1 + dst * (1 - src.a).
  wgpu::BlendState blend_state{
      .color =
          {
              .operation = wgpu::BlendOperation::Add,
              .srcFactor = wgpu::BlendFactor::One,
              .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
          },
      .alpha =
          {
              .operation = wgpu::BlendOperation::Add,
              .srcFactor = wgpu::BlendFactor::One,
              .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
          },
  };

  wgpu::ColorTargetState color_target{
      .format = color_format,
      .blend = &blend_state,
      .writeMask = wgpu::ColorWriteMask::All,
  };

  wgpu::VertexState vertex_state{
      .module = shader_module,
      .entryPoint = "vertexMain",
      .bufferCount = 0,
      .buffers = nullptr,
  };

  wgpu::FragmentState fragment_state{
      .module = shader_module,
      .entryPoint = "fragmentMain",
      .targetCount = 1,
      .targets = &color_target,
  };

  // Normal depth-stencil state (SelfOverlap::kAny / kAccumulate).
  wgpu::StencilFaceState normal_stencil_face{
      .compare = wgpu::CompareFunction::Always,
      .failOp = wgpu::StencilOperation::Keep,
      .depthFailOp = wgpu::StencilOperation::Keep,
      .passOp = wgpu::StencilOperation::Keep,
  };
  wgpu::DepthStencilState ds_normal{
      .format = depth_stencil_format,
      .depthWriteEnabled = false,
      .depthCompare = wgpu::CompareFunction::Always,
      .stencilFront = normal_stencil_face,
      .stencilBack = normal_stencil_face,
      .stencilReadMask = 0xFF,
      .stencilWriteMask = 0x00,
  };

  wgpu::RenderPipelineDescriptor pipeline_desc{
      .layout = pipeline_layout,
      .vertex = vertex_state,
      .primitive =
          {
              .topology = wgpu::PrimitiveTopology::TriangleList,
              .cullMode = wgpu::CullMode::None,
          },
      .depthStencil = depth_stencil_format != wgpu::TextureFormat::Undefined
                          ? &ds_normal
                          : nullptr,
      .multisample = {.count = effective_sample_count},
      .fragment = &fragment_state,
  };

  wgpu::RenderPipeline pipeline_normal =
      device.CreateRenderPipeline(&pipeline_desc);
  if (pipeline_normal == nullptr) {
    return absl::InternalError(
        "Failed to create normal WebGPU render pipeline.");
  }

  // Discard depth-stencil state (SelfOverlap::kDiscard).
  wgpu::StencilFaceState discard_stencil_face{
      .compare = wgpu::CompareFunction::Equal,
      .failOp = wgpu::StencilOperation::Keep,
      .depthFailOp = wgpu::StencilOperation::Keep,
      .passOp = wgpu::StencilOperation::IncrementClamp,
  };
  wgpu::DepthStencilState ds_discard = ds_normal;
  ds_discard.stencilFront = discard_stencil_face;
  ds_discard.stencilBack = discard_stencil_face;
  ds_discard.stencilReadMask = 0xFF;
  ds_discard.stencilWriteMask = 0xFF;

  wgpu::RenderPipeline pipeline_discard;
  wgpu::RenderPipeline pipeline_clear_stencil;
  if (depth_stencil_format != wgpu::TextureFormat::Undefined) {
    pipeline_desc.depthStencil = &ds_discard;
    pipeline_discard = device.CreateRenderPipeline(&pipeline_desc);
    if (pipeline_discard == nullptr) {
      return absl::InternalError(
          "Failed to create discard WebGPU render pipeline.");
    }

    wgpu::VertexState clear_vertex_state{
        .module = shader_module,
        .entryPoint = "clearVertexMain",
    };
    wgpu::ColorTargetState clear_color_target{
        .format = color_format,
        .writeMask = wgpu::ColorWriteMask::None,
    };
    wgpu::FragmentState clear_fragment_state{
        .module = shader_module,
        .entryPoint = "clearFragmentMain",
        .targetCount = 1,
        .targets = &clear_color_target,
    };
    wgpu::DepthStencilState ds_clear{
        .format = depth_stencil_format,
        .depthWriteEnabled = wgpu::OptionalBool::False,
        .depthCompare = wgpu::CompareFunction::Always,
        .stencilFront =
            {
                .compare = wgpu::CompareFunction::Always,
                .failOp = wgpu::StencilOperation::Zero,
                .depthFailOp = wgpu::StencilOperation::Zero,
                .passOp = wgpu::StencilOperation::Zero,
            },
        .stencilBack =
            {
                .compare = wgpu::CompareFunction::Always,
                .failOp = wgpu::StencilOperation::Zero,
                .depthFailOp = wgpu::StencilOperation::Zero,
                .passOp = wgpu::StencilOperation::Zero,
            },
        .stencilReadMask = 0xFF,
        .stencilWriteMask = 0xFF,
    };
    wgpu::RenderPipelineDescriptor clear_pipeline_desc{
        .vertex = clear_vertex_state,
        .primitive =
            {
                .topology = wgpu::PrimitiveTopology::TriangleList,
                .cullMode = wgpu::CullMode::None,
            },
        .depthStencil = &ds_clear,
        .multisample = {.count = effective_sample_count},
        .fragment = &clear_fragment_state,
    };
    pipeline_clear_stencil = device.CreateRenderPipeline(&clear_pipeline_desc);
    if (pipeline_clear_stencil == nullptr) {
      return absl::InternalError(
          "Failed to create stencil clear WebGPU render pipeline.");
    }
  }

  // Create a 1x1 white RGBA8Unorm placeholder texture for unused texture slots.
  wgpu::TextureDescriptor placeholder_tex_desc{
      .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
      .dimension = wgpu::TextureDimension::e2D,
      .size = {.width = 1, .height = 1, .depthOrArrayLayers = 1},
      .format = wgpu::TextureFormat::RGBA8Unorm,
  };
  wgpu::Texture placeholder_texture =
      device.CreateTexture(&placeholder_tex_desc);
  const std::array<uint8_t, 4> white_pixel = {255, 255, 255, 255};
  wgpu::TexelCopyTextureInfo dst{
      .texture = placeholder_texture,
      .aspect = wgpu::TextureAspect::All,
  };
  wgpu::TexelCopyBufferLayout layout{
      .bytesPerRow = 4,
      .rowsPerImage = 1,
  };
  wgpu::Extent3D extent{.width = 1, .height = 1, .depthOrArrayLayers = 1};
  device.GetQueue().WriteTexture(&dst, white_pixel.data(), white_pixel.size(),
                                 &layout, &extent);
  wgpu::TextureView placeholder_texture_view = placeholder_texture.CreateView();

  wgpu::SamplerDescriptor sampler_desc{
      .addressModeU = wgpu::AddressMode::Repeat,
      .addressModeV = wgpu::AddressMode::Repeat,
      .magFilter = wgpu::FilterMode::Linear,
      .minFilter = wgpu::FilterMode::Linear,
  };
  wgpu::Sampler default_sampler = device.CreateSampler(&sampler_desc);

  return WebGpuRenderer(
      std::move(device), std::move(texture_store), color_format,
      depth_stencil_format, sample_count, std::move(bind_group_layout),
      std::move(pipeline_normal), std::move(pipeline_discard),
      std::move(pipeline_clear_stencil), std::move(placeholder_texture),
      std::move(placeholder_texture_view), std::move(default_sampler));
}

namespace {
struct TextureLayerParams {
  absl::string_view client_id;
  BrushPaint::TextureWrap wrap_x = BrushPaint::TextureWrap::kRepeat;
  BrushPaint::TextureWrap wrap_y = BrushPaint::TextureWrap::kRepeat;
  WebGpuRenderer::ShaderTextureLayer shader_params{};
};

TextureLayerParams TextureToLayoutParams(
    const BrushPaint::TilingTexture& tiling) {
  return {
      .client_id = tiling.client_texture_id,
      .wrap_x = tiling.wrap_x,
      .wrap_y = tiling.wrap_y,
      .shader_params =
          {
              .mapping_mode = kTextureMappingModeTiling,
              .blend_mode = static_cast<uint32_t>(tiling.blend_mode),
              .tiling_origin = static_cast<uint32_t>(tiling.origin),
              .tiling_size_unit = static_cast<uint32_t>(tiling.size_unit),
              .tiling_size = {tiling.size.x, tiling.size.y},
              .tiling_offset = {tiling.offset.x, tiling.offset.y},
              .tiling_rotation = tiling.rotation.ValueInRadians(),
          },
  };
}

TextureLayerParams TextureToLayoutParams(
    const BrushPaint::StampingTexture& stamping) {
  return {
      .client_id = stamping.client_texture_id,
      .wrap_x = BrushPaint::TextureWrap::kClamp,
      .wrap_y = BrushPaint::TextureWrap::kClamp,
      .shader_params =
          {
              .mapping_mode = kTextureMappingModeStamping,
              .blend_mode = static_cast<uint32_t>(stamping.blend_mode),
              .tiling_origin = 0,
              .tiling_size_unit = 0,
              .tiling_size = {0.0f, 0.0f},
              .tiling_offset = {0.0f, 0.0f},
              .tiling_rotation = 0.0f,
              .stamping_animation_frames =
                  static_cast<uint32_t>(stamping.animation_frames),
              .stamping_animation_rows =
                  static_cast<uint32_t>(stamping.animation_rows),
              .stamping_animation_columns =
                  static_cast<uint32_t>(stamping.animation_columns),
              .stamping_animation_repeat_mode =
                  static_cast<uint32_t>(stamping.animation_repeat_mode),
          },
  };
}

struct CachedMeshBuffers {
  wgpu::Device device;
  wgpu::Buffer vertex_buffer;
  wgpu::Buffer index_buffer;
};

const CachedMeshBuffers& GetOrCreateMeshBuffers(const wgpu::Device& device,
                                                const Mesh& mesh) {
  if (!mesh.HasCachedRenderingData<CachedMeshBuffers>() ||
      mesh.GetCachedRenderingData<CachedMeshBuffers>().device.Get() !=
          device.Get()) {
    absl::Span<const std::byte> raw_vertex_data = mesh.RawVertexData();
    absl::Span<const std::byte> raw_index_data = mesh.RawIndexData();
    wgpu::Buffer vertex_buffer = CreateAndUploadBuffer(
        device, wgpu::BufferUsage::Storage, raw_vertex_data.data(),
        raw_vertex_data.size(), /*min_size=*/16);
    wgpu::Buffer index_buffer = CreateAndUploadBuffer(
        device, wgpu::BufferUsage::Index, raw_index_data.data(),
        raw_index_data.size(), /*min_size=*/4);
    mesh.SetCachedRenderingData(CachedMeshBuffers{
        .device = device,
        .vertex_buffer = std::move(vertex_buffer),
        .index_buffer = std::move(index_buffer),
    });
  }
  return mesh.GetCachedRenderingData<CachedMeshBuffers>();
}

}  // namespace
WebGpuRenderer::WebGpuRenderer(
    wgpu::Device device,
    absl_nullable std::shared_ptr<const WebGpuTextureStore> texture_store,
    wgpu::TextureFormat color_format, wgpu::TextureFormat depth_stencil_format,
    std::optional<int> sample_count, wgpu::BindGroupLayout bind_group_layout,
    wgpu::RenderPipeline pipeline_normal, wgpu::RenderPipeline pipeline_discard,
    wgpu::RenderPipeline pipeline_clear_stencil,
    wgpu::Texture placeholder_texture,
    wgpu::TextureView placeholder_texture_view, wgpu::Sampler default_sampler)
    : device_(std::move(device)),
      texture_store_(std::move(texture_store)),
      color_format_(color_format),
      depth_stencil_format_(depth_stencil_format),
      sample_count_(sample_count),
      bind_group_layout_(std::move(bind_group_layout)),
      pipeline_normal_(std::move(pipeline_normal)),
      pipeline_discard_(std::move(pipeline_discard)),
      pipeline_clear_stencil_(std::move(pipeline_clear_stencil)),
      placeholder_texture_(std::move(placeholder_texture)) {
  samplers_.emplace(
      std::make_pair(wgpu::AddressMode::Repeat, wgpu::AddressMode::Repeat),
      default_sampler);
  placeholder_texture_views_.fill(std::move(placeholder_texture_view));
  default_samplers_.fill(std::move(default_sampler));
}

wgpu::Sampler WebGpuRenderer::GetOrCreateSampler(
    wgpu::AddressMode address_mode_u, wgpu::AddressMode address_mode_v) {
  auto [it, inserted] =
      samplers_.try_emplace(std::make_pair(address_mode_u, address_mode_v));
  if (inserted) {
    wgpu::SamplerDescriptor samp_desc{
        .addressModeU = address_mode_u,
        .addressModeV = address_mode_v,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
    };
    it->second = device_.CreateSampler(&samp_desc);
  }
  return it->second;
}

void WebGpuRenderer::ClearStencilBuffer(
    wgpu::RenderPassEncoder& render_pass) const {
  if (pipeline_clear_stencil_ == nullptr) return;
  // Unlike `MetalRenderer`, which swaps `MTLDepthStencilState` dynamically on
  // the render command encoder and draws an invisible viewport-covering stroke
  // with its main pipeline, WebGPU bakes `wgpu::DepthStencilState` (and the
  // color write mask) into the immutable `wgpu::RenderPipeline`. We therefore
  // switch to a dedicated `pipeline_clear_stencil_` pipeline and draw a
  // full-screen triangle to zero the stencil buffer mid-pass.
  render_pass.SetPipeline(pipeline_clear_stencil_);
  render_pass.SetStencilReference(0);
  render_pass.Draw(3);
}

absl::Status WebGpuRenderer::EncodeMeshDraw(
    const Uniforms& uniforms, const BrushPaint& paint,
    const wgpu::Buffer& vertex_buffer, const wgpu::Buffer& index_buffer,
    size_t index_stride, uint32_t index_count,
    wgpu::RenderPassEncoder& render_pass) {
  if (index_count == 0 || vertex_buffer == nullptr || index_buffer == nullptr) {
    return absl::OkStatus();
  }
  if (index_stride != 2 && index_stride != 4) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported index stride: ", index_stride));
  }

  Uniforms final_uniforms = uniforms;

  std::array<wgpu::TextureView, kMaxTextureLayers> tex_views =
      placeholder_texture_views_;
  std::array<wgpu::Sampler, kMaxTextureLayers> samplers = default_samplers_;

  uint32_t active_layers = 0;
  if (texture_store_ != nullptr) {
    for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
      if (active_layers >= kMaxTextureLayers) {
        ABSL_LOG(WARNING) << "Exceeded maximum number of texture layers.";
        break;
      }

      TextureLayerParams params = std::visit(
          [](const auto& t) { return TextureToLayoutParams(t); }, layer);
      if (params.client_id.empty()) continue;
      wgpu::Texture tex = texture_store_->GetTexture(params.client_id);
      if (tex == nullptr) {
        ABSL_LOG(WARNING) << "Texture not found in WebGpuTextureStore for "
                          << params.client_id;
        continue;
      }

      final_uniforms.layers[active_layers] = params.shader_params;
      tex_views[active_layers] = tex.CreateView();
      samplers[active_layers] = GetOrCreateSampler(
          ToWgpuAddressMode(params.wrap_x), ToWgpuAddressMode(params.wrap_y));
      ++active_layers;
    }
  }
  final_uniforms.texture_layer_count = active_layers;

  wgpu::Buffer uniform_buffer = CreateAndUploadBuffer(
      device_, wgpu::BufferUsage::Uniform, &final_uniforms, sizeof(Uniforms),
      sizeof(Uniforms));

  std::array<wgpu::BindGroupEntry, 2 + kMaxTextureLayers * 2> bg_entries{{
      {
          .binding = 0,
          .buffer = uniform_buffer,
          .offset = 0,
          .size = sizeof(Uniforms),
      },
      {
          .binding = 1,
          .buffer = vertex_buffer,
          .offset = 0,
          .size = vertex_buffer.GetSize(),
      },
  }};

  for (uint32_t i = 0; i < kMaxTextureLayers; ++i) {
    bg_entries[2 + i] = wgpu::BindGroupEntry{
        .binding = 2 + i,
        .textureView = tex_views[i],
    };
    bg_entries[2 + kMaxTextureLayers + i] = wgpu::BindGroupEntry{
        .binding = 2 + kMaxTextureLayers + i,
        .sampler = samplers[i],
    };
  }

  wgpu::BindGroupDescriptor bg_desc{
      .layout = bind_group_layout_,
      .entryCount = bg_entries.size(),
      .entries = bg_entries.data(),
  };
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

absl::Status WebGpuRenderer::Draw(const InProgressStroke& in_progress_stroke,
                                  const AffineTransform& object_to_canvas,
                                  const AffineTransform& canvas_to_ndc,
                                  wgpu::RenderPassEncoder& render_pass,
                                  absl::Duration animator_clock_state) {
  const Brush* brush = in_progress_stroke.GetBrush();
  if (brush == nullptr || in_progress_stroke.InputCount() == 0) {
    return absl::OkStatus();
  }

  const StrokeInputBatch& inputs = in_progress_stroke.GetInputs();
  Point first_pos = inputs.Get(0).position;
  Point last_pos = inputs.Get(in_progress_stroke.InputCount() - 1).position;

  uint32_t coat_count = in_progress_stroke.BrushCoatCount();
  absl::Span<const BrushCoat> coats = brush->GetCoats();

  for (uint32_t coat_index = 0; coat_index < coat_count; ++coat_index) {
    if (coat_index >= coats.size()) break;
    const MutableMesh& mesh = in_progress_stroke.GetMesh(coat_index);
    if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;

    const MeshFormat& format = in_progress_stroke.GetMeshFormat(coat_index);
    StrokeVertex::FormatAttributeIndices attribute_indices =
        StrokeVertex::kFullFormatAttributeIndices;
    StrokeAttributeTypesAndOffsets offsets =
        GetInProgressStrokeAttributeTypesAndOffsets(format, attribute_indices);
    if (offsets.position_and_opacity_shift.offset < 0) continue;

    for (const BrushPaint& paint : coats[coat_index].paint_preferences) {
      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard &&
          depth_stencil_format_ == wgpu::TextureFormat::Undefined) {
        continue;
      }

      bool request_shader_aa = !sample_count_.has_value();
      uint32_t position_offset =
          static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
      int32_t hcl_offset =
          offsets.hcl_shift.has_value() ? offsets.hcl_shift->offset : -1;
      int32_t side_offset =
          request_shader_aa ? offsets.side_derivative_and_label.offset : -1;
      int32_t forward_offset =
          request_shader_aa ? offsets.forward_derivative_and_label.offset : -1;
      int32_t surface_offset =
          offsets.surface_uv_and_paint_animation_offset.has_value()
              ? offsets.surface_uv_and_paint_animation_offset->offset
              : -1;

      Color coat_color = EvaluateCoatColor(*brush, paint);
      float paint_animation_progress =
          CalculateCurrentAnimationPhaseForBrushPaint(
              animator_clock_state, brush->GetFamily(), paint,
              inputs.GetBasePaintAnimationPhase());
      Uniforms uniforms{
          .object_to_canvas_transform = AffineTransformToMat4(object_to_canvas),
          .projection_transform = AffineTransformToMat4(canvas_to_ndc),
          .color_oklab = MakeColorOklabUniform(coat_color),
          .position_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
          .side_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
          .forward_derivative_unpacking_transform = {0.0f, 0.0f, 0.0f, 0.0f},
          .vertex_stride = -static_cast<int32_t>(mesh.VertexStride()),
          .position_and_opacity_shift_offset = position_offset,
          .hcl_shift_offset = hcl_offset,
          // Not set until after stencil pass.
          .side_derivative_and_label_offset = -1,
          .forward_derivative_and_label_offset = -1,
          .surface_uv_and_paint_animation_offset_offset = surface_offset,
          .brush_size = brush->GetSize(),
          .first_input_pos = {first_pos.x, first_pos.y},
          .last_input_pos = {last_pos.x, last_pos.y},
          .paint_animation_progress = paint_animation_progress,
      };

      absl::Span<const std::byte> raw_vertex_data = mesh.RawVertexData();
      absl::Span<const std::byte> raw_index_data = mesh.RawIndexData();
      if (raw_vertex_data.empty() || raw_index_data.empty()) continue;

      wgpu::Buffer vertex_buffer = CreateAndUploadBuffer(
          device_, wgpu::BufferUsage::Storage, raw_vertex_data.data(),
          raw_vertex_data.size(), /*min_size=*/16);
      wgpu::Buffer index_buffer = CreateAndUploadBuffer(
          device_, wgpu::BufferUsage::Index, raw_index_data.data(),
          raw_index_data.size(), /*min_size=*/4);
      uint32_t index_count = static_cast<uint32_t>(mesh.TriangleCount()) * 3;

      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
        ClearStencilBuffer(render_pass);
        if (request_shader_aa) {
          ABSL_RETURN_IF_ERROR(EncodeMeshDraw(uniforms, paint, vertex_buffer,
                                              index_buffer, mesh.IndexStride(),
                                              index_count, render_pass));
        }
      }

      uniforms.side_derivative_and_label_offset = side_offset;
      uniforms.forward_derivative_and_label_offset = forward_offset;
      ABSL_RETURN_IF_ERROR(EncodeMeshDraw(uniforms, paint, vertex_buffer,
                                          index_buffer, mesh.IndexStride(),
                                          index_count, render_pass));
      break;
    }
  }
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::Draw(const Stroke& stroke,
                                  const AffineTransform& object_to_canvas,
                                  const AffineTransform& canvas_to_ndc,
                                  wgpu::RenderPassEncoder& render_pass,
                                  absl::Duration animator_clock_state) {
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
    absl::Span<const Mesh> meshes = shape.RenderGroupMeshes(coat_index);
    if (meshes.empty()) continue;

    const MeshFormat& format = shape.RenderGroupFormat(coat_index);
    StrokeVertex::FormatAttributeIndices attribute_indices =
        StrokeVertex::FindAttributeIndices(format);
    ABSL_ASSIGN_OR_RETURN(
        StrokeAttributeTypesAndOffsets offsets,
        GetValidatedStrokeAttributeTypesAndOffsets(format, attribute_indices));
    if (offsets.position_and_opacity_shift.offset < 0) continue;

    for (const BrushPaint& paint : coats[coat_index].paint_preferences) {
      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard &&
          depth_stencil_format_ == wgpu::TextureFormat::Undefined) {
        continue;
      }

      bool request_shader_aa = !sample_count_.has_value();
      uint32_t position_offset =
          static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
      int32_t hcl_offset =
          offsets.hcl_shift.has_value() ? offsets.hcl_shift->offset : -1;
      int32_t side_offset =
          request_shader_aa ? offsets.side_derivative_and_label.offset : -1;
      int32_t forward_offset =
          request_shader_aa ? offsets.forward_derivative_and_label.offset : -1;
      int32_t surface_offset =
          offsets.surface_uv_and_paint_animation_offset.has_value()
              ? offsets.surface_uv_and_paint_animation_offset->offset
              : -1;

      int32_t vertex_stride = static_cast<int32_t>(format.PackedVertexStride());
      Color coat_color = EvaluateCoatColor(brush, paint);
      std::array<float, 4> color_oklab = MakeColorOklabUniform(coat_color);
      float paint_animation_progress =
          CalculateCurrentAnimationPhaseForBrushPaint(
              animator_clock_state, brush.GetFamily(), paint,
              inputs.GetBasePaintAnimationPhase());

      Uniforms uniforms{
          .object_to_canvas_transform = AffineTransformToMat4(object_to_canvas),
          .projection_transform = AffineTransformToMat4(canvas_to_ndc),
          .color_oklab = color_oklab,
          .vertex_stride = vertex_stride,
          .position_and_opacity_shift_offset = position_offset,
          .hcl_shift_offset = hcl_offset,
          // Not set until after stencil pass.
          .side_derivative_and_label_offset = -1,
          .forward_derivative_and_label_offset = -1,
          .surface_uv_and_paint_animation_offset_offset = surface_offset,
          .brush_size = brush.GetSize(),
          .first_input_pos = {first_pos.x, first_pos.y},
          .last_input_pos = {last_pos.x, last_pos.y},
          .paint_animation_progress = paint_animation_progress,
      };

      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
        ClearStencilBuffer(render_pass);
        for (const Mesh& mesh : meshes) {
          if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;
          const CachedMeshBuffers& buffers =
              GetOrCreateMeshBuffers(device_, mesh);
          uniforms.position_unpacking_transform =
              GetMeshUnpackingTransform(mesh, attribute_indices.position);
          ABSL_RETURN_IF_ERROR(EncodeMeshDraw(
              uniforms, paint, buffers.vertex_buffer, buffers.index_buffer,
              mesh.IndexStride(),
              static_cast<uint32_t>(mesh.TriangleCount()) * 3, render_pass));
        }
      }

      uniforms.side_derivative_and_label_offset = side_offset;
      uniforms.forward_derivative_and_label_offset = forward_offset;
      for (const Mesh& mesh : meshes) {
        if (mesh.TriangleCount() == 0 || mesh.VertexCount() == 0) continue;
        const CachedMeshBuffers& buffers =
            GetOrCreateMeshBuffers(device_, mesh);
        uniforms.position_unpacking_transform =
            GetMeshUnpackingTransform(mesh, attribute_indices.position);
        uniforms.side_derivative_unpacking_transform =
            GetMeshUnpackingTransform(mesh, attribute_indices.side_derivative);
        uniforms.forward_derivative_unpacking_transform =
            GetMeshUnpackingTransform(mesh,
                                      attribute_indices.forward_derivative);
        ABSL_RETURN_IF_ERROR(EncodeMeshDraw(
            uniforms, paint, buffers.vertex_buffer, buffers.index_buffer,
            mesh.IndexStride(), static_cast<uint32_t>(mesh.TriangleCount()) * 3,
            render_pass));
      }
      break;
    }
  }
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::BeginRenderPass(wgpu::Texture target_texture,
                                             const Color& clear_color) {
  if (target_texture == nullptr) {
    return absl::InvalidArgumentError("Target texture must not be null.");
  }

  active_command_encoder_ = device_.CreateCommandEncoder();
  wgpu::TextureViewDescriptor color_view_desc{.format = color_format_};
  wgpu::TextureView color_view = target_texture.CreateView(&color_view_desc);
  Color::RgbaFloat rgba = clear_color.AsFloat(Color::Format::kGammaEncoded);

  wgpu::RenderPassColorAttachment color_attachment{
      .loadOp = wgpu::LoadOp::Clear,
      .clearValue = {.r = rgba.r, .g = rgba.g, .b = rgba.b, .a = rgba.a},
  };
  uint32_t effective_sample_count =
      sample_count_.has_value() ? static_cast<uint32_t>(*sample_count_) : 1;
  if (effective_sample_count > 1) {
    wgpu::TextureDescriptor msaa_tex_desc{
        .usage = wgpu::TextureUsage::RenderAttachment,
        .dimension = wgpu::TextureDimension::e2D,
        .size =
            {
                .width = target_texture.GetWidth(),
                .height = target_texture.GetHeight(),
                .depthOrArrayLayers = 1,
            },
        .format = color_format_,
        .sampleCount = effective_sample_count,
        .viewFormatCount = 1,
        .viewFormats = &color_format_,
    };
    active_msaa_color_texture_ = device_.CreateTexture(&msaa_tex_desc);
    active_msaa_color_view_ =
        active_msaa_color_texture_.CreateView(&color_view_desc);
    color_attachment.view = active_msaa_color_view_;
    color_attachment.resolveTarget = color_view;
    color_attachment.storeOp = wgpu::StoreOp::Discard;
  } else {
    color_attachment.view = color_view;
    color_attachment.storeOp = wgpu::StoreOp::Store;
  }

  wgpu::RenderPassDepthStencilAttachment ds_attachment{};
  wgpu::RenderPassDescriptor rp_desc{
      .colorAttachmentCount = 1,
      .colorAttachments = &color_attachment,
  };

  if (depth_stencil_format_ != wgpu::TextureFormat::Undefined) {
    wgpu::TextureDescriptor ds_tex_desc{
        .usage = wgpu::TextureUsage::RenderAttachment,
        .dimension = wgpu::TextureDimension::e2D,
        .size =
            {
                .width = target_texture.GetWidth(),
                .height = target_texture.GetHeight(),
                .depthOrArrayLayers = 1,
            },
        .format = depth_stencil_format_,
        .sampleCount = effective_sample_count,
    };
    active_depth_stencil_texture_ = device_.CreateTexture(&ds_tex_desc);
    active_depth_stencil_view_ = active_depth_stencil_texture_.CreateView();

    ds_attachment = wgpu::RenderPassDepthStencilAttachment{
        .view = active_depth_stencil_view_,
        .depthLoadOp = depth_stencil_format_ != wgpu::TextureFormat::Stencil8
                           ? wgpu::LoadOp::Clear
                           : wgpu::LoadOp::Undefined,
        .depthStoreOp = depth_stencil_format_ != wgpu::TextureFormat::Stencil8
                            ? wgpu::StoreOp::Discard
                            : wgpu::StoreOp::Undefined,
        .depthClearValue = 1.0f,
        .stencilLoadOp = wgpu::LoadOp::Clear,
        .stencilStoreOp = wgpu::StoreOp::Store,
        .stencilClearValue = 0,
    };
    rp_desc.depthStencilAttachment = &ds_attachment;
  }

  active_render_pass_ = active_command_encoder_.BeginRenderPass(&rp_desc);
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::EnsureActiveRenderPass(
    absl::string_view caller) const {
  if (active_render_pass_ == nullptr) {
    return absl::FailedPreconditionError(
        absl::StrCat("BeginRenderPass must be called before ", caller, "."));
  }
  return absl::OkStatus();
}

absl::Status WebGpuRenderer::DrawInActivePass(
    const InProgressStroke& in_progress_stroke,
    const AffineTransform& object_to_canvas,
    const AffineTransform& canvas_to_ndc, absl::Duration animator_clock_state) {
  ABSL_RETURN_IF_ERROR(EnsureActiveRenderPass("DrawInActivePass"));
  return Draw(in_progress_stroke, object_to_canvas, canvas_to_ndc,
              active_render_pass_, animator_clock_state);
}

absl::Status WebGpuRenderer::DrawInActivePass(
    const Stroke& stroke, const AffineTransform& object_to_canvas,
    const AffineTransform& canvas_to_ndc, absl::Duration animator_clock_state) {
  ABSL_RETURN_IF_ERROR(EnsureActiveRenderPass("DrawInActivePass"));
  return Draw(stroke, object_to_canvas, canvas_to_ndc, active_render_pass_,
              animator_clock_state);
}

absl::Status WebGpuRenderer::EndRenderPass() {
  ABSL_RETURN_IF_ERROR(EnsureActiveRenderPass("EndRenderPass"));
  active_render_pass_.End();
  wgpu::CommandBuffer cmd = active_command_encoder_.Finish();
  device_.GetQueue().Submit(1, &cmd);

  active_render_pass_ = nullptr;
  active_msaa_color_view_ = nullptr;
  active_msaa_color_texture_ = nullptr;
  active_depth_stencil_view_ = nullptr;
  active_depth_stencil_texture_ = nullptr;
  active_command_encoder_ = nullptr;
  return absl::OkStatus();
}

}  // namespace ink::rendering::webgpu
