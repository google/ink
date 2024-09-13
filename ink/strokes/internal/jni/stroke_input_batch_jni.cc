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

#include <optional>

#include "absl/log/absl_check.h"
#include "ink/geometry/angle.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

#define STROKE_INPUT_BATCH_JNI_METHOD(return_type, method_name) \
  JNI_METHOD(strokes, StrokeInputBatchNative, return_type, method_name)

#define MUTABLE_STROKE_INPUT_BATCH_JNI_METHOD(return_type, method_name) \
  JNI_METHOD(strokes, MutableStrokeInputBatchNative, return_type, method_name)

extern "C" {

// ******** Native Implementation of Immutable/Mutable StrokeInputBatch ********
STROKE_INPUT_BATCH_JNI_METHOD(jlong, createNativePeer)
(JNIEnv* env, jobject thiz) {
  auto* batch = new ink::StrokeInputBatch();
  return reinterpret_cast<jlong>(batch);
}

STROKE_INPUT_BATCH_JNI_METHOD(void, freeNativePeer)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0)
      << "Invalid native pointer for StrokeInputBatch.";
  delete reinterpret_cast<ink::StrokeInputBatch*>(native_pointer);
}

STROKE_INPUT_BATCH_JNI_METHOD(jint, getSize)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.Size();
}

STROKE_INPUT_BATCH_JNI_METHOD(void, populate)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index, jobject j_input,
 jclass input_tool_type_class) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  ink::StrokeInput input = batch.Get(index);
  ink::UpdateJObjectInput(env, input, j_input, input_tool_type_class);
}

STROKE_INPUT_BATCH_JNI_METHOD(jlong, getDurationMillis)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.GetDuration().ToMillis();
}

STROKE_INPUT_BATCH_JNI_METHOD(jint, getToolType)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return ink::ToolTypeToJInt(batch.GetToolType());
}

STROKE_INPUT_BATCH_JNI_METHOD(jfloat, getStrokeUnitLengthCm)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  std::optional<ink::PhysicalDistance> stroke_unit_length =
      batch.GetStrokeUnitLength();
  if (!stroke_unit_length.has_value()) return 0;
  return stroke_unit_length->ToCentimeters();
}

STROKE_INPUT_BATCH_JNI_METHOD(jboolean, hasStrokeUnitLength)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0)
      << "Invalid native pointer for StrokeInputBatch.";
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.HasStrokeUnitLength();
}

STROKE_INPUT_BATCH_JNI_METHOD(jboolean, hasPressure)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.HasPressure();
}

STROKE_INPUT_BATCH_JNI_METHOD(jboolean, hasTilt)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.HasTilt();
}

STROKE_INPUT_BATCH_JNI_METHOD(jboolean, hasOrientation)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const ink::StrokeInputBatch& batch =
      ink::CastToStrokeInputBatch(native_pointer);
  return batch.HasOrientation();
}

// ************ Native Implementation of MutableStrokeInputBatch ************
MUTABLE_STROKE_INPUT_BATCH_JNI_METHOD(jstring, appendSingle)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint tool_type, jfloat x,
 jfloat y, jlong elapsed_time_millis, jfloat stroke_unit_length_cm,
 jfloat pressure, jfloat tilt, jfloat orientation) {
  ink::StrokeInputBatch* batch =
      ink::CastToMutableStrokeInputBatch(native_pointer);

  ink::StrokeInput input = {
      .tool_type = ink::JIntToToolType(tool_type),
      .position = {x, y},
      .elapsed_time = ink::Duration32::Millis(elapsed_time_millis),
      .stroke_unit_length =
          ink::PhysicalDistance::Centimeters(stroke_unit_length_cm),
      .pressure = pressure,
      .tilt = ink::Angle::Radians(tilt),
      .orientation = ink::Angle::Radians(orientation)};
  auto status = batch->Append(input);
  if (!status.ok()) {
    return env->NewStringUTF(status.ToString().c_str());
  }
  return nullptr;
}

MUTABLE_STROKE_INPUT_BATCH_JNI_METHOD(jstring, appendBatch)
(JNIEnv* env, jobject thiz, jlong native_pointer,
 jlong append_from_native_pointer) {
  ink::StrokeInputBatch* batch =
      ink::CastToMutableStrokeInputBatch(native_pointer);
  const ink::StrokeInputBatch& append_from_batch =
      ink::CastToStrokeInputBatch(append_from_native_pointer);

  auto status = batch->Append(append_from_batch);
  if (!status.ok()) {
    return env->NewStringUTF(status.ToString().c_str());
  }
  return nullptr;
}

MUTABLE_STROKE_INPUT_BATCH_JNI_METHOD(void, clear)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  ink::StrokeInputBatch* batch =
      ink::CastToMutableStrokeInputBatch(native_pointer);
  batch->Clear();
}

MUTABLE_STROKE_INPUT_BATCH_JNI_METHOD(jlong, copy)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  ink::StrokeInputBatch* batch =
      ink::CastToMutableStrokeInputBatch(native_pointer);
  return reinterpret_cast<jlong>(new ink::StrokeInputBatch(*batch));
}

}  // extern "C"
