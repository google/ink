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

#ifndef INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_NAIVE_INPUT_MODELER_H_
#define INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_NAIVE_INPUT_MODELER_H_

#include <vector>

#include "absl/types/span.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler/input_model_impl.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

// A naive model that passes through raw inputs mostly unchanged, with no
// smoothing or upsampling. Velocity and acceleration for modeled inputs are
// calculated in a very simple way from the adjacent input points.
class NaiveInputModeler : public InputModelImpl {
 public:
  NaiveInputModeler() = default;

  void ExtendStroke(InputModelerState& state,
                    std::vector<ModeledStrokeInput>& modeled_inputs,
                    const StrokeInputBatch& real_inputs,
                    const StrokeInputBatch& predicted_inputs) override;

 private:
  void AppendInputs(InputModelerState& state,
                    std::vector<ModeledStrokeInput>& modeled_inputs,
                    const StrokeInputBatch& inputs);
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_NAIVE_INPUT_MODELER_H_
