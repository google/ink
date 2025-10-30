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

#include "ink/strokes/internal/stroke_input_modeler/spring_based_input_modeler.h"

#include <cstddef>
#include <optional>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/log/absl_check.h"
#include "ink/brush/brush_family.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/numbers.h"
#include "ink/types/physical_distance.h"
#include "ink_stroke_modeler/params.h"
#include "ink_stroke_modeler/stroke_modeler.h"
#include "ink_stroke_modeler/types.h"

namespace ink::strokes_internal {
namespace {

using ::ink::numbers::kPi;
using ::ink::stroke_model::PositionModelerParams;
using ::ink::stroke_model::SamplingParams;
using ::ink::stroke_model::StylusStateModelerParams;

constexpr float kDefaultLoopMitigationSpeedLowerBoundInCmPerSec = 0.0f;
constexpr float kDefaultLoopMitigationSpeedUpperBoundInCmPerSec = 25.0f;
constexpr float kDefaultLoopMitigationInterpolationStrengthAtSpeedLowerBound =
    1.0f;
constexpr float kDefaultLoopMitigationInterpolationStrengthAtSpeedUpperBound =
    0.5f;
const stroke_model::Duration kDefaultLoopMitigationMinSpeedSamplingWindow =
    stroke_model::Duration(0.04);

// The minimum output rate was chosen to match legacy behavior, which was
// in turn chosen to upsample enough to produce relatively smooth-looking
// curves on 60 Hz touchscreens.
constexpr double kMinOutputRateHz = 180;

// LINT.IfChange(input_model_types)

PositionModelerParams::LoopContractionMitigationParameters
MakeLoopContractionMitigationParameters(
    SpringBasedInputModeler::Version version,
    std::optional<PhysicalDistance> stroke_unit_length) {
  switch (version) {
    case SpringBasedInputModeler::Version::kSpringModel: {
      // Without the stroke unit length, we cannot determine the speed of
      // the stroke inputs, so we cannot enable loop mitigation.
      if (!stroke_unit_length.has_value()) {
        return {.is_enabled = false};
      }
      return {
          .is_enabled = true,
          .speed_lower_bound = kDefaultLoopMitigationSpeedLowerBoundInCmPerSec /
                               stroke_unit_length->ToCentimeters(),
          .speed_upper_bound = kDefaultLoopMitigationSpeedUpperBoundInCmPerSec /
                               stroke_unit_length->ToCentimeters(),
          .interpolation_strength_at_speed_lower_bound =
              kDefaultLoopMitigationInterpolationStrengthAtSpeedLowerBound,
          .interpolation_strength_at_speed_upper_bound =
              kDefaultLoopMitigationInterpolationStrengthAtSpeedUpperBound,
          .min_speed_sampling_window =
              kDefaultLoopMitigationMinSpeedSamplingWindow,
      };
    }
  }
}

StylusStateModelerParams MakeStylusStateModelerParams(
    SpringBasedInputModeler::Version version) {
  switch (version) {
    case SpringBasedInputModeler::Version::kSpringModel:
      return {.use_stroke_normal_projection = true};
  }
}

SamplingParams MakeSamplingParams(SpringBasedInputModeler::Version version,
                                  float brush_epsilon) {
  switch (version) {
    case SpringBasedInputModeler::Version::kSpringModel:
      return {.min_output_rate = kMinOutputRateHz,
              .end_of_stroke_stopping_distance = brush_epsilon,
              .max_estimated_angle_to_traverse_per_input = kPi / 8};
  }
}

// LINT.ThenChange(../../../brush/brush_family.h:input_model_types)

void ResetStrokeModeler(stroke_model::StrokeModeler& stroke_modeler,
                        SpringBasedInputModeler::Version version,
                        float brush_epsilon,
                        std::optional<PhysicalDistance> stroke_unit_length) {
  // We use the defaults for `PositionModelerParams` and
  // `StylusStateModelerParams`.
  ABSL_CHECK_OK(stroke_modeler.Reset(
      {// We turn off wobble smoothing because, in order to choose parameters
       // appropriately, we need to know the input rate and range of speeds that
       // we'll see for a stroke, which we don't have access to.
       .wobble_smoother_params = {.is_enabled = false},
       // We turn off loop contraction mitigation if the input model is not
       // spring model v2.
       .position_modeler_params = {.loop_contraction_mitigation_params =
                                       MakeLoopContractionMitigationParameters(
                                           version, stroke_unit_length)},
       // `brush_epsilon` is used for the stopping distance because once end of
       // the stroke is with `brush_epsilon` of the final input, further changes
       // are not considered visually distinct.
       .sampling_params = MakeSamplingParams(version, brush_epsilon),
       // If we use loop mitigation, we need to use the new projection method.
       .stylus_state_modeler_params = MakeStylusStateModelerParams(version),
       // We disable the internal predictor on the `StrokeModeler`,
       // because it performs prediction after modeling. We wish to
       // accept external un-modeled prediction, as in the case of
       // platform provided prediction.
       .prediction_params = stroke_model::DisabledPredictorParams()}));
}

}  // namespace

SpringBasedInputModeler::SpringBasedInputModeler(Version version,
                                                 float brush_epsilon)
    : version_(version), brush_epsilon_(brush_epsilon) {
  // The `stroke_modeler_` cannot be reset until we get the first input in order
  // to know the `StrokeInput::ToolType`.
  ABSL_CHECK_GT(brush_epsilon, 0);
}

void SpringBasedInputModeler::ExtendStroke(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) {
  absl::Cleanup update_time_and_distance = [&]() {
    UpdateStateTimeAndDistance(state, modeled_inputs);
  };

  if (!last_real_stroke_input_.has_value()) {
    ResetStrokeModeler(stroke_modeler_, version_, brush_epsilon_,
                       state.stroke_unit_length);
    stroke_modeler_has_input_ = false;
  }

  // Clear any "unstable" modeled inputs.
  modeled_inputs.resize(state.stable_input_count);

  // Re-model the current last real input as "stable" only if there are new
  // real inputs to process:
  if (last_real_stroke_input_.has_value() && !real_inputs.IsEmpty()) {
    ModelInput(modeled_inputs, *last_real_stroke_input_,
               /* last_input_in_update = */ false);
  }

  // Model all except the last new real input as "stable". The last one must
  // always be processed as "unstable", even in the case that current
  // `predicted_inputs` are non-empty, because a future update might have no new
  // predicted inputs.
  for (size_t i = 0; i + 1 < real_inputs.Size(); ++i) {
    ModelInput(modeled_inputs, real_inputs.Get(i),
               /* last_input_in_update = */ false);
  }

  // Save the state of the stroke modeler and model the remaining inputs as
  // "unstable".
  stroke_modeler_.Save();
  bool stroke_modeler_save_has_input = stroke_modeler_has_input_;
  state.stable_input_count = modeled_inputs.size();

  if (!real_inputs.IsEmpty()) {
    last_real_stroke_input_ = real_inputs.Last();
    ModelInput(modeled_inputs, *last_real_stroke_input_,
               /* last_input_in_update = */ predicted_inputs.IsEmpty());
  } else if (last_real_stroke_input_.has_value()) {
    ModelInput(modeled_inputs, *last_real_stroke_input_,
               /* last_input_in_update = */ predicted_inputs.IsEmpty());
  }

  state.real_input_count = modeled_inputs.size();

  for (size_t i = 0; i < predicted_inputs.Size(); ++i) {
    ModelInput(modeled_inputs, predicted_inputs.Get(i),
               /* last_input_in_update = */ i == predicted_inputs.Size() - 1);
  }

  stroke_modeler_.Restore();
  stroke_modeler_has_input_ = stroke_modeler_save_has_input;
}

void SpringBasedInputModeler::ModelInput(
    std::vector<ModeledStrokeInput>& modeled_inputs, const StrokeInput& input,
    bool last_input_in_update) {
  // The smoothing done by the `stroke_modeler_` causes the modeled results to
  // lag behind the current end of the stroke. This is usually made up for by
  // modeler's internal predictor, but we are disabling that to support external
  // prediction. Therefore, if we have more than one input in the stroke, we
  // need to model the last input in each update as a `kUp` event, which makes
  // the modeler catch up. This action is incompatible with further inputs, so
  // the `kUp` must be done after calling `stroke_modeler_.Save()`. This is why
  // this function should only be called with `last_input_in_update == true`
  // after the modeler save call. Note that the stroke modeler does model a
  // result for a `kDown` event, so a `kUp` is not necessary for a stroke
  // consisting of a single input.

  auto event_type = stroke_model::Input::EventType::kMove;
  if (!stroke_modeler_has_input_) {
    stroke_modeler_has_input_ = true;
    event_type = stroke_model::Input::EventType::kDown;
  } else if (last_input_in_update) {
    // The stroke modeler requires distinct `kDown` and `kUp` events, so we can
    // only pass a `kUp` if this is not the first input of the stroke.
    event_type = stroke_model::Input::EventType::kUp;
  }

  result_buffer_.clear();

  // `StrokeInputBatch` and `InProgressStroke` are designed to perform all the
  // necessary validation so that this operation should not fail.
  ABSL_CHECK_OK(stroke_modeler_.Update(
      {.event_type = event_type,
       .position = {input.position.x, input.position.y},
       .time = stroke_model::Time(input.elapsed_time.ToSeconds()),
       .pressure = input.pressure,
       .tilt = input.tilt.ValueInRadians(),
       .orientation = input.orientation.ValueInRadians()},
      result_buffer_));

  std::optional<Point> previous_position;
  float traveled_distance = 0;
  if (!modeled_inputs.empty()) {
    previous_position = modeled_inputs.back().position;
    traveled_distance = modeled_inputs.back().traveled_distance;
  }

  for (const stroke_model::Result& result : result_buffer_) {
    Point position = {.x = result.position.x, .y = result.position.y};

    if (previous_position.has_value()) {
      float delta = (position - *previous_position).Magnitude();
      if (delta < brush_epsilon_) continue;

      traveled_distance += delta;
    }

    modeled_inputs.push_back({
        .position = position,
        .velocity = {.x = result.velocity.x, .y = result.velocity.y},
        .acceleration = {.x = result.acceleration.x,
                         .y = result.acceleration.y},
        .traveled_distance = traveled_distance,
        .elapsed_time = Duration32::Seconds(result.time.Value()),
        .pressure = result.pressure,
        .tilt = Angle::Radians(result.tilt),
        .orientation = Angle::Radians(result.orientation),
    });

    previous_position = position;
  }
}

void SpringBasedInputModeler::UpdateStateTimeAndDistance(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs) {
  if (modeled_inputs.empty()) {
    return;
  }

  const ModeledStrokeInput& last_input = modeled_inputs.back();
  state.complete_elapsed_time = last_input.elapsed_time;
  state.complete_traveled_distance = last_input.traveled_distance;

  if (state.real_input_count == 0) return;

  const ModeledStrokeInput& last_real_input =
      modeled_inputs[state.real_input_count - 1];
  state.total_real_distance = last_real_input.traveled_distance;
  state.total_real_elapsed_time = last_real_input.elapsed_time;
}

}  // namespace ink::strokes_internal
