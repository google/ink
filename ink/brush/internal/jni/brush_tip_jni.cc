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

#include <cstdint>

#include "ink/brush/internal/jni/brush_tip_native.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"

namespace {
using ::ink::jni::ThrowExceptionFromStatusCallback;
}  // namespace

extern "C" {

JNI_METHOD(brush, BrushTipNative, jlong, create)
(JNIEnv* env, jobject thiz, jfloat scale_x, jfloat scale_y,
 jfloat corner_rounding, jfloat slant_degrees, jfloat pinch,
 jfloat rotation_degrees, jfloat particle_gap_distance_scale,
 jlong particle_gap_duration_millis,
 jlongArray behavior_native_pointers_array) {
  const jsize num_behaviors =
      env->GetArrayLength(behavior_native_pointers_array);
  jlong* behavior_pointers =
      env->GetLongArrayElements(behavior_native_pointers_array, nullptr);
  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  int64_t result = BrushTipNative_create(
      env, scale_x, scale_y, corner_rounding, slant_degrees, pinch,
      rotation_degrees, particle_gap_distance_scale,
      particle_gap_duration_millis,
      reinterpret_cast<int64_t*>(behavior_pointers), num_behaviors,
      &ThrowExceptionFromStatusCallback);
  env->ReleaseLongArrayElements(behavior_native_pointers_array,
                                behavior_pointers, JNI_ABORT);
  return result;
}

JNI_METHOD(brush, BrushTipNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  BrushTipNative_free(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getScaleX)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getScaleX(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getScaleY)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getScaleY(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getCornerRounding)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getCornerRounding(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getSlantDegrees)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getSlantDegrees(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getPinch)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getPinch(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getRotationDegrees)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getRotationDegrees(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jfloat, getParticleGapDistanceScale)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getParticleGapDistanceScale(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jlong, getParticleGapDurationMillis)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getParticleGapDurationMillis(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jint, getBehaviorCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushTipNative_getBehaviorCount(native_pointer);
}

JNI_METHOD(brush, BrushTipNative, jlong, newCopyOfBrushBehavior)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushTipNative_newCopyOfBrushBehavior(native_pointer, index);
}

}  // extern "C"
