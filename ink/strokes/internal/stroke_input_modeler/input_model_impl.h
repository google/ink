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

#ifndef INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_INPUT_MODEL_IMPL_H_
#define INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_INPUT_MODEL_IMPL_H_

#include <vector>

#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"

namespace ink::strokes_internal {

// Abstract base class for an implementation of a `BrushFamily::InputModel`
// specification.
class InputModelImpl {
 public:
  virtual ~InputModelImpl() = default;

  // Models new real and predicted inputs and adds them to the current stroke by
  // modifying `state` and `modeled_inputs` (as well as private fields of this
  // object). This is used by `StrokeInputModeler::ExtendStroke()` to implement
  // that method; see its doc comment for more details.
  //
  // When this is called, `modeled_inputs` will already have been trimmed down
  // to just its real inputs, and `state.complete_elapsed_time` and
  // `state.complete_traveled_distance` will have been updated accordingly. This
  // method is responsible for updating any previously-unstable real modeled
  // inputs, modeling the new `real_inputs` and `predicted_inputs`, and updating
  // `state` accordingly.
  virtual void ExtendStroke(InputModelerState& state,
                            std::vector<ModeledStrokeInput>& modeled_inputs,
                            const StrokeInputBatch& real_inputs,
                            const StrokeInputBatch& predicted_inputs) = 0;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_INPUT_MODEL_IMPL_H_
