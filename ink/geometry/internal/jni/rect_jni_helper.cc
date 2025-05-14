
// Copyright 2024-2025 Google LLC
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

#include "ink/geometry/internal/jni/rect_jni_helper.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/rect.h"
#include "ink/jni/internal/jni_defines.h"

namespace ink::jni {

jobject CreateJImmutableBoxFromRect(JNIEnv* env, Rect rect,
                                    jclass immutable_box_class,
                                    jclass immutable_vec_class) {
  ABSL_CHECK(immutable_box_class);
  jmethodID from_two_points_method = env->GetStaticMethodID(
      immutable_box_class, "fromTwoPoints",
      "(L" INK_PACKAGE "/geometry/Vec;L" INK_PACKAGE
      "/geometry/Vec;)L" INK_PACKAGE "/geometry/ImmutableBox;");
  ABSL_CHECK(from_two_points_method);
  return env->CallStaticObjectMethod(
      immutable_box_class, from_two_points_method,
      CreateJImmutableVecFromPoint(env, {rect.XMin(), rect.YMin()},
                                   immutable_vec_class),
      CreateJImmutableVecFromPoint(env, {rect.XMax(), rect.YMax()},
                                   immutable_vec_class));
}

void FillJMutableBoxFromRectOrThrow(JNIEnv* env, jobject mutable_box,
                                    Rect rect) {
  jclass mutable_box_class = env->GetObjectClass(mutable_box);

  jmethodID set_x_bound_method =
      env->GetMethodID(mutable_box_class, "setXBounds",
                       "(FF)L" INK_PACKAGE "/geometry/MutableBox;");
  ABSL_CHECK_NE(set_x_bound_method, nullptr);
  env->CallObjectMethod(mutable_box, set_x_bound_method, rect.XMin(),
                        rect.XMax());
  if (env->ExceptionCheck()) return;

  jmethodID set_y_bound_method =
      env->GetMethodID(mutable_box_class, "setYBounds",
                       "(FF)L" INK_PACKAGE "/geometry/MutableBox;");
  ABSL_CHECK_NE(set_y_bound_method, nullptr);
  env->CallObjectMethod(mutable_box, set_y_bound_method, rect.YMin(),
                        rect.YMax());
}

}  // namespace ink::jni
