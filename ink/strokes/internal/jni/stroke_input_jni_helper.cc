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

#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"

#include <jni.h>

#include "ink/jni/internal/jni_jvm_interface.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native.h"

namespace ink::jni {

namespace {

void UpdateJStrokeInputOrThrow(JNIEnv* env, jfloat x, jfloat y,
                               jlong elapsed_time_millis, jint tool_type_int,
                               jfloat stroke_unit_length_cm, jfloat pressure,
                               jfloat tilt_radians, jfloat orientation_radians,
                               jobject j_input_out) {
  env->CallVoidMethod(j_input_out, MethodStrokeInputUpdate(env), x, y,
                      elapsed_time_millis, tool_type_int, stroke_unit_length_cm,
                      pressure, tilt_radians, orientation_radians);
}

}  // namespace

void UpdateJStrokeInputOrThrow(JNIEnv* env,
                               const StrokeInputBatchNative_Input& input_in,
                               jobject j_input_out) {
  UpdateJStrokeInputOrThrow(
      env, input_in.x, input_in.y, input_in.elapsed_time_millis,
      input_in.tool_type_int, input_in.stroke_unit_length_cm, input_in.pressure,
      input_in.tilt_radians, input_in.orientation_radians, j_input_out);
}

void UpdateJStrokeInputOrThrow(JNIEnv* env,
                               const InProgressStrokeNative_Input& input_in,
                               jobject j_input_out) {
  UpdateJStrokeInputOrThrow(
      env, input_in.x, input_in.y, input_in.elapsed_time_millis,
      input_in.tool_type_int, input_in.stroke_unit_length_cm, input_in.pressure,
      input_in.tilt_radians, input_in.orientation_radians, j_input_out);
}

}  // namespace ink::jni
