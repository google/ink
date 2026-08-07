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

#ifndef INK_RENDERING_METAL_METAL_RENDERER_NATIVE_H_
#define INK_RENDERING_METAL_METAL_RENDERER_NATIVE_H_

// C-compatible library header for Kotlin-native bindings.

#include <simd/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Creates a heap-allocated `ink::rendering::MetalRenderer`, returning a raw
// pointer to it. `device` is a pointer to a MTLDevice to use for rendering.
// `color_pixel_format` and `stencil_pixel_format` are the `MTLPixelFormat` of
// the color and stencil textures to render to. `sample_count` is the number of
// samples per pixel for MSAA. If -1, shader-based antialiasing will be used
// instead.
int64_t MetalRendererNative_create(
    void* device, uint64_t color_pixel_format, uint64_t stencil_pixel_format,
    int sample_count,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

// Draws an in-progress stroke using the given render encoder. `render_encoder`
// is a pointer to a MTLRenderCommandEncoder. `in_progress_stroke_native_ptr` is
// a raw pointer to a native `ink::InProgressStroke`. The remaining parameters
// are the elements of the model, view, and projection transforms.
void MetalRendererNative_drawInProgressStroke(
    int64_t native_ptr, void* render_encoder,
    int64_t in_progress_stroke_native_ptr, float model_transform_m00,
    float model_transform_m10, float model_transform_m20,
    float model_transform_m01, float model_transform_m11,
    float model_transform_m21, float view_transform_m00,
    float view_transform_m10, float view_transform_m20,
    float view_transform_m01, float view_transform_m11,
    float view_transform_m21, float projection_transform_m00,
    float projection_transform_m10, float projection_transform_m20,
    float projection_transform_m01, float projection_transform_m11,
    float projection_transform_m21);

// Draws a completed stroke using the given render encoder. `render_encoder`
// is a pointer to a MTLRenderCommandEncoder. `stroke_native_ptr` is a raw
// pointer to a native `ink::Stroke`. The remaining parameters are the elements
// of the model, view, and projection transforms.
void MetalRendererNative_drawStroke(
    int64_t native_ptr, void* render_encoder, int64_t stroke_native_ptr,
    float model_transform_m00, float model_transform_m10,
    float model_transform_m20, float model_transform_m01,
    float model_transform_m11, float model_transform_m21,
    float view_transform_m00, float view_transform_m10,
    float view_transform_m20, float view_transform_m01,
    float view_transform_m11, float view_transform_m21,
    float projection_transform_m00, float projection_transform_m10,
    float projection_transform_m20, float projection_transform_m01,
    float projection_transform_m11, float projection_transform_m21);

// Deletes the heap-allocated `ink::rendering::MetalRenderer`.
void MetalRendererNative_free(int64_t native_ptr);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_RENDERING_METAL_METAL_RENDERER_NATIVE_H_
