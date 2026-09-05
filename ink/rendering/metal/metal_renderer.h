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

#ifndef INK_RENDERING_METAL_METAL_RENDERER_H_
#define INK_RENDERING_METAL_METAL_RENDERER_H_

#include <simd/matrix_types.h>
#include <simd/vector_make.h>
#include <simd/vector_types.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_paint.h"
#include "ink/color/color.h"
#include "ink/rendering/metal/metal_renderer_objc_helper.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/stroke.h"

namespace ink::rendering {

// A utility for rendering Ink strokes using Metal.
class MetalRenderer {
 public:
  // LINT.IfChange(max_texture_layers)
  static constexpr int kMaxTextureLayers = 4;
  // LINT.ThenChange(
  //   ../webgpu/StrokeShader.wgsl:max_texture_layers,
  //   ../../brush/brush_paint.cc:max_texture_layers
  // )

  // One of the texture layers in the brush.
  //
  // Corresponds to the `TextureLayer` struct in `StrokeShader.wgsl`.
  //
  // Grouped into 16 byte chunks and padded to ensure 16 byte alignment, as
  // Metal copies uniform data in 16-byte increments.
  struct ShaderTextureLayer {
    // =========================================================================
    // 16 bytes total
    // =========================================================================
    uint32_t mapping_mode = 0;  // 4 bytes
    uint32_t blend_mode =
        static_cast<uint32_t>(BrushPaint::BlendMode::kModulate);  // 4 bytes
    uint32_t tiling_origin = static_cast<uint32_t>(
        BrushPaint::TextureOrigin::kStrokeSpaceOrigin);  // 4 bytes
    uint32_t tiling_size_unit = static_cast<uint32_t>(
        BrushPaint::TextureSizeUnit::kStrokeCoordinates);  // 4 bytes

    // =========================================================================
    // 16 bytes total
    // =========================================================================
    simd_float2 tiling_size = simd_make_float2(0, 0);    // 8 bytes
    simd_float2 tiling_offset = simd_make_float2(0, 0);  // 8 bytes

    // =========================================================================
    // *************** Only 4 bytes used total, padding added to make 16 bytes *
    // =========================================================================
    float tiling_rotation = 0.0f;  // 4 bytes
    float padding0 = 0.0f;
    float padding1 = 0.0f;
    float padding2 = 0.0f;
  };

  static_assert(sizeof(ShaderTextureLayer) % 16 == 0);

  // Uniforms passed to the vertex and fragment shaders.
  //
  // Corresponds to the `Uniforms` struct in `StrokeShader.wgsl`.
  //
  // Grouped into 16 byte chunks and padded to ensure 16 byte alignment, as
  // Metal copies uniform data in 16-byte increments.
  // LINT.IfChange(uniforms)
  struct Uniforms {
    // =========================================================================
    // 16 byte multiples each
    // =========================================================================
    simd_float4x4 model_transform;       // 64 bytes
    simd_float4x4 view_transform;        // 64 bytes
    simd_float4x4 projection_transform;  // 64 bytes

    // =========================================================================
    // 16 bytes each
    // =========================================================================
    simd_float4 color;                                   // 16 bytes
    simd_float4 position_unpacking_transform;            // 16 bytes
    simd_float4 side_derivative_unpacking_transform;     // 16 bytes
    simd_float4 forward_derivative_unpacking_transform;  // 16 bytes

    // =========================================================================
    // 16 bytes total
    // =========================================================================
    // Stride in bytes.
    //
    // This is positive for packed vertex data and negative for unpacked vertex
    // data.
    int32_t vertex_stride = 0;  // 4 bytes

    // Offset of the position and opacity shift attribute in bytes.
    //
    // This attribute is always present.
    uint32_t position_and_opacity_shift_offset = 0;  // 4 bytes

    // Offset of the HCL shift attribute in bytes.
    //
    // This attribute is optional, and a value of -1 indicates that the
    // attribute is not present.
    int32_t hcl_shift_offset = -1;  // 4 bytes

    // Offset of the side derivative and label attribute in bytes.
    //
    // This attribute is optional, and a value of -1 indicates that the
    // attribute is not present and therefore shader-based anti-aliasing will
    // not be performed.
    int32_t side_derivative_and_label_offset = -1;  // 4 bytes

    // =========================================================================
    // 16 bytes total
    // =========================================================================
    // Offset of the forward derivative and label attribute in bytes.
    //
    // This attribute is optional, and a value of -1 indicates that the
    // attribute is not present and therefore shader-based anti-aliasing will
    // not be performed.
    int32_t forward_derivative_and_label_offset = -1;  // 4 bytes

    // Offset of the surface UV and animation offset attribute in bytes.
    //
    // This attribute is optional, and a value of -1 indicates that the
    // attribute is not present.
    int32_t surface_uv_and_paint_animation_offset_offset = -1;  // 4 bytes

    // The size of the brush in stroke coordinate space units.
    float brush_size = 0.0f;  // 4 bytes

    // The number of texture layers in the brush, up to `max_texture_layers`.
    uint32_t texture_layer_count = 0;  // 4 bytes

    // =========================================================================
    // 16 bytes total
    // =========================================================================
    // The position of the first input in stroke coordinate space.
    simd_float2 first_input_pos;  // 8 bytes

    // The position of the last input in stroke coordinate space.
    simd_float2 last_input_pos;  // 8 bytes

    // =========================================================================
    // 16 byte multiples each
    // =========================================================================
    // LINT.IfChange(texture_layers_tuple)
    ShaderTextureLayer layers[kMaxTextureLayers] = {};
    // LINT.ThenChange(:max_texture_layers)
  };
  // LINT.ThenChange(../webgpu/StrokeShader.wgsl:uniforms)

  static_assert(sizeof(Uniforms) % 16 == 0);

  // Creates a `MetalRenderer`.
  //
  // `color_pixel_format`: `MTLPixelFormat` cast to uint64_t.
  // `stencil_pixel_format`: `MTLPixelFormat` cast to uint64_t.
  // `sample_count`: The number of samples used for multisampling. Set to
  //   1 to enable shader-based anti-aliasing instead.
  // `texture_bitmap_store`: Optional opaque pointer to
  // `id<INKTextureBitmapStore>`.
  static absl::StatusOr<MetalRenderer> Create(
      void* device, uint64_t color_pixel_format, uint64_t stencil_pixel_format,
      std::optional<int> sample_count = std::nullopt,
      void* absl_nullable texture_bitmap_store = nullptr);

  MetalRenderer(const MetalRenderer&) = delete;
  MetalRenderer& operator=(const MetalRenderer&) = delete;
  MetalRenderer(MetalRenderer&&) noexcept = default;
  MetalRenderer& operator=(MetalRenderer&&) noexcept = default;
  ~MetalRenderer() = default;

  // Draws the given `in_progress_stroke` to the given `render_encoder`.
  //
  // The stroke is drawn using the given projection matrix and color, and is
  // rendered into the target specified by the render encoder.
  //
  // `in_progress_stroke`: The stroke to be drawn.
  // `model_transform`: The model transform to be applied in the vertex shader.
  //   Must transform from stroke coordinates to world coordinates.
  // `view_transform`: The view transform to be applied in the vertex shader.
  //   Must transform from world coordinates to view coordinates.
  // `projection_transform`: The projection matrix to be applied in the vertex
  //   shader. Must transform from world coordinates to Y-up normalized device
  //   coordinates (the lower-left corner is (-1, -1) and the upper-right corner
  //   is (1, 1)).
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  void Draw(const InProgressStroke& in_progress_stroke,
            simd_float4x4 model_transform, simd_float4x4 view_transform,
            simd_float4x4 projection_transform, void* render_encoder);

  // Draws the given `stroke` to the given `render_encoder`.
  //
  // The stroke is drawn using the given projection matrix and color, and is
  // rendered into the target specified by the render encoder.
  //
  // `stroke`: The stroke to be drawn.
  // `model_transform`: The model transform to be applied in the vertex shader.
  //   Must transform from stroke coordinates to world coordinates.
  // `view_transform`: The view transform to be applied in the vertex shader.
  //   Must transform from world coordinates to view coordinates.
  // `projection_transform`: The projection matrix to be applied in the vertex
  //   shader. Must transform from world coordinates to Y-up normalized device
  //   coordinates (the lower-left corner is (-1, -1) and the upper-right
  //   corner is (1, 1)).
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  void Draw(const Stroke& stroke, simd_float4x4 model_transform,
            simd_float4x4 view_transform, simd_float4x4 projection_transform,
            void* render_encoder);

 private:
  // Constructs a `MetalRenderer` with initialized Metal pipeline objects.
  MetalRenderer(
      std::unique_ptr<void, std::function<void(void*)>> objc_renderer_state,
      bool request_shader_based_anti_aliasing);

  // Draws an in-progress stroke with the stencil state already configured.
  //
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  void DrawWithStencilAlreadySet(
      const InProgressStroke& in_progress_stroke, uint32_t coat_index,
      const BrushPaint& paint, bool request_shader_aa,
      simd_float2 first_input_pos, simd_float2 last_input_pos,
      simd_float4x4 model_transform, simd_float4x4 view_transform,
      simd_float4x4 projection_transform, void* render_encoder);

  // Draws a completed stroke with the stencil state already configured.
  //
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  void DrawWithStencilAlreadySet(
      const Stroke& stroke, uint32_t coat_index, const BrushPaint& paint,
      bool request_shader_aa, simd_float2 first_input_pos,
      simd_float2 last_input_pos, simd_float4x4 model_transform,
      simd_float4x4 view_transform, simd_float4x4 projection_transform,
      void* render_encoder);

  // Clears the stencil buffer using `clear_stencil_stroke_`.
  //
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  void ClearStencilBuffer(void* render_encoder);

  Color EvaluateCoatColor(const Brush& brush, const BrushPaint& paint);
  simd_float4 MakeColorUniform(const Color& color);

  // Creates the `ShaderTextureLayer`s for the given `BrushPaint` and sets the
  // fragment textures and samplers on the given render encoder.
  //
  // `render_encoder`: Opaque pointer to `id<MTLRenderCommandEncoder>`.
  // `layers`: Pointer to `Uniforms.layers`.
  // `layer_count`: Pointer to `Uniforms.texture_layer_count`.
  void FillTextureLayers(const BrushPaint& paint, void* render_encoder,
                         ShaderTextureLayer* layers, uint32_t* layer_count);

  // Opaque pointer to `INKMetalRendererState` which handles cleanup of ObjC
  // state when the renderer is destroyed.
  std::unique_ptr<void, std::function<void(void*)>> objc_renderer_state_;

  // Whether the client has requested shader-based anti-aliasing.
  //
  // If false, MSAA is enabled and anti-aliasing is performed by the GPU. If
  // true, anti-aliasing is performed in the shader. If a coat has been stripped
  // of the appropriate vertex attributes for shader-based anti-aliasing, it
  // will not be rendered at all.
  bool request_shader_based_anti_aliasing_ = false;

  // An invisible stroke used for clearing the stencil buffer.
  //
  // This is created once during initialization and reused for each render pass.
  // It is designed to cover the entire viewport when drawn with identity MVP
  // matrices, by taking advantage of the normalized coordinate space ranging
  // from (0, 0) to (1, 1). To do this, a single input point is specified at the
  // center of the viewport, with a brush size large enough to cover the entire
  // viewport.
  //
  // A stroke is drawn rather than a simple quad to be able to use the same
  // rendering pipeline as the one used for drawing actual strokes. This saves
  // not only code complexity, but also improves performance as changing the
  // render pipeline is more expensive than a simple draw call.
  InProgressStroke clear_stencil_stroke_;
};

}  // namespace ink::rendering

#endif  // INK_RENDERING_METAL_METAL_RENDERER_H_
