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

#include "ink/strokes/internal/stroke_input_modeler/naive_input_modeler.h"

#include <cstddef>
#include <vector>

#include "ink/geometry/distance.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

void NaiveInputModeler::ExtendStroke(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) {
  AppendInputs(state, modeled_inputs, real_inputs);
  if (!modeled_inputs.empty()) {
    const ModeledStrokeInput& last_real_input = modeled_inputs.back();
    state.total_real_elapsed_time = last_real_input.elapsed_time;
    state.total_real_distance = last_real_input.traveled_distance;
  }
  state.real_input_count = modeled_inputs.size();
  state.stable_input_count = state.real_input_count;
  AppendInputs(state, modeled_inputs, predicted_inputs);
}

void NaiveInputModeler::AppendInputs(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    const StrokeInputBatch& inputs) {
  for (size_t i = 0; i < inputs.Size(); ++i) {
    StrokeInput input = inputs.Get(i);
    float traveled_distance = 0;
    Vec velocity;
    Vec acceleration;
    if (!modeled_inputs.empty()) {
      const ModeledStrokeInput& last_input = modeled_inputs.back();
      traveled_distance = last_input.traveled_distance +
                          Distance(last_input.position, input.position);
      float delta_seconds =
          (input.elapsed_time - last_input.elapsed_time).ToSeconds();
      if (delta_seconds > 0) {
        velocity = (input.position - last_input.position) / delta_seconds;
        acceleration = (velocity - last_input.velocity) / delta_seconds;
      }
    }
    state.complete_elapsed_time = input.elapsed_time;
    state.complete_traveled_distance = traveled_distance;
    modeled_inputs.push_back(ModeledStrokeInput{
        .position = input.position,
        .velocity = velocity,
        .acceleration = acceleration,
        .traveled_distance = traveled_distance,
        .elapsed_time = input.elapsed_time,
        .pressure = input.pressure,
        .tilt = input.tilt,
        .orientation = input.orientation,
    });
  }
}

}  // namespace ink::strokes_internal
