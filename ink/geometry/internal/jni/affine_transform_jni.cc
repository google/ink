// Copyright 2024 Google LLC
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

#include <jni.h>

#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/internal/jni/parallelogram_jni_helper.h"
#include "ink/geometry/quad.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::AffineTransform;
using ::ink::Angle;
using ::ink::Quad;

}  // namespace

extern "C" {

JNI_METHOD(geometry_internal, AffineTransformNative, jobject,
           nativeCreateFromApplyParallelogram)
(JNIEnv* env, jclass clazz, jfloat affine_transform_A,
 jfloat affine_transform_B, jfloat affine_transform_C,
 jfloat affine_transform_D, jfloat affine_transform_E,
 jfloat affine_transform_F, jfloat quad_center_x, jfloat quad_center_y,
 jfloat quad_width, jfloat quad_height, jfloat quad_rotation,
 jfloat quad_shear_factor, jclass immutable_parallelogram_class,
 jclass immutable_vec_class) {
  return ink::CreateJImmutableParallelogram(
      env,
      AffineTransform(affine_transform_A, affine_transform_B,
                      affine_transform_C, affine_transform_D,
                      affine_transform_E, affine_transform_F)
          .Apply(Quad::FromCenterDimensionsRotationAndShear(
              {.x = quad_center_x, .y = quad_center_y}, quad_width, quad_height,
              Angle::Radians(quad_rotation), quad_shear_factor)),
      immutable_parallelogram_class, immutable_vec_class);
}

JNI_METHOD(geometry_internal, AffineTransformNative, void,
           nativePopulateFromApplyParallelogram)
(JNIEnv* env, jclass clazz, jfloat affine_transform_A,
 jfloat affine_transform_B, jfloat affine_transform_C,
 jfloat affine_transform_D, jfloat affine_transform_E,
 jfloat affine_transform_F, jfloat quad_center_x, jfloat quad_center_y,
 jfloat quad_width, jfloat quad_height, jfloat quad_rotation,
 jfloat quad_shear_factor, jobject mutable_quad) {
  ink::FillJMutableParallelogram(
      env,
      AffineTransform(affine_transform_A, affine_transform_B,
                      affine_transform_C, affine_transform_D,
                      affine_transform_E, affine_transform_F)
          .Apply(Quad::FromCenterDimensionsRotationAndShear(
              {.x = quad_center_x, .y = quad_center_y}, quad_width, quad_height,
              Angle::Radians(quad_rotation), quad_shear_factor)),
      mutable_quad);
}

}  // extern "C
