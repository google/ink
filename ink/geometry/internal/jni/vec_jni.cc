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

#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/vec.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::Vec;

}  // namespace

extern "C" {

JNI_METHOD(geometry_internal, VecNative, jobject, unitVec)
(JNIEnv* env, jclass clazz, jfloat vec_X, jfloat vec_Y,
 jclass immutable_vec_class) {
  return CreateJImmutableVecFromVec(env, Vec{vec_X, vec_Y}.AsUnitVec(),
                                    immutable_vec_class);
}

JNI_METHOD(geometry_internal, VecNative, void, populateUnitVec)
(JNIEnv* env, jclass clazz, jfloat vec_X, jfloat vec_Y,
 jobject output_mutable_vec) {
  FillJMutableVecFromVec(env, output_mutable_vec,
                         Vec{vec_X, vec_Y}.AsUnitVec());
}

JNI_METHOD(geometry_internal, VecNative, jfloat, absoluteAngleBetween)
(JNIEnv* env, jclass clazz, jfloat first_vec_X, jfloat first_vec_Y,
 jfloat second_vec_X, jfloat second_vec_Y) {
  return Vec::AbsoluteAngleBetween(Vec{first_vec_X, first_vec_Y},
                                   Vec{second_vec_X, second_vec_Y})
      .ValueInRadians();
}

JNI_METHOD(geometry_internal, VecNative, jfloat, signedAngleBetween)
(JNIEnv* env, jclass clazz, jfloat first_vec_X, jfloat first_vec_Y,
 jfloat second_vec_X, jfloat second_vec_Y) {
  return Vec::SignedAngleBetween(Vec{first_vec_X, first_vec_Y},
                                 Vec{second_vec_X, second_vec_Y})
      .ValueInRadians();
}

}  // extern "C
