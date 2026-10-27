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
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
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

  // Creates an RGBA8Unorm texture from `rgba_data` (`width * height * 4` bytes)
  // and registers it under `id`.
  absl::StatusOr<wgpu::Texture> SetRgba8Texture(
      absl::string_view id, const wgpu::Device& device, uint32_t width,
      uint32_t height, absl::Span<const uint8_t> rgba_data);

  // Retrieves the texture for `id`, or a null `wgpu::Texture` if not found.
  wgpu::Texture GetTexture(absl::string_view id) const;

 private:
  absl::flat_hash_map<std::string, wgpu::Texture> textures_;
};

// Dawn C++ renderer for Ink strokes using `StrokeShader.wgsl`.
class WebGpuRenderer {
 public:
  // Creates and initializes a `WebGpuRenderer` on `device`.
  static absl::StatusOr<WebGpuRenderer> Create(
      wgpu::Device device,
      std::shared_ptr<const WebGpuTextureStore> texture_store = nullptr,
      wgpu::TextureFormat color_format = wgpu::TextureFormat::RGBA8Unorm,
      wgpu::TextureFormat depth_stencil_format = wgpu::TextureFormat::Stencil8);

  // Encodes draw commands for an `InProgressStroke` into `render_pass`.
  absl::Status Draw(const InProgressStroke& stroke,
                    const AffineTransform& object_to_canvas,
                    const AffineTransform& canvas_to_ndc,
                    wgpu::RenderPassEncoder& render_pass);

  // Encodes draw commands for a finished `Stroke` into `render_pass`.
  absl::Status Draw(const Stroke& stroke,
                    const AffineTransform& object_to_canvas,
                    const AffineTransform& canvas_to_ndc,
                    wgpu::RenderPassEncoder& render_pass);

  // Convenience render-pass lifecycle methods for rendering one or more strokes
  // into `target_texture` with an automatically managed stencil attachment.
  absl::Status BeginRenderPass(wgpu::Texture target_texture,
                               const Color& clear_color = Color::White());
  absl::Status DrawInActivePass(const InProgressStroke& stroke,
                                const AffineTransform& object_to_canvas,
                                const AffineTransform& canvas_to_ndc);
  absl::Status DrawInActivePass(const Stroke& stroke,
                                const AffineTransform& object_to_canvas,
                                const AffineTransform& canvas_to_ndc);
  absl::Status EndRenderPass();

  const wgpu::Device& GetDevice() const { return device_; }

  // Per-layer texture metadata matching `struct TextureLayer` in
  // `StrokeShader.wgsl`.
  struct alignas(16) ShaderTextureLayer {
    uint32_t mapping_mode = 0;
    uint32_t blend_mode = 0;
    uint32_t tiling_origin = 0;
    uint32_t tiling_size_unit = 0;
    std::array<float, 2> tiling_size = {1.0f, 1.0f};
    std::array<float, 2> tiling_offset = {0.0f, 0.0f};
    float tiling_rotation = 0.0f;
    float padding0 = 0.0f;
    float padding1 = 0.0f;
    float padding2 = 0.0f;
  };
  static_assert(sizeof(ShaderTextureLayer) == 48);

  // Uniform block matching `struct Uniforms` in `StrokeShader.wgsl`.
  struct alignas(16) Uniforms {
    std::array<float, 16> model_transform = {};
    std::array<float, 16> view_transform = {};
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
    int32_t surface_uv_and_animation_offset_offset = -1;
    float brush_size = 0.0f;
    uint32_t texture_layer_count = 0;
    std::array<float, 2> first_input_pos = {0.0f, 0.0f};
    std::array<float, 2> last_input_pos = {0.0f, 0.0f};
    std::array<ShaderTextureLayer, 4> layers = {};
  };
  static_assert(sizeof(Uniforms) == 496);
  static_assert(sizeof(Uniforms) % 16 == 0);

 private:
  WebGpuRenderer(wgpu::Device device,
                 std::shared_ptr<const WebGpuTextureStore> texture_store,
                 wgpu::TextureFormat color_format,
                 wgpu::TextureFormat depth_stencil_format,
                 wgpu::BindGroupLayout bind_group_layout,
                 wgpu::RenderPipeline pipeline_normal,
                 wgpu::RenderPipeline pipeline_discard,
                 wgpu::Texture dummy_texture,
                 wgpu::TextureView dummy_texture_view,
                 wgpu::Sampler default_sampler);

  absl::Status EncodeMeshDraw(const Uniforms& uniforms, const BrushPaint& paint,
                              absl::Span<const std::byte> raw_vertex_data,
                              absl::Span<const std::byte> raw_index_data,
                              size_t index_stride, uint32_t index_count,
                              wgpu::RenderPassEncoder& render_pass);

  wgpu::Device device_;
  std::shared_ptr<const WebGpuTextureStore> texture_store_;
  wgpu::TextureFormat color_format_;
  wgpu::TextureFormat depth_stencil_format_;
  wgpu::BindGroupLayout bind_group_layout_;
  wgpu::RenderPipeline pipeline_normal_;
  wgpu::RenderPipeline pipeline_discard_;
  wgpu::Texture dummy_texture_;
  wgpu::TextureView dummy_texture_view_;
  wgpu::Sampler default_sampler_;

  // State for active render pass started via `BeginRenderPass`.
  wgpu::CommandEncoder active_command_encoder_;
  wgpu::Texture active_depth_stencil_texture_;
  wgpu::TextureView active_depth_stencil_view_;
  wgpu::RenderPassEncoder active_render_pass_;
};

}  // namespace ink::rendering::webgpu

#endif  // INK_RENDERING_WEBGPU_WEBGPU_RENDERER_H_
