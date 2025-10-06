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

#include "ink/strokes/internal/stroke_input_modeler.h"

#include <algorithm>
#include <memory>
#include <variant>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "ink/brush/brush_family.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler/input_model_impl.h"
#include "ink/strokes/internal/stroke_input_modeler/naive_input_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler/sliding_window_input_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler/spring_based_input_modeler.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {
namespace {

absl_nonnull std::unique_ptr<InputModelImpl> CreateInputModeler(
    const BrushFamily::SpringModel& spring_model, float brush_epsilon) {
  return std::make_unique<SpringBasedInputModeler>(
      SpringBasedInputModeler::Version::kSpringModel, brush_epsilon);
}

absl_nonnull std::unique_ptr<InputModelImpl> CreateInputModeler(
    const BrushFamily::ExperimentalRawPositionModel& raw_position_model,
    float brush_epsilon) {
  return std::make_unique<SpringBasedInputModeler>(
      SpringBasedInputModeler::Version::kExperimentalRawPositionModel,
      brush_epsilon);
}

absl_nonnull std::unique_ptr<InputModelImpl> CreateInputModeler(
    const BrushFamily::ExperimentalNaiveModel& naive_model,
    float brush_epsilon) {
  return std::make_unique<NaiveInputModeler>();
}

absl_nonnull std::unique_ptr<InputModelImpl> CreateInputModeler(
    const BrushFamily::SlidingWindowModel& sliding_window_model,
    float brush_epsilon) {
  return std::make_unique<SlidingWindowInputModeler>(
      sliding_window_model.window_size, sliding_window_model.upsampling_period);
}

}  // namespace

void StrokeInputModeler::StartStroke(const BrushFamily::InputModel& input_model,
                                     float brush_epsilon) {
  ABSL_CHECK_GT(brush_epsilon, 0);
  state_ = InputModelerState{};
  modeled_inputs_.clear();
  input_model_impl_ = std::visit(
      [brush_epsilon](auto& model) {
        return CreateInputModeler(model, brush_epsilon);
      },
      input_model);
}

void StrokeInputModeler::ExtendStroke(const StrokeInputBatch& real_inputs,
                                      const StrokeInputBatch& predicted_inputs,
                                      Duration32 current_elapsed_time) {
  ABSL_CHECK_NE(input_model_impl_, nullptr)
      << "`StartStroke()` has not been called.";
  ErasePredictedModeledInputs();
  SetToolTypeAndStrokeUnitLength(real_inputs, predicted_inputs);
  input_model_impl_->ExtendStroke(state_, modeled_inputs_, real_inputs,
                                  predicted_inputs);
  ABSL_DCHECK_LE(state_.stable_input_count, state_.real_input_count);
  ABSL_DCHECK_LE(state_.real_input_count, modeled_inputs_.size());
  state_.complete_elapsed_time =
      std::max(state_.complete_elapsed_time, current_elapsed_time);
}

void StrokeInputModeler::ErasePredictedModeledInputs() {
  modeled_inputs_.resize(state_.real_input_count);
  state_.complete_elapsed_time = state_.total_real_elapsed_time;
  state_.complete_traveled_distance = state_.total_real_distance;
}

void StrokeInputModeler::SetToolTypeAndStrokeUnitLength(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) {
  if (!real_inputs.IsEmpty()) {
    state_.tool_type = real_inputs.GetToolType();
    state_.stroke_unit_length = real_inputs.GetStrokeUnitLength();
  } else if (!predicted_inputs.IsEmpty()) {
    state_.tool_type = predicted_inputs.GetToolType();
    state_.stroke_unit_length = predicted_inputs.GetStrokeUnitLength();
  }
}

}  // namespace ink::strokes_internal
