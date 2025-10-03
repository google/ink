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

#ifndef INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_H_
#define INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_H_

#include <memory>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/types/span.h"
#include "ink/brush/brush_family.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler/input_model_impl.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

// A `StrokeInputModeler` handles modeling (smoothing, de-noising, upsampling,
// etc.)  raw stroke inputs, both "real" and "predicted", to produce a sequence
// of `ModeledStrokeInput`s that can be fed into Ink's stroke extrusion engine.
//
// This type requires that the incremental `StrokeInputBatch` objects are
// already checked to form a valid input sequence before being passed to
// `ExtendStroke()`.
//
// Each call to `ExtendStroke()` generates some number of `ModeledStrokeInput`s
// that are categorized as either "stable" or "unstable". Stable modeled inputs
// are not modified for the rest of the stroke, whereas unstable modeled inputs
// are usually replaced by the next call to `ExtendStroke()`. All stable modeled
// inputs come as a result of modeling "real" (as opposed to predicted) raw
// inputs; however, not every real modeled input will necessarily be stable
// (because the modeler may e.g. take future inputs into account as part of a
// sliding window).
class StrokeInputModeler {
 public:
  StrokeInputModeler() = default;
  StrokeInputModeler(const StrokeInputModeler&) = delete;
  StrokeInputModeler& operator=(const StrokeInputModeler&) = delete;
  StrokeInputModeler(StrokeInputModeler&&) = default;
  StrokeInputModeler& operator=(StrokeInputModeler&&) = default;
  ~StrokeInputModeler() = default;

  // Clears any ongoing stroke and sets up the modeler to accept new stroke
  // input.
  //
  // The value of `brush_epsilon` is CHECK-validated to be greater than zero and
  // is used as the minimum distance between resulting `ModeledStrokeInput`.
  // This function must be called before starting to call `ExtendStroke()`.
  void StartStroke(const BrushFamily::InputModel& input_model,
                   float brush_epsilon);

  // Models new real and predicted inputs and adds them to the current stroke.
  //
  // The `current_elapsed_time` should be the duration from the start of the
  // stroke until "now". The `elapsed_time` of inputs may be "in the future"
  // relative to this duration.
  //
  // This always clears any previously generated unstable modeled inputs. Either
  // or both of `real_inputs` and `predicted_inputs` may be empty. CHECK-fails
  // if `StartStroke()` has not been called at least once.
  void ExtendStroke(const StrokeInputBatch& real_inputs,
                    const StrokeInputBatch& predicted_inputs,
                    Duration32 current_elapsed_time);

  // Returns the current modeling state so far for the current stroke.
  const InputModelerState& GetState() const { return state_; }

  // Returns all currently-modeled inputs based on the raw inputs so far.  The
  // first `GetState().stable_input_count` elements of this list are "stable"
  // and will not be changed by future calls to `ExtendStroke()`.
  absl::Span<const ModeledStrokeInput> GetModeledInputs() const {
    return modeled_inputs_;
  }

 private:
  // Helper method for `ExtendStroke()`. Erases all predicted inputs from
  // `modeled_inputs_`, and updates `state_` accordingly.
  void ErasePredictedModeledInputs();

  // Helper method for `ExtendStroke()`. Sets `state_.tool_type` and
  // `state_.stroke_unit_length` from the inputs (if they are non-empty).
  void SetToolTypeAndStrokeUnitLength(const StrokeInputBatch& real_inputs,
                                      const StrokeInputBatch& predicted_inputs);

  InputModelerState state_;
  std::vector<ModeledStrokeInput> modeled_inputs_;
  absl_nullable std::unique_ptr<InputModelImpl> input_model_impl_;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_H_
