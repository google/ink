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

#include "ink/geometry/internal/jni/point_jni_helper.h"

#include <jni.h>

#include "absl/log/check.h"
#include "ink/geometry/point.h"

namespace ink {

void FillJMutablePoint(JNIEnv* env, jobject mutable_point, Point point) {
  jclass mutable_point_class = env->GetObjectClass(mutable_point);

  jmethodID set_x_method =
      env->GetMethodID(mutable_point_class, "setX", "(F)V");
  CHECK(set_x_method);
  env->CallVoidMethod(mutable_point, set_x_method, point.x);

  jmethodID set_y_method =
      env->GetMethodID(mutable_point_class, "setY", "(F)V");
  CHECK(set_y_method);
  env->CallVoidMethod(mutable_point, set_y_method, point.y);
}

}  // namespace ink
