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

#include <jni.h>

#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/internal/jni/vec_native.h"
#include "ink/jni/internal/jni_defines.h"

using ::ink::jni::CreateJImmutableVecOrThrow;
using ::ink::jni::FillJMutableVecOrThrow;

extern "C" {

JNI_METHOD(geometry, VecNative, jobject, unitVec)
(JNIEnv* env, jobject object, jfloat vec_X, jfloat vec_Y) {
  VecNative_Vec unit_vec = VecNative_unitVec(vec_X, vec_Y);
  return CreateJImmutableVecOrThrow(env, unit_vec);
}

JNI_METHOD(geometry, VecNative, void, populateUnitVec)
(JNIEnv* env, jobject object, jfloat vec_X, jfloat vec_Y,
 jobject output_mutable_vec) {
  FillJMutableVecOrThrow(env, VecNative_unitVec(vec_X, vec_Y),
                         output_mutable_vec);
}

JNI_METHOD(geometry, VecNative, jfloat, absoluteAngleBetweenInDegrees)
(JNIEnv* env, jobject object, jfloat first_vec_X, jfloat first_vec_Y,
 jfloat second_vec_X, jfloat second_vec_Y) {
  return VecNative_absoluteAngleBetweenInDegrees(first_vec_X, first_vec_Y,
                                                 second_vec_X, second_vec_Y);
}

JNI_METHOD(geometry, VecNative, jfloat, signedAngleBetweenInDegrees)
(JNIEnv* env, jobject object, jfloat first_vec_X, jfloat first_vec_Y,
 jfloat second_vec_X, jfloat second_vec_Y) {
  return VecNative_signedAngleBetweenInDegrees(first_vec_X, first_vec_Y,
                                               second_vec_X, second_vec_Y);
}

}  // extern "C"
