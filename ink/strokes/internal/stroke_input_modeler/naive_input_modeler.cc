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

#include <algorithm>
#include <cstddef>

#include "ink/geometry/distance.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

void NaiveInputModeler::StartStroke(float brush_epsilon) {
  modeled_inputs_.clear();
  state_ = State{};
}

void NaiveInputModeler::ExtendStroke(const StrokeInputBatch& real_inputs,
                                     const StrokeInputBatch& predicted_inputs,
                                     Duration32 current_elapsed_time) {
  modeled_inputs_.resize(state_.real_input_count);
  AppendInputs(real_inputs);
  if (modeled_inputs_.empty()) {
    state_.complete_elapsed_time = Duration32::Zero();
    state_.total_real_elapsed_time = Duration32::Zero();
    state_.total_real_distance = 0;
  } else {
    const ModeledStrokeInput& last_real_input = modeled_inputs_.back();
    state_.complete_elapsed_time = last_real_input.elapsed_time;
    state_.total_real_elapsed_time = last_real_input.elapsed_time;
    state_.total_real_distance = last_real_input.traveled_distance;
  }
  state_.real_input_count = modeled_inputs_.size();
  state_.stable_input_count = state_.real_input_count;
  AppendInputs(predicted_inputs);
  state_.complete_elapsed_time =
      std::max(state_.complete_elapsed_time, current_elapsed_time);
}

void NaiveInputModeler::AppendInputs(const StrokeInputBatch& inputs) {
  if (inputs.HasStrokeUnitLength()) {
    state_.stroke_unit_length = inputs.GetStrokeUnitLength();
  }
  for (size_t i = 0; i < inputs.Size(); ++i) {
    StrokeInput input = inputs.Get(i);
    float traveled_distance = 0;
    Vec velocity;
    Vec acceleration;
    if (!modeled_inputs_.empty()) {
      const ModeledStrokeInput& last_input = modeled_inputs_.back();
      traveled_distance = last_input.traveled_distance +
                          Distance(last_input.position, input.position);
      float delta_seconds =
          (input.elapsed_time - last_input.elapsed_time).ToSeconds();
      if (delta_seconds > 0) {
        velocity = (input.position - last_input.position) / delta_seconds;
        acceleration = (velocity - last_input.velocity) / delta_seconds;
      }
    }
    state_.tool_type = input.tool_type;
    state_.complete_elapsed_time = input.elapsed_time;
    state_.complete_traveled_distance = traveled_distance;
    modeled_inputs_.push_back(ModeledStrokeInput{
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
