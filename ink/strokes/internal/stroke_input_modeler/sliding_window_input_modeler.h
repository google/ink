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
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

class SlidingWindowInputModeler : public StrokeInputModeler {
 public:
  explicit SlidingWindowInputModeler(Duration32 window_size)
      : half_window_size_(window_size * 0.5) {}

  void StartStroke(float brush_epsilon) override;
  void ExtendStroke(const StrokeInputBatch& real_inputs,
                    const StrokeInputBatch& predicted_inputs,
                    Duration32 current_elapsed_time) override;
  const State& GetState() const override { return state_; }
  absl::Span<const ModeledStrokeInput> GetModeledInputs() const override {
    return modeled_inputs_;
  }

 private:
  // Helper method for `ExtendStroke()`. Erases all unstable inputs from
  // `modeled_inputs_` and `sliding_window_`.
  void EraseUnstableInputs();

  // Helper method for `ExtendStroke()` and `AppendRealInputsToSlidingWindow()`.
  // Appends raw inputs to `sliding_window_`, upsampling as necessary. Also
  // initializes `state_.tool_type` and `state_.stroke_unit_length` as
  // necessary.
  void AppendInputsToSlidingWindow(const StrokeInputBatch& inputs);

  // Helper method for `ExtendStroke()`. Models each unstable input and appends
  // it to `modeled_inputs_`.
  void ModelUnstableInputs();

  // Helper method for `ModelUnstableInputs()`. Models the stroke input at
  // `input_index` within `sliding_window_`, (computing only position and
  // pressure/tilt/orientation for now), and appends the new modeled input to
  // the end of `modeled_inputs_`.
  void ModelUnstableInputPosition(int input_index);

  // Helper method for `ExtendStroke()`. Updates `state_` fields for
  // `total_real` and `complete` distance and time, based on current
  // `modeled_inputs_` and `state_.real_input_count`.
  void UpdateRealAndCompleteDistanceAndTime(Duration32 current_elapsed_time);

  // Helper method for `ExtendStroke()`. Marks stable all real modeled inputs
  // that are at least `half_window_size_` away from the last real input (and
  // which will therefore not change further when further real inputs are added
  // later), updating `state_.stable_input_count` and
  // `stable_sliding_window_input_count_` accordingly.
  void MarkInputsStable();

  // Helper method for `ExtendStroke()`. Removes all predicted stroke inputs
  // from the end of `sliding_window_`, and removes from the start of
  // `sliding_window_` all stable stroke inputs that are no longer needed for
  // modeling the remaining unstable inputs (updating
  // `stable_sliding_window_input_count_` accordingly).
  void TrimSlidingWindow();

  std::vector<ModeledStrokeInput> modeled_inputs_;
  State state_;
  StrokeInputBatch sliding_window_;
  int stable_sliding_window_input_count_ = 0;
  Duration32 half_window_size_;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SLIDING_WINDOW_INPUT_MODELER_H_
