// Copyright 2018 Google LLC
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

#include "ink/engine/input/physics_input_modeler.h"
#include "ink/engine/input/input_modeler.h"

#include "third_party/absl/strings/substitute.h"
#include "ink/engine/brushes/tip_dynamics.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/input/modeled_input.h"
#include "ink/engine/math_defines.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/signal_filters/time_variant_moving_avg.h"

namespace ink {
namespace input {

using signal_filters::TimeVariantMovingAvg;

template <typename OutputIt>
void PhysicsInputModeler::GenModeledInput(
    const Camera& cam, const InputModelParams& params, TipDynamics* dynamics,
    const InputData& data, const InputData& last_input_sent_to_model,
    OutputIt output) {
  if (data.Get(Flag::TDown)) {
    dynamics->Reset(data);
  }
  dynamics->AddRawInputData(data);

  glm::vec2 last_direction = data.screen_pos - data.last_screen_pos;
  int interpolation_points = params.NumInterpolationPoints();
  int total_points =
      data.Get(Flag::TUp) ? params.MaxPointsAfterUp() : interpolation_points;

  if (kDebugRawInput || data.Get(Flag::TDown)) {
    total_points = 1;
    interpolation_points = 1;
  }

  for (int i = 0; i < total_points; i++) {
    // If interpolation_points == 3, then interp will be 1/3, 2/3, 1, 1, 1 ...
    float interp =
        util::Clamp01(static_cast<float>(i + 1) / interpolation_points);

    auto l_pos_world =
        util::Lerp(last_input_sent_to_model.world_pos, data.world_pos, interp);
    auto l_pos_screen =
        cam.ConvertPosition(l_pos_world, CoordType::kWorld, CoordType::kScreen);
    InputTimeS l_time =
        util::Lerp(last_input_sent_to_model.time, data.time, interp);

    TipDynamics::TipState tip_state = dynamics->Tick(l_pos_world, l_time, cam);
    auto screen_pos = cam.ConvertPosition(
        tip_state.world_position, CoordType::kWorld, CoordType::kScreen);
    *output++ = ModeledInput{tip_state.world_position, l_time,
                             tip_state.tip_size, tip_state.stylus_state};

    // Keep generating points after the up to:
    //   1) Give the result a feeling of weight
    //   2) Make up for the model running behind input
    //   3) Force the model to go near the TUp location
    if (data.Get(Flag::TUp)) {
      dynamics->ModSpeedForStrokeEnd(params.SpeedModForStrokeEnd());

      auto screen_distance_to_end =
          geometry::Distance(screen_pos, data.screen_pos);
      // If we've generated the maximum number of points or have reached our
      // target, stop
      if (screen_distance_to_end < 1 || kDebugRawInput) {
        break;
      } else {
        // look for a direction shift -- us looping around the target point
        // doesn't look good
        auto dir = screen_pos - l_pos_screen;
        if (glm::dot(dir, last_direction) > 0) {
          break;
        }
      }
    }
  }
}

PhysicsInputModeler::PhysicsInputModeler(
    std::shared_ptr<InputPredictor> predictor)
    : predictor_(predictor), params_(InputType::Invalid) {
  Init();
}

void PhysicsInputModeler::Init() {
  last_input_sent_to_model_.time = InputTimeS(0);
  TipDynamics::ModelConstants mc;
  mc.shape_mass = params_.Mass();
  mc.shape_drag = params_.Drag();
  dynamics_.SetModelConstants(mc);

  // Generic tip shape params (equivalent to MARKER).
  BrushParams::TipShapeParams tip_shape_params;
  tip_shape_params.size_ratio = 1;
  tip_shape_params.speed_limit = 200;
  tip_shape_params.radius_behavior =
      BrushParams::TipShapeParams::RadiusBehavior::FIXED;

  // now virtual SetRadiusParams(tip_shape_params, 0.3);
  dynamics_.params_ = tip_shape_params;
  dynamics_.SetSize(0.3);

  avg_world_pos_ = TimeVariantMovingAvg<glm::vec2, InputTimeS>();
  avg_cm_speed_ = TimeVariantMovingAvg<float, InputTimeS>();
}

void PhysicsInputModeler::Reset(const Camera& cam, InputModelParams params) {
  params_ = params;
  cam_ = cam;
  modeled_input_.clear();
  last_input_received_ = InputData();
  Init();

  predictor_->Reset(cam, params.PredictInterval(), 1.0f / params.MaxSampleHz());
}

void PhysicsInputModeler::SetParams(BrushParams::TipShapeParams params,
                                    float base_world_radius) {
  dynamics_.params_ = params;
  dynamics_.SetSize(base_world_radius);
}

ModeledInput PhysicsInputModeler::PopNextModelResult() {
  ASSERT(HasModelResult());
  auto res = modeled_input_.front();
  modeled_input_.pop_front();
  return res;
}

bool PhysicsInputModeler::HasModelResult() const {
  return !modeled_input_.empty();
}

std::vector<ModeledInput> PhysicsInputModeler::PredictModelResults() const {
  auto model_velocity = cam_.ConvertVector(
      dynamics_.VelocityWorld(), CoordType::kWorld, CoordType::kScreen);
  auto predicted_data = predictor_->PredictedPoints(
      cam_.ConvertPosition(last_modeled_input_.world_pos, CoordType::kWorld,
                           CoordType::kScreen),
      model_velocity);

  std::vector<ModeledInput> res;
  if (predictor_->PredictionExpectsModeling()) {
    res.reserve(predicted_data.size() * params_.NumInterpolationPoints() +
                params_.MaxPointsAfterUp());
    auto predicted_dynamics = dynamics_;
    auto model_constants = predicted_dynamics.GetModelConstants();
    if (last_input_sent_to_model_.type != input::InputType::Touch) {
      model_constants.shape_drag *= 0.5;
      predicted_dynamics.ModSpeedForStrokeEnd(params_.SpeedModForStrokeEnd());
      predicted_dynamics.ModSpeedForStrokeEnd(params_.SpeedModForStrokeEnd());
    } else {
      model_constants.shape_drag *= 0.7;
    }
    predicted_dynamics.SetModelConstants(model_constants);

    InputData last_sent = last_input_sent_to_model_;
    for (const auto& to_send : predicted_data) {
      predicted_dynamics.ModSpeedForStrokeEnd(params_.SpeedModForStrokeEnd());
      GenModeledInput(cam_, params_, &predicted_dynamics, to_send, last_sent,
                      std::back_inserter(res));
      last_sent = to_send;
    }
  } else {
    res.reserve(predicted_data.size());
    // Linearly taper the predicted line size from the size of the previous
    // radius depending on the tool's taper_amount.
    TipSizeWorld ending_size =
        last_modeled_input_.tip_size * (1.0 - dynamics_.params_.taper_amount);
    for (int i = 0; i < predicted_data.size(); i++) {
      auto prediction = predicted_data[i];
      ModeledInput modeled_input = last_modeled_input_;
      modeled_input.time = prediction.time;
      modeled_input.world_pos = prediction.world_pos;
      modeled_input.tip_size =
          util::Lerp(last_modeled_input_.tip_size, ending_size,
                     static_cast<float>(i) / predicted_data.size());

      res.push_back(modeled_input);
    }
  }

  return res;
}

void PhysicsInputModeler::AddInputToModel(InputData data) {
  SLOG(SLOG_INPUT, "Input model received input at time=$0", data.time);

  // Wobbliness needs to be handled before we save the InputData, or the
  // prediction will still be wobbly.
  if (!kDebugRawInput) FilterWobble(&data);
  last_input_received_ = data;

  DurationS min_sample_dt(1.0 / params_.MaxSampleHz());
  DurationS dt = data.time - last_input_sent_to_model_.time;
  bool send_to_modeler = dt >= min_sample_dt || data.Get(Flag::TUp) ||
                         data.Get(Flag::TDown) || kDebugRawInput;
  if (send_to_modeler) {
    size_t first_size = modeled_input_.size();
    GenModeledInput(cam_, params_, &dynamics_, data, last_input_sent_to_model_,
                    std::back_inserter(modeled_input_));
    SLOG(SLOG_INPUT, "Input model generated $0 inputs",
         modeled_input_.size() - first_size);
    last_modeled_input_ = modeled_input_.back();
    last_input_sent_to_model_ = data;
  } else {
    SLOG(SLOG_INPUT, "InputData discarded: above maximum sample rate.");
  }

  predictor_->Update(data, send_to_modeler);
}

std::string PhysicsInputModeler::ToString() const {
  return Substitute("input model params:($0)", params_);
}

void PhysicsInputModeler::FilterWobble(InputData* data) {
  // The moving average acts as a low-pass signal filter, removing
  // high-frequency fluctuations in the position caused by the discrete nature
  // of the touch digitizer. To compensate for the distance between the average
  // position and the actual position, we interpolate between them, based on
  // speed, to determine the position to use for the input model.
  // The TDown and TUp packets are not altered -- we still want the line to
  // start and end in the same place.
  if (data->Get(Flag::TDown)) {
    DurationS timeout(params_.WobbleTimeoutRatio() / params_.MaxSampleHz());
    avg_world_pos_ = TimeVariantMovingAvg<glm::vec2, InputTimeS>(
        data->world_pos, data->time, timeout);
    // Initialize with the "fast" speed -- otherwise, we'll lag behind at the
    // start of the stroke.
    avg_cm_speed_ = TimeVariantMovingAvg<float, InputTimeS>(
        params_.WobbleFastSpeedCM(), data->time, timeout);
  } else if (!data->Get(Flag::TUp)) {
    float delta_cm =
        cam_.ConvertDistance(glm::length(data->DeltaScreen()),
                             DistanceType::kScreen, DistanceType::kCm);
    DurationS delta_time = data->DeltaTime();

    // If neither time nor position has changed, then this must be a duplicate
    // packet -- don't update the averages.
    if (delta_time > 0 || delta_cm > 0) {
      float cm_speed = 0;
      if (delta_time == 0) {
        // We're going to assume that you're not actually moving infinitely
        // fast.
        cm_speed = std::max(params_.WobbleFastSpeedCM(), avg_cm_speed_.Value());
      } else {
        cm_speed = delta_cm / static_cast<double>(delta_time);
      }
      avg_cm_speed_.Sample(cm_speed, data->time);
      avg_world_pos_.Sample(data->world_pos, data->time);
    }

    data->world_pos = util::Lerp(
        avg_world_pos_.Value(), data->world_pos,
        util::Normalize(params_.WobbleSlowSpeedCM(),
                        params_.WobbleFastSpeedCM(), avg_cm_speed_.Value()));
    data->screen_pos = cam_.ConvertPosition(data->world_pos, CoordType::kWorld,
                                            CoordType::kScreen);
    InputData::SetLastPacketInfo(data, &last_input_received_);
  }
}
}  // namespace input
}  // namespace ink
