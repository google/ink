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

#include "ink/rendering/metal/metal_renderer_native.h"

#include <simd/matrix.h>
#include <simd/types.h>
#include <simd/vector_make.h>

#include <cstdint>
#include <memory>
#include <optional>

#include "absl/status/statusor.h"
#include "ink/rendering/metal/metal_renderer.h"
#include "ink/rendering/metal/metal_renderer_objc_helper.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native_helper.h"
#include "ink/strokes/internal/jni/stroke_native_helper.h"
#include "ink/strokes/stroke.h"

using ::ink::InProgressStroke;
using ::ink::Stroke;
using ::ink::native::CastToInProgressStrokeWrapper;
using ::ink::native::CastToStroke;
using ::ink::rendering::MetalRenderer;

int64_t NewNativeMetalRenderer(MetalRenderer& metal_renderer) {
  return reinterpret_cast<int64_t>(
      new MetalRenderer(std::move(metal_renderer)));
}

MetalRenderer& CastToMetalRenderer(int64_t native_ptr) {
  return *reinterpret_cast<MetalRenderer*>(native_ptr);
}

void DeleteNativeMetalRenderer(int64_t native_ptr) {
  delete reinterpret_cast<MetalRenderer*>(native_ptr);
}

simd_float4x4 AffineTransformToSimdMatrix(float m00, float m10, float m20,
                                          float m01, float m11, float m21) {
  // Constructed by columns
  return simd_matrix(
      simd_make_float4(m00, m01, 0, 0), simd_make_float4(m10, m11, 0, 0),
      simd_make_float4(0, 0, 1, 0), simd_make_float4(m20, m21, 0, 1));
}

extern "C" {

int64_t MetalRendererNative_create(
    void* device, uint64_t color_pixel_format, uint64_t stencil_pixel_format,
    int sample_count,
    void* (*texture_for_id_callback)(int64_t metal_renderer_native_ptr,
                                     const char* texture_id),
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  std::shared_ptr<void> texture_bitmap_store;
  if (texture_for_id_callback != nullptr) {
    texture_bitmap_store =
        ink::rendering::metal_objc::CreateKotlinTextureStoreWrapper(
            texture_for_id_callback);
  }

  absl::StatusOr<MetalRenderer> metal_renderer = MetalRenderer::Create(
      device, color_pixel_format, stencil_pixel_format,
      sample_count >= 0 ? std::make_optional(sample_count) : std::nullopt,
      texture_bitmap_store.get());

  if (!metal_renderer.ok()) {
    throw_from_status_callback(/*jni_env=*/nullptr,
                               static_cast<int>(metal_renderer.status().code()),
                               metal_renderer.status().ToString().c_str());
    return 0;
  }
  int64_t native_ptr = NewNativeMetalRenderer(*metal_renderer);
  if (texture_bitmap_store != nullptr) {
    ink::rendering::metal_objc::SetKotlinMetalRendererNativePtr(
        texture_bitmap_store.get(), native_ptr);
  }
  return native_ptr;
}

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
    float projection_transform_m21) {
  MetalRenderer& metal_renderer = CastToMetalRenderer(native_ptr);
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(in_progress_stroke_native_ptr).Stroke();
  simd_float4x4 model_transform = AffineTransformToSimdMatrix(
      model_transform_m00, model_transform_m10, model_transform_m20,
      model_transform_m01, model_transform_m11, model_transform_m21);
  simd_float4x4 view_transform = AffineTransformToSimdMatrix(
      view_transform_m00, view_transform_m10, view_transform_m20,
      view_transform_m01, view_transform_m11, view_transform_m21);
  simd_float4x4 projection_transform = AffineTransformToSimdMatrix(
      projection_transform_m00, projection_transform_m10,
      projection_transform_m20, projection_transform_m01,
      projection_transform_m11, projection_transform_m21);
  metal_renderer.Draw(in_progress_stroke, model_transform, view_transform,
                      projection_transform, render_encoder);
}

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
    float projection_transform_m11, float projection_transform_m21) {
  MetalRenderer& metal_renderer = CastToMetalRenderer(native_ptr);
  const Stroke& stroke = CastToStroke(stroke_native_ptr);
  simd_float4x4 model_transform = AffineTransformToSimdMatrix(
      model_transform_m00, model_transform_m10, model_transform_m20,
      model_transform_m01, model_transform_m11, model_transform_m21);
  simd_float4x4 view_transform = AffineTransformToSimdMatrix(
      view_transform_m00, view_transform_m10, view_transform_m20,
      view_transform_m01, view_transform_m11, view_transform_m21);
  simd_float4x4 projection_transform = AffineTransformToSimdMatrix(
      projection_transform_m00, projection_transform_m10,
      projection_transform_m20, projection_transform_m01,
      projection_transform_m11, projection_transform_m21);
  metal_renderer.Draw(stroke, model_transform, view_transform,
                      projection_transform, render_encoder);
}

void MetalRendererNative_free(int64_t native_ptr) {
  DeleteNativeMetalRenderer(native_ptr);
}

}  // extern "C"
