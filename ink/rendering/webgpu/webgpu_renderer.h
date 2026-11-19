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

#ifndef INK_RENDERING_WEBGPU_WEBGPU_RENDERER_H_
#define INK_RENDERING_WEBGPU_WEBGPU_RENDERER_H_

#include <webgpu/webgpu_cpp.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "ink/brush/brush_paint.h"
#include "ink/color/color.h"
#include "ink/geometry/affine_transform.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/stroke.h"

namespace ink::rendering::webgpu {

// Stores textures mapped by their client texture ID for use by WebGpuRenderer.
class WebGpuTextureStore {
 public:
  WebGpuTextureStore() = default;

  // Registers a pre-created WebGPU texture for `id`.
  void SetTexture(absl::string_view id, wgpu::Texture texture);

  // Retrieves the texture for `id`, or a null `wgpu::Texture` if not found.
  wgpu::Texture GetTexture(absl::string_view id) const;

 private:
  absl::flat_hash_map<std::string, wgpu::Texture> textures_;
};

// Dawn C++ renderer for Ink strokes using `StrokeShader.wgsl`.
class WebGpuRenderer {
 public:
  // Creates and initializes a `WebGpuRenderer` on `device`.
  //
  // `sample_count`: The number of samples used for multisampling, or
  //   `std::nullopt` to enable shader-based anti-aliasing instead. Set to `1`
  //   to disable both MSAA and shader-based anti-aliasing.
  static absl::StatusOr<WebGpuRenderer> Create(
      wgpu::Device device,
      absl_nullable std::shared_ptr<const WebGpuTextureStore> texture_store =
          nullptr,
      wgpu::TextureFormat color_format = wgpu::TextureFormat::RGBA8Unorm,
      wgpu::TextureFormat depth_stencil_format = wgpu::TextureFormat::Stencil8,
      std::optional<int> sample_count = std::nullopt);

  // Encodes draw commands for an `InProgressStroke` into `render_pass`.
  //
  // `object_to_canvas`: An affine transform from stroke coordinates to
  //   render-target hardware pixel coordinates (e.g., incorporating device
  //   pixel ratio / display scale rather than virtual CSS pixels or UI points)
  //   so shader-based anti-aliasing computes pixel coverage accurately.
  // `canvas_to_ndc`: An affine transform mapping render-target hardware pixel
  //   coordinates to normalized device coordinates (`[-1, 1]`).
  // `animator_clock_state`: The current state of the paint animation clock,
  //   used to calculate the animation phase for animated brush paints.
  absl::Status Draw(const InProgressStroke& in_progress_stroke,
                    const AffineTransform& object_to_canvas,
                    const AffineTransform& canvas_to_ndc,
                    wgpu::RenderPassEncoder& render_pass,
                    absl::Duration animator_clock_state = absl::ZeroDuration());

  // Encodes draw commands for a finished `Stroke` into `render_pass`.
  //
  // `object_to_canvas`: An affine transform from stroke coordinates to
  //   render-target hardware pixel coordinates (e.g., incorporating device
  //   pixel ratio / display scale rather than virtual CSS pixels or UI points)
  //   so shader-based anti-aliasing computes pixel coverage accurately.
  // `canvas_to_ndc`: An affine transform mapping render-target hardware pixel
  //   coordinates to normalized device coordinates (`[-1, 1]`).
  // `animator_clock_state`: The current state of the paint animation clock,
  //   used to calculate the animation phase for animated brush paints.
  absl::Status Draw(const Stroke& stroke,
                    const AffineTransform& object_to_canvas,
                    const AffineTransform& canvas_to_ndc,
                    wgpu::RenderPassEncoder& render_pass,
                    absl::Duration animator_clock_state = absl::ZeroDuration());

  // Convenience render-pass lifecycle methods for rendering one or more strokes
  // into `target_texture` with an automatically managed stencil attachment.
  // `object_to_canvas`, `canvas_to_ndc`, and `animator_clock_state` have the
  // same meaning as in `Draw`.
  absl::Status BeginRenderPass(wgpu::Texture target_texture,
                               const Color& clear_color = Color::White());
  absl::Status DrawInActivePass(
      const InProgressStroke& in_progress_stroke,
      const AffineTransform& object_to_canvas,
      const AffineTransform& canvas_to_ndc,
      absl::Duration animator_clock_state = absl::ZeroDuration());
  absl::Status DrawInActivePass(
      const Stroke& stroke, const AffineTransform& object_to_canvas,
      const AffineTransform& canvas_to_ndc,
      absl::Duration animator_clock_state = absl::ZeroDuration());
  absl::Status EndRenderPass();

  const wgpu::Device& GetDevice() const { return device_; }
  wgpu::TextureFormat GetColorFormat() const { return color_format_; }
  std::optional<int> GetSampleCount() const { return sample_count_; }

  // Per-layer texture metadata matching `struct TextureLayer` in
  // `StrokeShader.wgsl`.
  // LINT.IfChange(texture_layer)
  struct alignas(16) ShaderTextureLayer {
    uint32_t mapping_mode = 0;
    uint32_t blend_mode = 0;
    uint32_t tiling_origin = 0;
    uint32_t tiling_size_unit = 0;
    std::array<float, 2> tiling_size = {1.0f, 1.0f};
    std::array<float, 2> tiling_offset = {0.0f, 0.0f};
    float tiling_rotation = 0.0f;
    uint32_t stamping_animation_frames = 1;
    uint32_t stamping_animation_rows = 1;
    uint32_t stamping_animation_columns = 1;
    uint32_t stamping_animation_repeat_mode =
        static_cast<uint32_t>(BrushPaint::AnimationRepeatMode::kRestart);
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
  };
  static_assert(sizeof(ShaderTextureLayer) == 64);
  // LINT.ThenChange(StrokeShader.wgsl:texture_layer,
  // ../metal/metal_renderer.h:texture_layer)

  // Uniform block matching `struct Uniforms` in `StrokeShader.wgsl`.
  // LINT.IfChange(uniforms)
  struct alignas(16) Uniforms {
    std::array<float, 16> object_to_canvas_transform = {};
    std::array<float, 16> projection_transform = {};
    std::array<float, 4> color_oklab = {};
    std::array<float, 4> position_unpacking_transform = {};
    std::array<float, 4> side_derivative_unpacking_transform = {};
    std::array<float, 4> forward_derivative_unpacking_transform = {};
    int32_t vertex_stride = 0;
    uint32_t position_and_opacity_shift_offset = 0;
    int32_t hcl_shift_offset = -1;
    int32_t side_derivative_and_label_offset = -1;
    int32_t forward_derivative_and_label_offset = -1;
    int32_t surface_uv_and_paint_animation_offset_offset = -1;
    float brush_size = 0.0f;
    uint32_t texture_layer_count = 0;
    std::array<float, 2> first_input_pos = {0.0f, 0.0f};
    std::array<float, 2> last_input_pos = {0.0f, 0.0f};
    float paint_animation_progress = 0.0f;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
    std::array<ShaderTextureLayer, 4> layers = {};
  };
  static_assert(sizeof(Uniforms) == 512);
  static_assert(sizeof(Uniforms) % 16 == 0);
  // LINT.ThenChange(StrokeShader.wgsl:uniforms,
  // ../metal/metal_renderer.h:uniforms)

 private:
  WebGpuRenderer(
      wgpu::Device device,
      absl_nullable std::shared_ptr<const WebGpuTextureStore> texture_store,
      wgpu::TextureFormat color_format,
      wgpu::TextureFormat depth_stencil_format, std::optional<int> sample_count,
      wgpu::BindGroupLayout bind_group_layout,
      wgpu::RenderPipeline pipeline_normal,
      wgpu::RenderPipeline pipeline_discard,
      wgpu::RenderPipeline pipeline_clear_stencil,
      wgpu::Texture placeholder_texture,
      wgpu::TextureView placeholder_texture_view,
      wgpu::Sampler default_sampler);

  void ClearStencilBuffer(wgpu::RenderPassEncoder& render_pass) const;
  wgpu::Sampler GetOrCreateSampler(wgpu::AddressMode address_mode_u,
                                   wgpu::AddressMode address_mode_v);
  absl::Status EncodeMeshDraw(const Uniforms& uniforms, const BrushPaint& paint,
                              const wgpu::Buffer& vertex_buffer,
                              const wgpu::Buffer& index_buffer,
                              size_t index_stride, uint32_t index_count,
                              wgpu::RenderPassEncoder& render_pass);
  absl::Status EnsureActiveRenderPass(absl::string_view caller) const;

  // LINT.IfChange(max_texture_layers)
  static constexpr uint32_t kMaxTextureLayers = 4;
  // LINT.ThenChange(StrokeShader.wgsl:max_texture_layers)

  wgpu::Device device_;
  absl_nullable std::shared_ptr<const WebGpuTextureStore> texture_store_;
  wgpu::TextureFormat color_format_;
  wgpu::TextureFormat depth_stencil_format_;
  std::optional<int> sample_count_;
  wgpu::BindGroupLayout bind_group_layout_;
  wgpu::RenderPipeline pipeline_normal_;
  wgpu::RenderPipeline pipeline_discard_;
  wgpu::RenderPipeline pipeline_clear_stencil_;
  wgpu::Texture placeholder_texture_;
  std::array<wgpu::TextureView, kMaxTextureLayers> placeholder_texture_views_;
  std::array<wgpu::Sampler, kMaxTextureLayers> default_samplers_;
  absl::flat_hash_map<std::pair<wgpu::AddressMode, wgpu::AddressMode>,
                      wgpu::Sampler>
      samplers_;

  // State for active render pass started via `BeginRenderPass`.
  wgpu::CommandEncoder active_command_encoder_;
  wgpu::Texture active_msaa_color_texture_;
  wgpu::TextureView active_msaa_color_view_;
  wgpu::Texture active_depth_stencil_texture_;
  wgpu::TextureView active_depth_stencil_view_;
  wgpu::RenderPassEncoder active_render_pass_;
};

}  // namespace ink::rendering::webgpu

#endif  // INK_RENDERING_WEBGPU_WEBGPU_RENDERER_H_
