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

#include "ink/geometry/internal/jni/parallelogram_jni_helper.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/jni/internal/jni_defines.h"

namespace ink {

jobject CreateJImmutableParallelogram(JNIEnv* env, const Quad& quad,
                                      jclass immutable_parallelogram_class,
                                      jclass immutable_vec_class) {
  ABSL_CHECK(immutable_parallelogram_class);

  jmethodID from_center_dim_rot_shear_method = env->GetStaticMethodID(
      immutable_parallelogram_class, "fromCenterDimensionsRotationAndShear",
      "(L" INK_PACKAGE "/geometry/ImmutableVec;FFFF)L" INK_PACKAGE
      "/geometry/ImmutableParallelogram;");
  ABSL_CHECK(from_center_dim_rot_shear_method);
  return env->CallStaticObjectMethod(
      immutable_parallelogram_class, from_center_dim_rot_shear_method,
      ink::CreateJImmutableVecFromPoint(env, {quad.Center().x, quad.Center().y},
                                        immutable_vec_class),
      quad.Width(), quad.Height(), quad.Rotation().ValueInRadians(),
      quad.ShearFactor());
}

void FillJMutableParallelogram(JNIEnv* env, const Quad& quad,
                               jobject mutable_parallelogram) {
  jclass mutable_parallelogram_class =
      env->GetObjectClass(mutable_parallelogram);

  jmethodID setter_method =
      env->GetMethodID(mutable_parallelogram_class,
                       "setCenterDimensionsRotationAndShear", "(FFFFFF)V");
  ABSL_CHECK(setter_method);
  env->CallVoidMethod(mutable_parallelogram, setter_method, quad.Center().x,
                      quad.Center().y, quad.Width(), quad.Height(),
                      quad.Rotation().ValueInRadians(), quad.ShearFactor());
}

}  // namespace ink
