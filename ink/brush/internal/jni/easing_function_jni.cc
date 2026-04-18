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

#include "absl/log/absl_check.h"
#include "ink/brush/internal/jni/easing_function_native.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"

using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, createCopyOf)
(JNIEnv* env, jobject thiz, jlong other_easing_function_native_pointer) {
  return EasingFunctionNative_createCopyOf(
      other_easing_function_native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, createPredefined)
(JNIEnv* env, jobject thiz, jint predefined_response_curve) {
  return EasingFunctionNative_createPredefined(
      env, predefined_response_curve, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, createCubicBezier)
(JNIEnv* env, jobject thiz, jfloat x1, jfloat y1, jfloat x2, jfloat y2) {
  return EasingFunctionNative_createCubicBezier(
      env, x1, y1, x2, y2, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, createSteps)
(JNIEnv* env, jobject thiz, jint step_count, jint step_position) {
  return EasingFunctionNative_createSteps(env, step_count, step_position,
                                          &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, createLinear)
(JNIEnv* env, jobject thiz, jfloatArray points_array) {
  jsize num_points = env->GetArrayLength(points_array) / 2;
  jfloat* points_elements = env->GetFloatArrayElements(points_array, nullptr);
  ABSL_CHECK(points_elements != nullptr);
  jlong result = EasingFunctionNative_createLinear(
      env, points_elements, num_points, &ThrowExceptionFromStatusCallback);
  // Don't need to copy back the array, which is not modified.
  env->ReleaseFloatArrayElements(points_array, points_elements, JNI_ABORT);
  return result;
}

JNI_METHOD(brush_behavior, EasingFunctionNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  EasingFunctionNative_free(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jlong, getParametersType)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getParametersType(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jint, getPredefinedValueInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getPredefinedValueInt(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getCubicBezierX1)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getCubicBezierX1(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getCubicBezierY1)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getCubicBezierY1(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getCubicBezierX2)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getCubicBezierX2(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getCubicBezierY2)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getCubicBezierY2(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jint, getLinearNumPoints)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getLinearNumPoints(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getLinearPointX)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return EasingFunctionNative_getLinearPointX(native_pointer, index);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jfloat, getLinearPointY)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return EasingFunctionNative_getLinearPointY(native_pointer, index);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jint, getStepsCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getStepsCount(native_pointer);
}

JNI_METHOD(brush_behavior, EasingFunctionNative, jint, getStepsPositionInt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return EasingFunctionNative_getStepsPositionInt(native_pointer);
}

}  // extern "C"
