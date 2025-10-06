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

#ifndef INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SPRING_BASED_INPUT_MODELER_H_
#define INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SPRING_BASED_INPUT_MODELER_H_

#include <optional>
#include <vector>

#include "absl/types/span.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler/input_model_impl.h"
#include "ink/types/duration.h"
#include "ink_stroke_modeler/stroke_modeler.h"
#include "ink_stroke_modeler/types.h"

namespace ink::strokes_internal {

class SpringBasedInputModeler : public InputModelImpl {
 public:
  enum class Version {
    kSpringModel,
    kExperimentalRawPositionModel,
  };

  SpringBasedInputModeler(Version version, float brush_epsilon);

  void ExtendStroke(InputModelerState& state,
                    std::vector<ModeledStrokeInput>& modeled_inputs,
                    const StrokeInputBatch& real_inputs,
                    const StrokeInputBatch& predicted_inputs) override;

 private:
  // Models a single `input`.
  //
  // The value of `last_input_in_update` indicates whether this is the last
  // input being modeled from a single call to `ExtendStroke()`. This last input
  // must always be "unstable".
  void ModelInput(std::vector<ModeledStrokeInput>& modeled_inputs,
                  const StrokeInput& input, bool last_input_in_update);

  // Updates `state` elapsed time and distance properties.
  void UpdateStateTimeAndDistance(
      InputModelerState& state,
      std::vector<ModeledStrokeInput>& modeled_inputs);

  Version version_;
  // We use `brush_epsilon` to set up the parameters for `stroke_modeler_`, and
  // to determine the minimum distance that a new `stroke_model::Result` must
  // travel from the previous accepted one in order to be turned into a
  // `ModeledStrokeInput`.
  float brush_epsilon_;
  stroke_model::StrokeModeler stroke_modeler_;
  std::vector<stroke_model::Result> result_buffer_;
  std::optional<StrokeInput> last_real_stroke_input_;
  bool stroke_modeler_has_input_ = false;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_INPUT_MODELER_SPRING_BASED_INPUT_MODELER_H_
