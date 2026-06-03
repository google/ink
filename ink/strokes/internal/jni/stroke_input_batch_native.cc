// Copyright 2026 Google LLC
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

#include "ink/strokes/internal/jni/stroke_input_batch_native.h"

#include <cstdint>
#include <optional>

#include "absl/status/status.h"
#include "ink/geometry/angle.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native_helper.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

using ::ink::Angle;
using ::ink::Duration32;
using ::ink::PhysicalDistance;
using ::ink::StrokeInput;
using ::ink::native::CastToMutableStrokeInputBatch;
using ::ink::native::CastToStrokeInputBatch;
using ::ink::native::DeleteNativeStrokeInputBatch;
using ::ink::native::NewNativeStrokeInputBatch;

extern "C" {

int64_t StrokeInputBatchNative_create(void) {
  return NewNativeStrokeInputBatch();
}

void StrokeInputBatchNative_free(int64_t native_pointer) {
  DeleteNativeStrokeInputBatch(native_pointer);
}

int StrokeInputBatchNative_getSize(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).Size();
}

StrokeInputBatchNative_Input StrokeInputBatchNative_getStrokeInput(
    int64_t native_pointer, int index) {
  StrokeInput input = CastToStrokeInputBatch(native_pointer).Get(index);
  return {.tool_type_int = static_cast<int>(input.tool_type),
          .x = input.position.x,
          .y = input.position.y,
          .elapsed_time_millis =
              static_cast<int64_t>(input.elapsed_time.ToMillis()),
          .stroke_unit_length_cm = input.stroke_unit_length.ToCentimeters(),
          .pressure = input.pressure,
          .tilt_radians = input.tilt.ValueInRadians(),
          .orientation_radians = input.orientation.ValueInRadians()};
}

int64_t StrokeInputBatchNative_getDurationMillis(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).GetDuration().ToMillis();
}

int StrokeInputBatchNative_getToolType(int64_t native_pointer) {
  return static_cast<int>(CastToStrokeInputBatch(native_pointer).GetToolType());
}

float StrokeInputBatchNative_getStrokeUnitLengthCm(int64_t native_pointer) {
  std::optional<PhysicalDistance> stroke_unit_length =
      CastToStrokeInputBatch(native_pointer).GetStrokeUnitLength();
  if (!stroke_unit_length.has_value()) return 0;
  return stroke_unit_length->ToCentimeters();
}

bool StrokeInputBatchNative_hasStrokeUnitLength(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).HasStrokeUnitLength();
}

bool StrokeInputBatchNative_hasPressure(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).HasPressure();
}

bool StrokeInputBatchNative_hasTilt(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).HasTilt();
}

bool StrokeInputBatchNative_hasOrientation(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).HasOrientation();
}

int StrokeInputBatchNative_getNoiseSeed(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).GetNoiseSeed();
}

float StrokeInputBatchNative_getBaseAnimationPhase(int64_t native_pointer) {
  return CastToStrokeInputBatch(native_pointer).GetBaseAnimationPhase();
}

void MutableStrokeInputBatchNative_clear(int64_t native_pointer) {
  CastToMutableStrokeInputBatch(native_pointer).Clear();
}

bool MutableStrokeInputBatchNative_appendSingle(
    void* jni_env_pass_through, int64_t native_pointer, int tool_type, float x,
    float y, int64_t elapsed_time_millis, float stroke_unit_length_cm,
    float pressure, float tilt, float orientation,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  StrokeInput input = {
      .tool_type = static_cast<StrokeInput::ToolType>(tool_type),
      .position = {x, y},
      .elapsed_time = Duration32::Millis(elapsed_time_millis),
      .stroke_unit_length =
          PhysicalDistance::Centimeters(stroke_unit_length_cm),
      .pressure = pressure,
      .tilt = Angle::Radians(tilt),
      .orientation = Angle::Radians(orientation)};

  if (absl::Status status =
          CastToMutableStrokeInputBatch(native_pointer).Append(input);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return false;
  }
  return true;
}

bool MutableStrokeInputBatchNative_appendBatch(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t append_from_native_pointer,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  if (absl::Status status =
          CastToMutableStrokeInputBatch(native_pointer)
              .Append(CastToStrokeInputBatch(append_from_native_pointer));
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return false;
  }
  return true;
}

int64_t MutableStrokeInputBatchNative_newCopy(int64_t native_pointer) {
  return NewNativeStrokeInputBatch(CastToStrokeInputBatch(native_pointer));
}

void MutableStrokeInputBatchNative_setNoiseSeed(int64_t native_pointer,
                                                int seed) {
  CastToMutableStrokeInputBatch(native_pointer).SetNoiseSeed(seed);
}

void MutableStrokeInputBatchNative_setBaseAnimationPhase(int64_t native_pointer,
                                                         float phase) {
  CastToMutableStrokeInputBatch(native_pointer).SetBaseAnimationPhase(phase);
}

}  // extern "C"
