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
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native.h"
#include "ink/strokes/internal/jni/stroke_input_native_helper.h"

namespace ink::jni {

using ::ink::native::ToolTypeToInt;

void UpdateJStrokeInputOrThrow(JNIEnv* env, const StrokeInput& input_in,
                               jobject j_input_out) {
  env->CallVoidMethod(
      j_input_out, MethodStrokeInputUpdate(env), input_in.position.x,
      input_in.position.y, static_cast<jlong>(input_in.elapsed_time.ToMillis()),
      ToolTypeToInt(input_in.tool_type),
      input_in.stroke_unit_length.ToCentimeters(), input_in.pressure,
      input_in.tilt.ValueInRadians(), input_in.orientation.ValueInRadians());
}

void UpdateJStrokeInputOrThrow(JNIEnv* env,
                               const StrokeInputBatchNative_Input& input_in,
                               jobject j_input_out) {
  env->CallVoidMethod(j_input_out, MethodStrokeInputUpdate(env), input_in.x,
                      input_in.y, input_in.elapsed_time_millis,
                      input_in.tool_type_int, input_in.stroke_unit_length_cm,
                      input_in.pressure, input_in.tilt_radians,
                      input_in.orientation_radians);
}

}  // namespace ink::jni
