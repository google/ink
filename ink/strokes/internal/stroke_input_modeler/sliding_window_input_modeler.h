// Copyright 2025 Google LLC
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

#ifndef INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SLIDING_WINDOW_INPUT_MODELER_H_
#define INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SLIDING_WINDOW_INPUT_MODELER_H_

#include <vector>

#include "absl/types/span.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler/input_model_impl.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

class SlidingWindowInputModeler : public InputModelImpl {
 public:
  SlidingWindowInputModeler(Duration32 window_size,
                            Duration32 upsampling_period);

  void ExtendStroke(InputModelerState& state,
                    std::vector<ModeledStrokeInput>& modeled_inputs,
                    const StrokeInputBatch& real_inputs,
                    const StrokeInputBatch& predicted_inputs) override;

 private:
  // Helper method for `ExtendStroke()`. Erases all unstable inputs from
  // `modeled_inputs_`.
  void EraseUnstableModeledInputs(
      InputModelerState& state,
      std::vector<ModeledStrokeInput>& modeled_inputs);

  // Helper method for `ExtendStroke()`. Appends the given raw inputs to
  // `sliding_window_`, and initializes `state.tool_type` and
  // `state.stroke_unit_length` as necessary.
  void AppendRawInputsToSlidingWindow(InputModelerState& state,
                                      const StrokeInputBatch& raw_inputs);

  // Helper method for `ExtendStroke()`. Fully models each unstable input and
  // appends it to `modeled_inputs`, updating `state.real_input_count`
  // appropriately.
  void ModelUnstableInputs(InputModelerState& state,
                           std::vector<ModeledStrokeInput>& modeled_inputs,
                           int sliding_window_real_input_count);

  // Helper method for `ModelUnstableInputs()`. Models each unstable input
  // (computing only position and pressure/tilt/orientation for now) and appends
  // it to `modeled_inputs`.  Also updates `state.real_input_cutoff` to the
  // number of modeled inputs with `elapsed_time` no later than
  // `real_input_cutoff`.
  void ModelUnstableInputPositions(
      InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
      Duration32 real_input_cutoff);

  // Helper method for `ModelUnstableInputPositions()`. Appends a new modeled
  // input (computing only position and pressure/tilt/orientation for now) at
  // `elapsed_time`.  When this returns, `start_index` and `end_index` will be
  // the indices into `sliding_window_` of the first raw input before the window
  // and the last raw input after the window; before calling this, `start_index`
  // and `end_index` must be no larger than those indices, as they are only ever
  // marched forward.
  void ModelUnstableInputPosition(
      std::vector<ModeledStrokeInput>& modeled_inputs, Duration32 elapsed_time,
      int& start_index, int& end_index);

  // Helper method for `ExtendStroke()`. Updates `state` fields for `total_real`
  // and `complete` distance and time, based on current `modeled_inputs` and
  // `state.real_input_count`.
  void UpdateRealAndCompleteDistanceAndTime(
      InputModelerState& state,
      std::vector<ModeledStrokeInput>& modeled_inputs);

  // Helper method for `ExtendStroke()`. Marks stable all real modeled inputs
  // that are at least `half_window_size_` before
  // `state.total_real_elapsed_time` (and which will therefore not change
  // further when further real raw inputs are added later).
  void MarkStableModeledInputs(InputModelerState& state,
                               std::vector<ModeledStrokeInput>& modeled_inputs);

  // Helper method for `ExtendStroke()`. Removes all predicted raw stroke inputs
  // from the end of `sliding_window_`, and removes from the start of
  // `sliding_window_` all raw stroke inputs that are no longer needed for
  // remodeling the remaining unstable modeled inputs.
  void TrimSlidingWindow(InputModelerState& state,
                         std::vector<ModeledStrokeInput>& modeled_inputs,
                         int sliding_window_real_input_count);

  StrokeInputBatch sliding_window_;
  Duration32 half_window_size_;
  Duration32 upsampling_period_;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SLIDING_WINDOW_INPUT_MODELER_H_
