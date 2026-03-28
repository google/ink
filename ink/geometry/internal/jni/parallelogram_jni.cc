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

#include "ink/geometry/internal/jni/box_jni_helper.h"
#include "ink/geometry/internal/jni/parallelogram_native.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_jvm_interface.h"

using ::ink::jni::ClassImmutableVec;
using ::ink::jni::CreateJImmutableBoxOrThrow;
using ::ink::jni::CreateJImmutableVecOrThrow;
using ::ink::jni::FillJMutableBoxOrThrow;
using ::ink::jni::FillJMutableVecOrThrow;

extern "C" {

JNI_METHOD(geometry, ParallelogramNative, jobject, createBoundingBox)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew) {
  return CreateJImmutableBoxOrThrow(
      env, ParallelogramNative_computeBoundingBox(
               center_x, center_y, width, height, rotation_degrees, skew));
}

JNI_METHOD(geometry, ParallelogramNative, void, populateBoundingBox)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject mutable_box) {
  FillJMutableBoxOrThrow(
      env,
      ParallelogramNative_computeBoundingBox(center_x, center_y, width, height,
                                             rotation_degrees, skew),
      mutable_box);
}

JNI_METHOD(geometry, ParallelogramNative, jobjectArray, createSemiAxes)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew,
 jclass immutable_vec_class) {
  ParallelogramNative_SemiAxes axes = ParallelogramNative_computeSemiAxes(
      center_x, center_y, width, height, rotation_degrees, skew);
  jobjectArray vector_array =
      env->NewObjectArray(2, ClassImmutableVec(env), nullptr);
  jobject axis1_vec = CreateJImmutableVecOrThrow(env, axes.first);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 0, axis1_vec);
  jobject axis2_vec = CreateJImmutableVecOrThrow(env, axes.second);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 1, axis2_vec);
  return vector_array;
}

JNI_METHOD(geometry, ParallelogramNative, void, populateSemiAxes)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject out_axis1,
 jobject out_axis2) {
  ParallelogramNative_SemiAxes axes = ParallelogramNative_computeSemiAxes(
      center_x, center_y, width, height, rotation_degrees, skew);
  FillJMutableVecOrThrow(env, axes.first, out_axis1);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, axes.second, out_axis2);
}

JNI_METHOD(geometry, ParallelogramNative, jobjectArray, createCorners)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew) {
  ParallelogramNative_Corners corners = ParallelogramNative_computeCorners(
      center_x, center_y, width, height, rotation_degrees, skew);
  jobjectArray vector_array =
      env->NewObjectArray(4, ClassImmutableVec(env), nullptr);
  jobject corner1 = CreateJImmutableVecOrThrow(env, corners.corner1);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 0, corner1);
  jobject corner2 = CreateJImmutableVecOrThrow(env, corners.corner2);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 1, corner2);
  jobject corner3 = CreateJImmutableVecOrThrow(env, corners.corner3);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 2, corner3);
  jobject corner4 = CreateJImmutableVecOrThrow(env, corners.corner4);
  if (env->ExceptionCheck()) return nullptr;
  env->SetObjectArrayElement(vector_array, 3, corner4);
  return vector_array;
}

JNI_METHOD(geometry, ParallelogramNative, void, populateCorners)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject out_corner1,
 jobject out_corner2, jobject out_corner3, jobject out_corner4) {
  ParallelogramNative_Corners corners = ParallelogramNative_computeCorners(
      center_x, center_y, width, height, rotation_degrees, skew);
  FillJMutableVecOrThrow(env, corners.corner1, out_corner1);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners.corner2, out_corner2);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners.corner3, out_corner3);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners.corner4, out_corner4);
}

JNI_METHOD(geometry, ParallelogramNative, jboolean, contains)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jfloat point_x,
 jfloat point_y) {
  return ParallelogramNative_contains(center_x, center_y, width, height,
                                      rotation_degrees, skew, point_x, point_y);
}

}  // extern "C"
