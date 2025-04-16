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

#include "ink/geometry/internal/jni/vec_jni_helper.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"

namespace ink::jni {

jobject CreateJImmutableVecFromVec(JNIEnv* env, Vec vec,
                                   jclass immutable_vec_class) {
  ABSL_CHECK(immutable_vec_class);
  jmethodID constructor =
      env->GetMethodID(immutable_vec_class, "<init>", "(FF)V");
  ABSL_CHECK(constructor);
  return env->NewObject(immutable_vec_class, constructor, vec.x, vec.y);
}

jobject CreateJImmutableVecFromPoint(JNIEnv* env, Point point,
                                     jclass immutable_vec_class) {
  ABSL_CHECK(immutable_vec_class);
  jmethodID constructor =
      env->GetMethodID(immutable_vec_class, "<init>", "(FF)V");
  ABSL_CHECK(constructor);
  return env->NewObject(immutable_vec_class, constructor, point.x, point.y);
}

void FillJMutableVecFromVec(JNIEnv* env, jobject mutable_vec, Vec vec) {
  jclass mutable_vec_class = env->GetObjectClass(mutable_vec);

  jmethodID set_x_method = env->GetMethodID(mutable_vec_class, "setX", "(F)V");
  ABSL_CHECK(set_x_method);
  env->CallVoidMethod(mutable_vec, set_x_method, vec.x);

  jmethodID set_y_method = env->GetMethodID(mutable_vec_class, "setY", "(F)V");
  ABSL_CHECK(set_y_method);
  env->CallVoidMethod(mutable_vec, set_y_method, vec.y);
}

void FillJMutableVecFromPoint(JNIEnv* env, jobject mutable_vec, Point point) {
  jclass mutable_vec_class = env->GetObjectClass(mutable_vec);

  jmethodID set_x_method = env->GetMethodID(mutable_vec_class, "setX", "(F)V");
  ABSL_CHECK(set_x_method);
  env->CallVoidMethod(mutable_vec, set_x_method, point.x);

  jmethodID set_y_method = env->GetMethodID(mutable_vec_class, "setY", "(F)V");
  ABSL_CHECK(set_y_method);
  env->CallVoidMethod(mutable_vec, set_y_method, point.y);
}

}  // namespace ink::jni
