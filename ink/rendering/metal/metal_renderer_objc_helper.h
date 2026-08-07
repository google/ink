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

#ifndef INK_RENDERING_METAL_METAL_RENDERER_OBJC_HELPER_H_
#define INK_RENDERING_METAL_METAL_RENDERER_OBJC_HELPER_H_

#ifdef __cplusplus

#include <CoreFoundation/CFBase.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "absl/base/nullability.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_paint.h"

namespace ink::rendering::metal_objc {

// Initializes Metal pipeline states and texture caches for rendering.
// Returns an opaque pointer to an `INKMetalRendererState` instance, wrapped
// in a unique_ptr with a deleter which handles cleanup on the ObjC side.
// `device`: Opaque pointer to `id<MTLDevice>` used for rendering.
// `color_pixel_format`: `MTLPixelFormat` cast to `uint64_t`.
// `stencil_pixel_format`: `MTLPixelFormat` cast to `uint64_t`.
// `sample_count`: The number of samples used for multisampling, or nullopt to
//   use shader-based AA.
// `texture_bitmap_store`: Optional opaque pointer to
// `id<INKTextureBitmapStore>` used for
//   texture lookup.
absl::StatusOr<std::unique_ptr<void, std::function<void(void*)>>>
CreateINKMetalRendererState(
    void* device_ptr, uint64_t color_pixel_format_val,
    uint64_t stencil_pixel_format_val, std::optional<int> sample_count,
    void* absl_nullable texture_bitmap_store_ptr = nullptr);

// The functions below take opaque pointers to ObjC objects:
// - `renderer_state_ptr` is an opaque pointer to `INKMetalRendererState`.
// - `texture_ptr` is an opaque pointer to `id<MTLTexture>`.
// - `sampler_ptr` is an opaque pointer to `id<MTLSamplerState>`.
// - `render_encoder_ptr` is an opaque pointer to `id<MTLRenderCommandEncoder>`.
// - `renderer_state_ptr` is an opaque pointer to `INKMetalRendererState`.

// Creates Metal mesh buffers (vertex and index buffers) for an Ink mesh.
// Returns an opaque pointer to an `INKMeshBuffers` instance, wrapped in a
// shared_ptr with a deleter which handles cleanup on the ObjC side. This uses
// shared_ptr so it can be stored on a `std::any` metadata slot on the `Mesh`,
// which only allows copyable types.
// `vertex_data`: Pointer to the vertex data to be stored in the buffers.
// `vertex_data_bytes`: Size of the vertex data in bytes.
// `index_data`: Pointer to the index data to be stored in the buffers.
// `index_data_bytes`: Size of the index data in bytes.
absl_nullable std::shared_ptr<void> CreateINKMeshBuffers(
    void* renderer_state_ptr, const void* vertex_data, size_t vertex_data_bytes,
    const void* index_data, size_t index_data_bytes);

// Sets render pipeline state on encoder.
void SetRenderPipelineState(void* render_encoder_ptr, void* renderer_state_ptr);

// Sets discard self-overlap stencil state on encoder.
void SetDiscardSelfOverlapStencilState(void* render_encoder_ptr,
                                       void* renderer_state_ptr);

// Sets clear stencil buffer stencil state on encoder.
void SetClearStencilBufferStencilState(void* render_encoder_ptr,
                                       void* renderer_state_ptr);

// Sets no-op stencil state on encoder.
void SetNoOpStencilState(void* render_encoder_ptr, void* renderer_state_ptr);

// Binds fragment texture and sampler state to encoder.
void SetFragmentTextureAndSampler(void* render_encoder_ptr, void* texture_ptr,
                                  void* sampler_ptr, uint32_t index);

// Allocates Metal buffers and issues indexed triangle draw calls.
void DrawIndexedTriangles(void* render_encoder_ptr, const void* uniforms_data,
                          size_t uniforms_data_bytes, uint32_t triangle_count,
                          size_t index_stride,
                          void* absl_nullable mesh_buffers_ptr);

// Retrieves or loads a Metal texture by texture ID.
void* GetOrLoadMTLTexture(void* renderer_state_ptr,
                          const char* client_texture_id);

// Retrieves or creates a Metal sampler state.
void* GetOrCreateMTLSamplerState(void* renderer_state_ptr,
                                 BrushPaint::TextureWrap wrap_x,
                                 BrushPaint::TextureWrap wrap_y);

// Whether the renderer has the stencil states required for discarding
// self-overlap.
bool SupportsSelfOverlapDiscard(void* renderer_state_ptr);

// Whether the renderer has a texture bitmap store for texture lookup.
bool SupportsTextureLookup(void* renderer_state_ptr);

}  // namespace ink::rendering::metal_objc

#endif  // __cplusplus

#endif  // INK_RENDERING_METAL_METAL_RENDERER_OBJC_HELPER_H_
