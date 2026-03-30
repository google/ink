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

#ifndef INK_GEOMETRY_INTERNAL_JNI_AFFINE_TRANSFORM_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_AFFINE_TRANSFORM_NATIVE_H_

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
} AffineTransformNative_Vec;

typedef struct {
  AffineTransformNative_Vec center;
  float width;
  float height;
  float rotation_degrees;
  float skew;
} AffineTransformNative_Parallelogram;

AffineTransformNative_Parallelogram AffineTransformNative_apply(
    float a, float b, float c, float d, float e, float f, float quad_center_x,
    float quad_center_y, float quad_width, float quad_height,
    float quad_rotation_degrees, float quad_shear_factor);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_AFFINE_TRANSFORM_NATIVE_H_
