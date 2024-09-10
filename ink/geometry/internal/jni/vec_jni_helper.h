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

#ifndef INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_
#define INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_

#include <jni.h>

#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"

namespace ink {

jobject CreateJImmutableVecFromVec(JNIEnv* env, Vec vec,
                                   jclass immutable_vec_class);

jobject CreateJImmutableVecFromPoint(JNIEnv* env, Point point,
                                     jclass immutable_vec_class);

void FillJMutableVecFromVec(JNIEnv* env, jobject mutable_vec, Vec vec);

void FillJMutableVecFromPoint(JNIEnv* env, jobject mutable_vec, Point point);

}  // namespace ink

#endif  // INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_
