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

#include <array>
#include <utility>

#include "ink/geometry/angle.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/jni/box_jni_helper.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/vec.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_jvm_interface.h"

using ::ink::Angle;
using ::ink::Envelope;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Vec;
using ::ink::jni::ClassImmutableVec;
using ::ink::jni::CreateJImmutableBoxOrThrow;
using ::ink::jni::CreateJImmutableVecOrThrow;
using ::ink::jni::FillJMutableBoxOrThrow;
using ::ink::jni::FillJMutableVecOrThrow;

extern "C" {

JNI_METHOD(geometry, ParallelogramNative, jobject, createBoundingBox)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  return CreateJImmutableBoxOrThrow(env, Envelope(quad).AsRect().value());
}

JNI_METHOD(geometry, ParallelogramNative, void, populateBoundingBox)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject mutable_box) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  FillJMutableBoxOrThrow(env, Envelope(quad).AsRect().value(), mutable_box);
}

JNI_METHOD(geometry, ParallelogramNative, jobjectArray, createSemiAxes)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew,
 jclass immutable_vec_class) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::pair<Vec, Vec> axes = quad.SemiAxes();
  jobjectArray vector_array =
      env->NewObjectArray(2, ClassImmutableVec(env), nullptr);
  env->SetObjectArrayElement(vector_array, 0,
                             CreateJImmutableVecOrThrow(env, axes.first));
  env->SetObjectArrayElement(vector_array, 1,
                             CreateJImmutableVecOrThrow(env, axes.second));
  return vector_array;
}

JNI_METHOD(geometry, ParallelogramNative, void, populateSemiAxes)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject out_axis1,
 jobject out_axis2) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::pair<Vec, Vec> axes = quad.SemiAxes();
  FillJMutableVecOrThrow(env, axes.first, out_axis1);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, axes.second, out_axis2);
}

JNI_METHOD(geometry, ParallelogramNative, jobjectArray, createCorners)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::array<Point, 4> corners = quad.Corners();
  jobjectArray vector_array =
      env->NewObjectArray(4, ClassImmutableVec(env), nullptr);
  for (int i = 0; i < 4; ++i) {
    jobject corner = CreateJImmutableVecOrThrow(env, corners[i]);
    if (env->ExceptionCheck()) return nullptr;
    env->SetObjectArrayElement(vector_array, i, corner);
  }
  return vector_array;
}

JNI_METHOD(geometry, ParallelogramNative, void, populateCorners)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jobject out_corner1,
 jobject out_corner2, jobject out_corner3, jobject out_corner4) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::array<Point, 4> corners = quad.Corners();
  FillJMutableVecOrThrow(env, corners[0], out_corner1);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners[1], out_corner2);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners[2], out_corner3);
  if (env->ExceptionCheck()) return;
  FillJMutableVecOrThrow(env, corners[3], out_corner4);
}

JNI_METHOD(geometry, ParallelogramNative, jboolean, contains)
(JNIEnv* env, jobject object, jfloat center_x, jfloat center_y, jfloat width,
 jfloat height, jfloat rotation_degrees, jfloat skew, jfloat point_x,
 jfloat point_y) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  return quad.Contains({point_x, point_y});
}

}  // extern "C"
