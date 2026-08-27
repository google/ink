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

#include "ink/rendering/metal/metal_renderer.h"

#include <simd/matrix.h>
#include <simd/vector.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/status_macros.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/color_function.h"
#include "ink/brush/stock_brushes.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/rendering/common/stroke_attribute_helpers.h"
#include "ink/rendering/metal/metal_renderer_objc_helper.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/strokes/stroke.h"
#include "ink/types/duration.h"

namespace ink::rendering {

namespace {

using ::ink::strokes_internal::StrokeVertex;

// LINT.IfChange(texture_mapping_modes)
constexpr uint32_t kTextureMappingModeTiling = 0;
constexpr uint32_t kTextureMappingModeStamping = 1;
// LINT.ThenChange(../../../rendering/webgpu/StrokeShader.wgsl:texture_mapping_modes)

struct MeshBuffers {
  // Opaque pointer to `INKMeshBuffers`, configured to handle cleanup.
  std::shared_ptr<void> objc_ptr;
};

InProgressStroke MakeClearStencilStroke() {
  // TODO: b/510313825 - Use a square tip to reduce the number of triangles for
  // this invisible stroke used for clearing the stencil buffer.
  absl::StatusOr<Brush> brush = Brush::Create(
      stock_brushes::Marker(), Color::FromFloat(0, 0, 0, 0), 10.0f, 0.001f);
  ABSL_CHECK_OK(brush);
  InProgressStroke stroke;
  stroke.Start(*brush);
  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      StrokeInput{
          .position = Point{0.5f, 0.5f},
          .elapsed_time = Duration32::Zero(),
      },
  });
  ABSL_CHECK_OK(inputs);
  ABSL_CHECK_OK(stroke.EnqueueInputs(*inputs, StrokeInputBatch()));
  stroke.FinishInputs();
  ABSL_CHECK_OK(stroke.UpdateShape(Duration32::Infinite()));
  return stroke;
}

simd_float4 GetMeshUnpackingTransform(const Mesh& mesh, int attribute_index) {
  if (attribute_index < 0) return simd_make_float4(0, 0, 0, 0);
  const MeshAttributeCodingParams& params =
      mesh.VertexAttributeUnpackingParams(attribute_index);
  if (params.components.Size() < 2) return simd_make_float4(0, 0, 0, 0);
  return simd_make_float4(
      params.components[0].offset, params.components[0].scale,
      params.components[1].offset, params.components[1].scale);
}

}  // namespace

absl::StatusOr<MetalRenderer> MetalRenderer::Create(
    void* device, uint64_t color_pixel_format, uint64_t stencil_pixel_format,
    std::optional<int> sample_count, void* absl_nullable texture_bitmap_store) {
  ABSL_ASSIGN_OR_RETURN(auto objc_renderer_state,
                        metal_objc::CreateINKMetalRendererState(
                            device, color_pixel_format, stencil_pixel_format,
                            sample_count, texture_bitmap_store));
  return MetalRenderer(
      std::move(objc_renderer_state),
      /*request_shader_based_anti_aliasing=*/!sample_count.has_value());
}

MetalRenderer::MetalRenderer(
    std::unique_ptr<void, std::function<void(void*)>> objc_renderer_state,
    bool request_shader_based_anti_aliasing)
    : objc_renderer_state_(std::move(objc_renderer_state)),
      request_shader_based_anti_aliasing_(request_shader_based_anti_aliasing),
      clear_stencil_stroke_(MakeClearStencilStroke()) {}

void MetalRenderer::Draw(const InProgressStroke& in_progress_stroke,
                         simd_float4x4 model_transform,
                         simd_float4x4 view_transform,
                         simd_float4x4 projection_transform,
                         void* render_encoder) {
  const Brush* brush = in_progress_stroke.GetBrush();
  if (!brush) return;
  if (in_progress_stroke.InputCount() <= 0) return;

  const StrokeInputBatch& inputs = in_progress_stroke.GetInputs();
  StrokeInput first_input = inputs.Get(0);
  simd_float2 first_input_pos =
      simd_make_float2(first_input.position.x, first_input.position.y);
  StrokeInput last_input = inputs.Get(in_progress_stroke.InputCount() - 1);
  simd_float2 last_input_pos =
      simd_make_float2(last_input.position.x, last_input.position.y);

  metal_objc::SetRenderPipelineState(render_encoder,
                                     objc_renderer_state_.get());

  uint32_t coat_count = in_progress_stroke.BrushCoatCount();
  absl::Span<const BrushCoat> coats = brush->GetCoats();

  for (uint32_t coat_index = 0; coat_index < coat_count; ++coat_index) {
    if (coat_index >= coats.size()) break;
    for (const BrushPaint& paint : coats[coat_index].paint_preferences) {
      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
        if (!metal_objc::SupportsSelfOverlapDiscard(
                objc_renderer_state_.get())) {
          // A stencil buffer is required to discard self-overlap. Hopefully the
          // next paint preference will have a compatible self overlap setting.
          continue;
        }
        ClearStencilBuffer(render_encoder);
        metal_objc::SetDiscardSelfOverlapStencilState(
            render_encoder, objc_renderer_state_.get());

        if (request_shader_based_anti_aliasing_) {
          // Shader-based AA is not compatible with the standard stencil buffer
          // technique for discarding self-overlap. Because shader-based AA
          // explicitly reduces the opacity of certain fragments (at stroke
          // edges) compared to MSAA which adjusts colors after the shader is
          // complete, a fragment that gets filled first with a reduced-opacity
          // color will not be drawn again even if a fragment from the center
          // of the stroke would have later been drawn there with a higher
          // opacity. The result is that the stroke would have a reduced
          // opacity band that highlights the self-overlapping edges of the
          // stroke. To avoid this, we draw the stroke twice. First without any
          // of the shader-based AA logic to both draw those full-opacity
          // pixels and to fill the stencil buffer, discarding self-overlap.
          // Then without clearing the stencil buffer, we draw the stroke
          // again, this time with shader-based AA. This second pass will not
          // draw any of the fragments that were drawn in the first pass, but
          // will add the outset edge fragments with the correct opacity.
          DrawWithStencilAlreadySet(in_progress_stroke, coat_index, paint,
                                    false, first_input_pos, last_input_pos,
                                    model_transform, view_transform,
                                    projection_transform, render_encoder);
        }
      } else {
        // No need to clear the stencil buffer, as a nil stencil state will not
        // use it. It will be cleared the next time it is needed.
        metal_objc::SetNoOpStencilState(render_encoder,
                                        objc_renderer_state_.get());
      }

      DrawWithStencilAlreadySet(in_progress_stroke, coat_index, paint,
                                request_shader_based_anti_aliasing_,
                                first_input_pos, last_input_pos,
                                model_transform, view_transform,
                                projection_transform, render_encoder);

      // Only draw with the first compatible paint preference in each coat.
      break;
    }
  }
}

void MetalRenderer::DrawWithStencilAlreadySet(
    const InProgressStroke& in_progress_stroke, uint32_t coat_index,
    const BrushPaint& paint, bool request_shader_aa,
    simd_float2 first_input_pos, simd_float2 last_input_pos,
    simd_float4x4 model_transform, simd_float4x4 view_transform,
    simd_float4x4 projection_transform, void* render_encoder) {
  const Brush* brush = in_progress_stroke.GetBrush();
  if (brush == nullptr) return;

  Color coat_color = EvaluateCoatColor(*brush, paint);
  simd_float4 color_uniform = MakeColorUniform(coat_color);

  const MutableMesh& mesh = in_progress_stroke.GetMesh(coat_index);
  absl::Span<const std::byte> vertex_data = mesh.RawVertexData();
  absl::Span<const std::byte> index_data = mesh.RawIndexData();
  uint32_t triangle_count = mesh.TriangleCount();
  size_t vertex_stride = mesh.VertexStride();
  size_t index_stride = mesh.IndexStride();

  if (triangle_count == 0) return;

  const MeshFormat& format = in_progress_stroke.GetMeshFormat(coat_index);
  strokes_internal::StrokeVertex::FormatAttributeIndices attribute_indices =
      strokes_internal::StrokeVertex::kFullFormatAttributeIndices;
  rendering::StrokeAttributeTypesAndOffsets offsets =
      rendering::GetInProgressStrokeAttributeTypesAndOffsets(format,
                                                             attribute_indices);

  // The position and opacity shift attribute is always present.
  if (offsets.position_and_opacity_shift.offset < 0) return;
  uint32_t position_offset =
      static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
  // Optional attributes, set to -1 if not present.
  int32_t hsl_offset =
      offsets.hsl_shift.has_value() ? offsets.hsl_shift->offset : -1;
  int32_t side_offset = offsets.side_derivative_and_label.offset;
  int32_t forward_offset = offsets.forward_derivative_and_label.offset;
  int32_t surface_offset =
      offsets.surface_uv_and_paint_animation_offset.has_value()
          ? offsets.surface_uv_and_paint_animation_offset->offset
          : -1;

  if (request_shader_aa && (forward_offset < 0 || side_offset < 0)) {
    ABSL_LOG(WARNING)
        << "INKMetalRenderer shader-based anti-aliasing requested, but mesh "
           "format does not contain side derivative and forward derivative "
           "attributes.";
    return;
  }

  Uniforms uniforms{
      .model_transform = model_transform,
      .view_transform = view_transform,
      .projection_transform = projection_transform,
      .color = color_uniform,
      // Nothing to unpack for in-progress strokes.
      .position_unpacking_transform = simd_make_float4(0, 0, 0, 0),
      .side_derivative_unpacking_transform = simd_make_float4(0, 0, 0, 0),
      .forward_derivative_unpacking_transform = simd_make_float4(0, 0, 0, 0),
      // Negative stride indicates unpacked vertex data.
      .vertex_stride = -static_cast<int32_t>(vertex_stride),
      .position_and_opacity_shift_offset = position_offset,
      .hsl_shift_offset = hsl_offset,
      .side_derivative_and_label_offset = request_shader_aa ? side_offset : -1,
      .forward_derivative_and_label_offset =
          request_shader_aa ? forward_offset : -1,
      .surface_uv_and_paint_animation_offset_offset = surface_offset,
      .brush_size = brush->GetSize(),
      .first_input_pos = first_input_pos,
      .last_input_pos = last_input_pos,
  };
  FillTextureLayers(paint, render_encoder, std::begin(uniforms.layers),
                    &uniforms.texture_layer_count);

  // For InProgressStrokes, the buffers are created each time because the
  // underlying data is mutable.
  metal_objc::DrawIndexedTriangles(
      render_encoder, &uniforms, sizeof(Uniforms), triangle_count, index_stride,
      metal_objc::CreateINKMeshBuffers(objc_renderer_state_.get(),
                                       vertex_data.data(), vertex_data.size(),
                                       index_data.data(), index_data.size())
          .get());
}

void MetalRenderer::Draw(const Stroke& stroke, simd_float4x4 model_transform,
                         simd_float4x4 view_transform,
                         simd_float4x4 projection_transform,
                         void* render_encoder) {
  if (stroke.GetInputs().Size() <= 0) return;

  const StrokeInputBatch& inputs = stroke.GetInputs();
  StrokeInput first_input = inputs.Get(0);
  simd_float2 first_input_pos =
      simd_make_float2(first_input.position.x, first_input.position.y);
  StrokeInput last_input = inputs.Get(stroke.GetInputs().Size() - 1);
  simd_float2 last_input_pos =
      simd_make_float2(last_input.position.x, last_input.position.y);

  metal_objc::SetRenderPipelineState(render_encoder,
                                     objc_renderer_state_.get());

  const PartitionedMesh& shape = stroke.GetShape();
  uint32_t group_count = shape.RenderGroupCount();
  absl::Span<const BrushCoat> coats = stroke.GetBrush().GetCoats();

  for (uint32_t coat_index = 0; coat_index < group_count; ++coat_index) {
    if (coat_index >= coats.size()) break;
    for (const BrushPaint& paint : coats[coat_index].paint_preferences) {
      if (paint.self_overlap == BrushPaint::SelfOverlap::kDiscard) {
        if (!metal_objc::SupportsSelfOverlapDiscard(
                objc_renderer_state_.get())) {
          // A stencil buffer is required to discard self-overlap. Hopefully
          // the next paint preference will have a compatible self overlap
          // setting.
          continue;
        }
        ClearStencilBuffer(render_encoder);
        metal_objc::SetDiscardSelfOverlapStencilState(
            render_encoder, objc_renderer_state_.get());

        if (request_shader_based_anti_aliasing_) {
          // When using shader-based AA, we need to draw the stroke twice
          // using the same stencil buffer to avoid the anti-aliased edges
          // showing up as outlines of the self-overlapping portions of the
          // stroke.
          DrawWithStencilAlreadySet(stroke, coat_index, paint, false,
                                    first_input_pos, last_input_pos,
                                    model_transform, view_transform,
                                    projection_transform, render_encoder);
        }
      } else {
        // No need to clear the stencil buffer, as a nil stencil state will
        // not use it. It will be cleared the next time it is needed.
        metal_objc::SetNoOpStencilState(render_encoder,
                                        objc_renderer_state_.get());
      }

      DrawWithStencilAlreadySet(
          stroke, coat_index, paint, request_shader_based_anti_aliasing_,
          first_input_pos, last_input_pos, model_transform, view_transform,
          projection_transform, render_encoder);

      // Only draw with the first compatible paint preference in each coat.
      break;
    }
  }
}

void MetalRenderer::DrawWithStencilAlreadySet(
    const Stroke& stroke, uint32_t coat_index, const BrushPaint& paint,
    bool request_shader_aa, simd_float2 first_input_pos,
    simd_float2 last_input_pos, simd_float4x4 model_transform,
    simd_float4x4 view_transform, simd_float4x4 projection_transform,
    void* render_encoder) {
  Color coat_color = EvaluateCoatColor(stroke.GetBrush(), paint);
  simd_float4 color_uniform = MakeColorUniform(coat_color);

  const PartitionedMesh& shape = stroke.GetShape();
  const MeshFormat& format = shape.RenderGroupFormat(coat_index);
  strokes_internal::StrokeVertex::FormatAttributeIndices attribute_indices =
      strokes_internal::StrokeVertex::FindAttributeIndices(format);
  absl::StatusOr<StrokeAttributeTypesAndOffsets> offsets_or_status =
      rendering::GetValidatedStrokeAttributeTypesAndOffsets(format,
                                                            attribute_indices);
  if (!offsets_or_status.ok()) return;
  StrokeAttributeTypesAndOffsets offsets = *std::move(offsets_or_status);

  // The position and opacity shift attribute is always present.
  if (offsets.position_and_opacity_shift.offset < 0) return;
  uint32_t position_offset =
      static_cast<uint32_t>(offsets.position_and_opacity_shift.offset);
  // Optional attributes, set to -1 if not present.
  int32_t hsl_offset =
      offsets.hsl_shift.has_value() ? offsets.hsl_shift->offset : -1;
  int32_t side_offset = offsets.side_derivative_and_label.offset;
  int32_t forward_offset = offsets.forward_derivative_and_label.offset;
  int32_t surface_offset =
      offsets.surface_uv_and_paint_animation_offset.has_value()
          ? offsets.surface_uv_and_paint_animation_offset->offset
          : -1;

  if (request_shader_aa && (forward_offset < 0 || side_offset < 0)) {
    ABSL_LOG(WARNING)
        << "INKMetalRenderer shader-based anti-aliasing requested, but mesh "
           "format does not contain side derivative and forward derivative "
           "attributes.";
    return;
  }

  // Positive stride indicates packed vertex data.
  int32_t vertex_stride = static_cast<int32_t>(format.PackedVertexStride());

  absl::Span<const Mesh> meshes = shape.RenderGroupMeshes(coat_index);
  for (size_t mesh_index = 0; mesh_index < meshes.size(); ++mesh_index) {
    const Mesh& mesh = meshes[mesh_index];
    absl::Span<const std::byte> vertex_data = mesh.RawVertexData();
    absl::Span<const std::byte> index_data = mesh.RawIndexData();
    uint32_t triangle_count = mesh.TriangleCount();

    if (triangle_count == 0) continue;

    simd_float4 position_unpacking_transform =
        GetMeshUnpackingTransform(mesh, attribute_indices.position);
    simd_float4 side_derivative_unpacking_transform =
        (request_shader_aa && attribute_indices.side_derivative >= 0)
            ? GetMeshUnpackingTransform(mesh, attribute_indices.side_derivative)
            : simd_make_float4(0, 0, 0, 0);
    simd_float4 forward_derivative_unpacking_transform =
        (request_shader_aa && attribute_indices.forward_derivative >= 0)
            ? GetMeshUnpackingTransform(mesh,
                                        attribute_indices.forward_derivative)
            : simd_make_float4(0, 0, 0, 0);

    Uniforms uniforms{
        .model_transform = model_transform,
        .view_transform = view_transform,
        .projection_transform = projection_transform,
        .color = color_uniform,
        .position_unpacking_transform = position_unpacking_transform,
        .side_derivative_unpacking_transform =
            side_derivative_unpacking_transform,
        .forward_derivative_unpacking_transform =
            forward_derivative_unpacking_transform,
        .vertex_stride = vertex_stride,
        .position_and_opacity_shift_offset = position_offset,
        .hsl_shift_offset = hsl_offset,
        .side_derivative_and_label_offset =
            request_shader_aa ? side_offset : -1,
        .forward_derivative_and_label_offset =
            request_shader_aa ? forward_offset : -1,
        .surface_uv_and_paint_animation_offset_offset = surface_offset,
        .brush_size = stroke.GetBrush().GetSize(),
        .first_input_pos = first_input_pos,
        .last_input_pos = last_input_pos,
    };
    FillTextureLayers(paint, render_encoder, std::begin(uniforms.layers),
                      &uniforms.texture_layer_count);

    if (!mesh.HasCachedRenderingData<MeshBuffers>()) {
      mesh.SetCachedRenderingData(MeshBuffers{
          .objc_ptr = metal_objc::CreateINKMeshBuffers(
              objc_renderer_state_.get(), vertex_data.data(),
              vertex_data.size(), index_data.data(), index_data.size())});
    }
    metal_objc::DrawIndexedTriangles(
        render_encoder, &uniforms, sizeof(Uniforms), triangle_count,
        mesh.IndexStride(),
        mesh.GetCachedRenderingData<MeshBuffers>().objc_ptr.get());
  }
}

void MetalRenderer::ClearStencilBuffer(void* render_encoder) {
  metal_objc::SetClearStencilBufferStencilState(render_encoder,
                                                objc_renderer_state_.get());
  const Brush* brush = clear_stencil_stroke_.GetBrush();
  ABSL_CHECK_NE(brush, nullptr);
  absl::Span<const BrushCoat> coats = brush->GetCoats();
  ABSL_CHECK(!coats.empty() && !coats[0].paint_preferences.empty());
  const BrushPaint& paint = coats[0].paint_preferences[0];
  DrawWithStencilAlreadySet(clear_stencil_stroke_, 0, paint, false,
                            simd_make_float2(0, 0), simd_make_float2(0, 0),
                            matrix_identity_float4x4, matrix_identity_float4x4,
                            matrix_identity_float4x4, render_encoder);
}

Color MetalRenderer::EvaluateCoatColor(const Brush& brush,
                                       const BrushPaint& paint) {
  return ColorFunction::ApplyAll(paint.color_functions, brush.GetColor());
}

simd_float4 MetalRenderer::MakeColorUniform(const Color& color) {
  Color linear_color = color.InColorSpace(ColorSpace::kSrgb);
  Color::RgbaFloat float_color = linear_color.AsFloat(Color::Format::kLinear);
  return simd_make_float4(float_color.r, float_color.g, float_color.b,
                          float_color.a);
}

void MetalRenderer::FillTextureLayers(const BrushPaint& paint,
                                      void* render_encoder,
                                      ShaderTextureLayer* layers,
                                      uint32_t* layer_count) {
  *layer_count = 0;
  if (!metal_objc::SupportsTextureLookup(objc_renderer_state_.get())) {
    return;
  }

  for (const BrushPaint::TextureLayer& layer : paint.texture_layers) {
    if (*layer_count >= kMaxTextureLayers) {
      ABSL_LOG(WARNING) << "Exceeded maximum number of texture layers.";
      break;
    };

    std::string client_texture_id;
    BrushPaint::TextureWrap wrap_x = BrushPaint::TextureWrap::kRepeat;
    BrushPaint::TextureWrap wrap_y = BrushPaint::TextureWrap::kRepeat;
    ShaderTextureLayer layer_params;

    if (std::holds_alternative<BrushPaint::TilingTexture>(layer)) {
      const auto& tiling = std::get<BrushPaint::TilingTexture>(layer);
      client_texture_id = tiling.client_texture_id;
      wrap_x = tiling.wrap_x;
      wrap_y = tiling.wrap_y;
      layer_params = ShaderTextureLayer{
          .mapping_mode = kTextureMappingModeTiling,
          .blend_mode = static_cast<uint32_t>(tiling.blend_mode),
          .tiling_origin = static_cast<uint32_t>(tiling.origin),
          .tiling_size_unit = static_cast<uint32_t>(tiling.size_unit),
          .tiling_size = simd_make_float2(tiling.size.x, tiling.size.y),
          .tiling_offset = simd_make_float2(tiling.offset.x, tiling.offset.y),
          .tiling_rotation = tiling.rotation.ValueInRadians(),
      };
    } else if (std::holds_alternative<BrushPaint::StampingTexture>(layer)) {
      const auto& stamping = std::get<BrushPaint::StampingTexture>(layer);
      client_texture_id = stamping.client_texture_id;
      wrap_x = BrushPaint::TextureWrap::kClamp;
      wrap_y = BrushPaint::TextureWrap::kClamp;
      layer_params = ShaderTextureLayer{
          .mapping_mode = kTextureMappingModeStamping,
          .blend_mode = static_cast<uint32_t>(stamping.blend_mode),
          .tiling_origin = 0,
          .tiling_size_unit = 0,
          .tiling_size = simd_make_float2(0, 0),
          .tiling_offset = simd_make_float2(0, 0),
          .tiling_rotation = 0.0f,
      };
    } else {
      ABSL_LOG(WARNING) << "Texture layer type is unsupported.";
      continue;
    }

    if (client_texture_id.empty()) continue;

    void* texture = metal_objc::GetOrLoadMTLTexture(objc_renderer_state_.get(),
                                                    client_texture_id.c_str());
    if (!texture) {
      ABSL_LOG(WARNING) << "Failed to create MTLTexture for "
                        << client_texture_id;
      continue;
    }

    void* sampler = metal_objc::GetOrCreateMTLSamplerState(
        objc_renderer_state_.get(), wrap_x, wrap_y);
    if (!sampler) {
      ABSL_LOG(WARNING)
          << "Failed to create MTLSamplerState for texture wrap settings.";
      continue;
    }

    layers[*layer_count] = layer_params;

    // Tint's MSL generator ignores the binding index for texture layers,
    // and numbers each texture and each sampler sequentially starting from
    // 0. Note that the index here matches the index in the returned array,
    // not the index in `paint.texture_layers`, just in case any requested
    // layers failed to be initialized.
    metal_objc::SetFragmentTextureAndSampler(render_encoder, texture, sampler,
                                             *layer_count);
    (*layer_count)++;
  }
}

}  // namespace ink::rendering
