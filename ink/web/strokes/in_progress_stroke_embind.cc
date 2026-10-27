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

#include <emscripten/bind.h>

#include "absl/log/absl_check.h"
#include "ink/brush/brush.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/stroke.h"
#include "ink/types/duration.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_in_progress_stroke_bindings) {
  emscripten::class_<InProgressStroke>("InProgressStroke")
      .constructor<>()
      .function(
          "start",
          +[](InProgressStroke& stroke, const Brush& brush) {
            stroke.Start(brush, /*stroke_seed=*/0);
          })
      .function(
          "enqueueInputs",
          +[](InProgressStroke& stroke, const StrokeInputBatch& real_inputs,
              const StrokeInputBatch& predicted_inputs) {
            ABSL_CHECK_OK(stroke.EnqueueInputs(real_inputs, predicted_inputs));
          })
      .function("finishInputs", &InProgressStroke::FinishInputs)
      .function(
          "updateShape",
          +[](InProgressStroke& stroke, Duration32 current_elapsed_time) {
            ABSL_CHECK_OK(stroke.UpdateShape(current_elapsed_time));
          })
      .function(
          "copyToStroke",
          +[](const InProgressStroke& stroke) { return stroke.CopyToStroke(); })
      .function("clear", &InProgressStroke::Clear)
      .function(
          "brushCoatCount",
          +[](const InProgressStroke& stroke) {
            return static_cast<int>(stroke.BrushCoatCount());
          })
      .function(
          "inputCount",
          +[](const InProgressStroke& stroke) {
            return static_cast<int>(stroke.InputCount());
          })
      .function(
          "realInputCount",
          +[](const InProgressStroke& stroke) {
            return static_cast<int>(stroke.RealInputCount());
          })
      .function(
          "predictedInputCount",
          +[](const InProgressStroke& stroke) {
            return static_cast<int>(stroke.PredictedInputCount());
          })
      .function(
          "meshVertexCount",
          +[](const InProgressStroke& stroke, int coat_index) {
            return static_cast<int>(stroke.GetMesh(coat_index).VertexCount());
          })
      .function(
          "meshTriangleCount",
          +[](const InProgressStroke& stroke, int coat_index) {
            return static_cast<int>(stroke.GetMesh(coat_index).TriangleCount());
          });
}

}  // namespace
}  // namespace ink::web
