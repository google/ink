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

#include "ink/strokes/internal/jni/stroke_input_jni_helper.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/geometry/angle.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink::jni {

// Convert an int to an StrokeInput::ToolType enum.
//
// This should match the enum in InputToolType.kt.
StrokeInput::ToolType JIntToToolType(jint val) {
  return static_cast<StrokeInput::ToolType>(val);
}

jint ToolTypeToJInt(StrokeInput::ToolType type) {
  switch (type) {
    case StrokeInput::ToolType::kMouse:
      return 1;
    case StrokeInput::ToolType::kTouch:
      return 2;
    case StrokeInput::ToolType::kStylus:
      return 3;
    default:
      return 0;
  }
}

StrokeInput::ToolType JObjectToToolType(JNIEnv* env, jobject j_inputtooltype) {
  jclass inputtooltype_class = env->GetObjectClass(j_inputtooltype);
  ABSL_CHECK(inputtooltype_class) << "InputToolType class not found.";
  jfieldID inputtooltype_value_fieldID =
      env->GetFieldID(inputtooltype_class, "value", "I");
  ABSL_CHECK(inputtooltype_value_fieldID)
      << "Couldn't find InputToolType.value field.";
  jint tooltype_value =
      env->GetIntField(j_inputtooltype, inputtooltype_value_fieldID);
  ABSL_CHECK(!env->ExceptionCheck()) << "InputToolType.value accessor failed.";
  return JIntToToolType(tooltype_value);
}

jobject ToolTypeToJObject(JNIEnv* env, StrokeInput::ToolType tool_type,
                          jclass inputtooltype_class) {
  jmethodID inputtooltype_from_methodID = env->GetStaticMethodID(
      inputtooltype_class, "from", "(I)L" INK_PACKAGE "/brush/InputToolType;");
  ABSL_CHECK(inputtooltype_from_methodID)
      << "InputToolType.from method not found.";
  jobject j_inputtooltype = env->CallStaticObjectMethod(
      inputtooltype_class, inputtooltype_from_methodID,
      ToolTypeToJInt(tool_type));
  ABSL_CHECK(j_inputtooltype) << "InputToolType.from method failed.";
  return j_inputtooltype;
}

void UpdateJObjectInput(JNIEnv* env, const StrokeInput& input_in,
                        jobject j_input_out, jclass inputtooltype_class) {
  // Note: verbose variable naming used to clarify class_fieldname or
  // class_methodname through the multiple stages of references needed.

  jclass strokeinput_class = env->GetObjectClass(j_input_out);
  ABSL_CHECK(strokeinput_class) << "StrokeInput class not found.";

  jobject j_inputtooltype =
      ToolTypeToJObject(env, input_in.tool_type, inputtooltype_class);

  jmethodID strokeinput_update_methodID =
      env->GetMethodID(strokeinput_class, "update",
                       "(FFJL" INK_PACKAGE "/brush/InputToolType;FFFF)V");
  ABSL_CHECK(strokeinput_update_methodID)
      << "StrokeInput.update method not found.";

  jlong elapsed_time_millis = input_in.elapsed_time.ToMillis();
  env->CallVoidMethod(
      j_input_out, strokeinput_update_methodID, input_in.position.x,
      input_in.position.y, elapsed_time_millis, j_inputtooltype,
      input_in.stroke_unit_length.ToCentimeters(), input_in.pressure,
      input_in.tilt.ValueInRadians(), input_in.orientation.ValueInRadians());
  ABSL_CHECK(!env->ExceptionCheck()) << "StrokeInput.update method failed.";
}

StrokeInput JObjectToStrokeInput(JNIEnv* env, jobject j_input) {
  jclass strokeinput_class = env->GetObjectClass(j_input);
  ABSL_CHECK(strokeinput_class) << "StrokeInput class not found.";

  jmethodID strokeinput_tooltype_methodID =
      env->GetMethodID(strokeinput_class, "getToolType",
                       "()L" INK_PACKAGE "/brush/InputToolType;");
  ABSL_CHECK(strokeinput_tooltype_methodID)
      << "StrokeInput.getToolType method not found.";
  jobject j_inputtooltype =
      env->CallObjectMethod(j_input, strokeinput_tooltype_methodID);
  ABSL_CHECK(j_inputtooltype) << "StrokeInput.getToolType method failed.";

  StrokeInput::ToolType tool_type = JObjectToToolType(env, j_inputtooltype);

  jmethodID strokeinput_x_methodID =
      env->GetMethodID(strokeinput_class, "getX", "()F");
  ABSL_CHECK(strokeinput_x_methodID) << "StrokeInput.getX method not found.";
  jfloat j_x = env->CallFloatMethod(j_input, strokeinput_x_methodID);
  ABSL_CHECK(!env->ExceptionCheck()) << "StrokeInput.getX method failed.";

  jmethodID strokeinput_y_methodID =
      env->GetMethodID(strokeinput_class, "getY", "()F");
  ABSL_CHECK(strokeinput_y_methodID) << "StrokeInput.getY method not found.";
  jfloat j_y = env->CallFloatMethod(j_input, strokeinput_y_methodID);
  ABSL_CHECK(!env->ExceptionCheck()) << "StrokeInput.getY method failed.";

  jmethodID strokeinput_elapsedTimeMillis_methodID =
      env->GetMethodID(strokeinput_class, "getElapsedTimeMillis", "()J");
  ABSL_CHECK(strokeinput_elapsedTimeMillis_methodID)
      << "StrokeInput.getElapsedTimeMillis method not found.";

  jlong j_elapsedTimeMillis =
      env->CallLongMethod(j_input, strokeinput_elapsedTimeMillis_methodID);
  ABSL_CHECK(!env->ExceptionCheck())
      << "StrokeInput.getElapsedTimeMillis method failed.";

  jmethodID strokeinput_strokeUnitLengthCm_methodID =
      env->GetMethodID(strokeinput_class, "getStrokeUnitLengthCm", "()F");
  ABSL_CHECK(strokeinput_strokeUnitLengthCm_methodID)
      << "StrokeInput.getStrokeUnitLengthCm method not found.";

  jfloat j_strokeUnitLengthCm =
      env->CallFloatMethod(j_input, strokeinput_strokeUnitLengthCm_methodID);
  ABSL_CHECK(!env->ExceptionCheck())
      << "StrokeInput.getStrokeUnitLengthCm method failed.";

  jmethodID strokeinput_tiltRadians_methodID =
      env->GetMethodID(strokeinput_class, "getTiltRadians", "()F");
  ABSL_CHECK(strokeinput_tiltRadians_methodID)
      << "StrokeInput.getTiltRadians method not found.";

  jfloat j_tiltRadians =
      env->CallFloatMethod(j_input, strokeinput_tiltRadians_methodID);
  ABSL_CHECK(!env->ExceptionCheck())
      << "StrokeInput.getTiltRadians method failed.";

  jmethodID strokeinput_orientationRadians_methodID =
      env->GetMethodID(strokeinput_class, "getOrientationRadians", "()F");
  ABSL_CHECK(strokeinput_orientationRadians_methodID)
      << "StrokeInput.getOrientationRadians method not found.";

  jfloat j_orientationRadians =
      env->CallFloatMethod(j_input, strokeinput_orientationRadians_methodID);
  ABSL_CHECK(!env->ExceptionCheck())
      << "StrokeInput.getOrientationRadians method failed.";

  jmethodID strokeinput_pressure_methodID =
      env->GetMethodID(strokeinput_class, "getPressure", "()F");
  ABSL_CHECK(strokeinput_pressure_methodID)
      << "StrokeInput.getPressure method not found.";

  jfloat j_pressure =
      env->CallFloatMethod(j_input, strokeinput_pressure_methodID);
  ABSL_CHECK(!env->ExceptionCheck())
      << "StrokeInput.getPressure method failed.";

  return StrokeInput{
      .tool_type = tool_type,
      .position = {j_x, j_y},
      .elapsed_time = Duration32::Millis(j_elapsedTimeMillis),
      .stroke_unit_length = PhysicalDistance::Centimeters(j_strokeUnitLengthCm),
      .pressure = j_pressure,
      .tilt = Angle::Radians(j_tiltRadians),
      .orientation = Angle::Radians(j_orientationRadians)};
}

}  // namespace ink::jni
