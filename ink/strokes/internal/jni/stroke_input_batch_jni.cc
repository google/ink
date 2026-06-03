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

#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native.h"
#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"

using ::ink::jni::ThrowExceptionFromStatusCallback;
using ::ink::jni::UpdateJStrokeInputOrThrow;

extern "C" {

// ******** Native Implementation of Immutable/Mutable StrokeInputBatch ********
JNI_METHOD(strokes, StrokeInputBatchNative, jlong, create)
(JNIEnv* env, jobject thiz) { return StrokeInputBatchNative_create(); }

JNI_METHOD(strokes, StrokeInputBatchNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  StrokeInputBatchNative_free(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jint, getSize)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getSize(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, void, populate)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index, jobject j_input) {
  UpdateJStrokeInputOrThrow(
      env, StrokeInputBatchNative_getStrokeInput(native_pointer, index),
      j_input);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jlong, getDurationMillis)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getDurationMillis(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jint, getToolType)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getToolType(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jfloat, getStrokeUnitLengthCm)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getStrokeUnitLengthCm(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jboolean, hasStrokeUnitLength)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_hasStrokeUnitLength(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jboolean, hasPressure)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_hasPressure(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jboolean, hasTilt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_hasTilt(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jboolean, hasOrientation)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_hasOrientation(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jint, getNoiseSeed)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getNoiseSeed(native_pointer);
}

JNI_METHOD(strokes, StrokeInputBatchNative, jfloat, getBaseAnimationPhase)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return StrokeInputBatchNative_getBaseAnimationPhase(native_pointer);
}

// ************ Native Implementation of MutableStrokeInputBatch ************
JNI_METHOD(strokes, MutableStrokeInputBatchNative, jboolean, appendSingle)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint tool_type, jfloat x,
 jfloat y, jlong elapsed_time_millis, jfloat stroke_unit_length_cm,
 jfloat pressure, jfloat tilt, jfloat orientation) {
  return MutableStrokeInputBatchNative_appendSingle(
      env, native_pointer, tool_type, x, y, elapsed_time_millis,
      stroke_unit_length_cm, pressure, tilt, orientation,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(strokes, MutableStrokeInputBatchNative, jboolean, appendBatch)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong append_from_native_pointer) {
  return MutableStrokeInputBatchNative_appendBatch(
      env, native_pointer, append_from_native_pointer,
      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(strokes, MutableStrokeInputBatchNative, void, clear)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  MutableStrokeInputBatchNative_clear(native_pointer);
}

JNI_METHOD(strokes, MutableStrokeInputBatchNative, jlong, newCopy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return MutableStrokeInputBatchNative_newCopy(native_pointer);
}

JNI_METHOD(strokes, MutableStrokeInputBatchNative, void, setNoiseSeed)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint seed) {
  MutableStrokeInputBatchNative_setNoiseSeed(native_pointer, seed);
}

JNI_METHOD(strokes, MutableStrokeInputBatchNative, void, setBaseAnimationPhase)
(JNIEnv* env, jobject thiz, jlong native_pointer, jfloat phase) {
  MutableStrokeInputBatchNative_setBaseAnimationPhase(native_pointer, phase);
}

}  // extern "C"
