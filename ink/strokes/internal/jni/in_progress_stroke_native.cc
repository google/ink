// Copyright 2024-2026 Google LLC
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

#include "ink/strokes/internal/jni/in_progress_stroke_native.h"

#include <cstdint>
#include <optional>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native_helper.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native_helper.h"
#include "ink/strokes/internal/jni/stroke_native_helper.h"
#include "ink/types/duration.h"

using ::ink::Duration32;
using ::ink::InProgressStroke;
using ::ink::Point;
using ::ink::Rect;
using ::ink::StrokeInput;
using ::ink::StrokeInputBatch;
using ::ink::native::CastToBrush;
using ::ink::native::CastToInProgressStrokeWrapper;
using ::ink::native::CastToMutableInProgressStrokeWrapper;
using ::ink::native::CastToMutableStrokeInputBatch;
using ::ink::native::CastToStrokeInputBatch;
using ::ink::native::DeleteNativeInProgressStroke;
using ::ink::native::InProgressStrokeWrapper;
using ::ink::native::NewNativeInProgressStroke;
using ::ink::native::NewNativeMeshFormat;
using ::ink::native::NewNativeStroke;

extern "C" {

// Construct a native InProgressStroke and return a pointer to it as a long.
int64_t InProgressStrokeNative_create() { return NewNativeInProgressStroke(); }

void InProgressStrokeNative_free(int64_t native_pointer) {
  DeleteNativeInProgressStroke(native_pointer);
}

void InProgressStrokeNative_clear(int64_t native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer).Stroke().Clear();
}

// Starts the stroke with a brush.
void InProgressStrokeNative_start(int64_t native_pointer,
                                  int64_t brush_native_pointer, int noise_seed,
                                  float base_animation_phase) {
  CastToMutableInProgressStrokeWrapper(native_pointer)
      .Start(CastToBrush(brush_native_pointer), noise_seed,
             base_animation_phase);
}

bool InProgressStrokeNative_enqueueInputs(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t real_inputs_pointer, int64_t predicted_inputs_pointer,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  InProgressStroke& in_progress_stroke =
      CastToMutableInProgressStrokeWrapper(native_pointer).Stroke();
  if (absl::Status status = in_progress_stroke.EnqueueInputs(
          CastToStrokeInputBatch(real_inputs_pointer),
          CastToStrokeInputBatch(predicted_inputs_pointer));
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return false;
  }
  return true;
}

bool InProgressStrokeNative_updateShape(
    void* jni_env_pass_through_pass_through, int64_t native_pointer,
    int64_t j_current_elapsed_time_millis,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  if (absl::Status status =
          CastToMutableInProgressStrokeWrapper(native_pointer)
              .UpdateShape(Duration32::Millis(j_current_elapsed_time_millis));
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return false;
  }
  return true;
}

void InProgressStrokeNative_finishInput(int64_t native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer).Stroke().FinishInputs();
}

bool InProgressStrokeNative_isInputFinished(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .InputsAreFinished();
}

bool InProgressStrokeNative_isUpdateNeeded(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer).Stroke().NeedsUpdate();
}

bool InProgressStrokeNative_changesWithTime(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .ChangesWithTime();
}

int64_t InProgressStrokeNative_newStrokeFromCopy(int64_t native_pointer) {
  return NewNativeStroke(
      CastToInProgressStrokeWrapper(native_pointer).Stroke().CopyToStroke());
}

int64_t InProgressStrokeNative_newStrokeFromPrunedCopy(int64_t native_pointer) {
  return NewNativeStroke(
      CastToInProgressStrokeWrapper(native_pointer)
          .Stroke()
          .CopyToStroke(InProgressStroke::RetainAttributes::kUsedByThisBrush));
}

int InProgressStrokeNative_getInputCount(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer).Stroke().InputCount();
}

int InProgressStrokeNative_getRealInputCount(int64_t native_pointer) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  return in_progress_stroke.RealInputCount();
}

int InProgressStrokeNative_getPredictedInputCount(int64_t native_pointer) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  return in_progress_stroke.PredictedInputCount();
}

void InProgressStrokeNative_populateInputs(
    int64_t native_pointer, int64_t mutable_stroke_input_batch_pointer,
    int from, int to) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  StrokeInputBatch& batch =
      CastToMutableStrokeInputBatch(mutable_stroke_input_batch_pointer);
  batch.Clear();
  const StrokeInputBatch& inputs = in_progress_stroke.GetInputs();
  for (int i = from; i < to; ++i) {
    // The input here should have already been validated.
    ABSL_CHECK_OK(batch.Append(inputs.Get(i)));
  }
  batch.SetNoiseSeed(inputs.GetNoiseSeed());
  batch.SetBaseAnimationPhase(inputs.GetBaseAnimationPhase());
}

InProgressStrokeNative_Input InProgressStrokeNative_getInput(
    int64_t native_pointer, int index) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  StrokeInput input = in_progress_stroke.GetInputs().Get(index);
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

float InProgressStrokeNative_getBaseAnimationPhase(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .GetInputs()
      .GetBaseAnimationPhase();
}

int InProgressStrokeNative_getBrushCoatCount(int64_t native_pointer) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .BrushCoatCount();
}

bool InProgressStrokeNative_hasMeshBounds(int64_t native_pointer,
                                          int coat_index) {
  return !CastToInProgressStrokeWrapper(native_pointer)
              .Stroke()
              .GetMeshBounds(coat_index)
              .IsEmpty();
}

InProgressStrokeNative_Box InProgressStrokeNative_getMeshBounds(
    int64_t native_pointer, int coat_index) {
  const std::optional<Rect>& mesh_bounds =
      CastToInProgressStrokeWrapper(native_pointer)
          .Stroke()
          .GetMeshBounds(coat_index)
          .AsRect();
  ABSL_CHECK(mesh_bounds.has_value());
  return {.min_x = mesh_bounds->XMin(),
          .min_y = mesh_bounds->YMin(),
          .max_x = mesh_bounds->XMax(),
          .max_y = mesh_bounds->YMax()};
}

bool InProgressStrokeNative_hasUpdatedRegion(int64_t native_pointer) {
  return !CastToInProgressStrokeWrapper(native_pointer)
              .Stroke()
              .GetUpdatedRegion()
              .IsEmpty();
}

InProgressStrokeNative_Box InProgressStrokeNative_getUpdatedRegion(
    int64_t native_pointer) {
  const std::optional<Rect>& updated_region =
      CastToInProgressStrokeWrapper(native_pointer)
          .Stroke()
          .GetUpdatedRegion()
          .AsRect();
  ABSL_CHECK(updated_region.has_value());
  return {.min_x = updated_region->XMin(),
          .min_y = updated_region->YMin(),
          .max_x = updated_region->XMax(),
          .max_y = updated_region->YMax()};
}

void InProgressStrokeNative_resetUpdatedRegion(int64_t native_pointer) {
  CastToMutableInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .ResetUpdatedRegion();
}

int InProgressStrokeNative_getOutlineCount(int64_t native_pointer,
                                           int coat_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .GetCoatOutlines(coat_index)
      .size();
}

int InProgressStrokeNative_getOutlineVertexCount(int64_t native_pointer,
                                                 int coat_index,
                                                 int outline_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .Stroke()
      .GetCoatOutlines(coat_index)[outline_index]
      .size();
}

InProgressStrokeNative_Vec InProgressStrokeNative_getOutlinePosition(
    int64_t native_pointer, int coat_index, int outline_index,
    int outline_vertex_index) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  absl::Span<const uint32_t> outline =
      in_progress_stroke.GetCoatOutlines(coat_index)[outline_index];
  const Point& position = in_progress_stroke.GetMesh(coat_index)
                              .VertexPosition(outline[outline_vertex_index]);
  return {.x = position.x, .y = position.y};
}

InProgressStrokeNative_Vec InProgressStrokeNative_getPosition(
    int64_t native_pointer, int coat_index, int partition_index,
    int vertex_index) {
  const InProgressStroke& in_progress_stroke =
      CastToInProgressStrokeWrapper(native_pointer).Stroke();
  const Point& position =
      in_progress_stroke.GetMesh(coat_index).VertexPosition(vertex_index);
  return {.x = position.x, .y = position.y};
}

int InProgressStrokeNative_getMeshPartitionCount(int64_t native_pointer,
                                                 int coat_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .MeshPartitionCount(coat_index);
}

int InProgressStrokeNative_getVertexCount(int64_t native_pointer,
                                          int coat_index, int mesh_index) {
  return CastToInProgressStrokeWrapper(native_pointer)
      .VertexCount(coat_index, mesh_index);
}

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
int64_t InProgressStrokeNative_newCopyOfMeshFormat(int64_t native_pointer,
                                                   int coat_index) {
  const InProgressStrokeWrapper& in_progress_stroke_wrapper =
      CastToInProgressStrokeWrapper(native_pointer);
  return NewNativeMeshFormat(
      in_progress_stroke_wrapper.Stroke().GetMeshFormat(coat_index));
}

}  // extern "C"
