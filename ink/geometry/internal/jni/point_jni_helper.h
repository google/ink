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

#ifndef INK_GEOMETRY_INTERNAL_JNI_POINT_JNI_HELPER_H_
#define INK_GEOMETRY_INTERNAL_JNI_POINT_JNI_HELPER_H_

#include <jni.h>

#include "ink/geometry/point.h"

namespace ink {

void FillJMutablePoint(JNIEnv* env, jobject mutable_point, Point point);

}  // namespace ink

#endif  // INK_GEOMETRY_INTERNAL_JNI_POINT_JNI_HELPER_H_
