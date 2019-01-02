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

#include "ink/engine/brushes/tip_dynamics.h"

#include <algorithm>
#include <cmath>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

using glm::vec2;

TipDynamics::TipDynamics() : TipDynamics(ModelConstants()) {}

TipDynamics::TipDynamics(const ModelConstants& model_constants)
    : model_constants_(model_constants), size_(0.3f) {
  Reset(input::InputData());
}

void TipDynamics::Reset(const input::InputData& input) {
  time_ = input.time;
  position_ = input.world_pos;
  velocity_ = vec2(0, 0);
  speed_ = 0;
  previous_pressure_ = input.pressure;
  auto r = params_.GetRadius(size_);
  switch (params_.radius_behavior) {
    case BrushParams::TipShapeParams::RadiusBehavior::PRESSURE:
      tip_size_ = TipSizeWorld::FromPressure(r, input.pressure);
      break;
    case BrushParams::TipShapeParams::RadiusBehavior::TILT:
      tip_size_ = TipSizeWorld::FromTilt(r, input.tilt);
      break;
    default:
      tip_size_ = TipSizeWorld::FixedRadius(r.x);
  }
  stylus_modeler_.Clear();
}

void TipDynamics::UpdateSpeed(InputTimeS time, double world_dist,
                              vec2 new_position_world, Camera cam) {
  float speed = speed_;
  DurationS time_delta = time - time_;
  time_delta = time_delta >= 0 ? time_delta : 0;
  if (time_delta > 0) {
    // cm / second
    speed = cam.ConvertDistance(world_dist / static_cast<double>(time_delta),
                                DistanceType::kWorld, DistanceType::kCm);
    ASSERT(speed >= 0);
  }
  // speed penalty for direction changes
  vec2 vworld_dist = new_position_world - position_;
  auto vspeed = vec2(0);
  if (time_delta != 0) {
    vspeed = vworld_dist / static_cast<float>(time_delta);
  }

  if (glm::length(velocity_) > 0.001 &&
      cam.ConvertDistance(glm::length(vspeed), DistanceType::kWorld,
                          DistanceType::kCm) > 0.001) {
    auto cosang = glm::dot(vspeed, velocity_) /
                  (glm::length(vspeed) * glm::length(velocity_));
    cosang = util::Normalize(0.65f, 1.0f, cosang);
    cosang = pow(cosang, 3);
    speed = speed * cosang;
  }
  speed_ = speed;
}

void TipDynamics::ModSpeedForStrokeEnd(float multiplier) {
  speed_ *= multiplier;
  velocity_ *= multiplier;
}

void TipDynamics::AddRawInputData(const input::InputData& input) {
  stylus_modeler_.AddInputToModel(input);
}

TipDynamics::TipState TipDynamics::Tick(vec2 new_position_world,
                                        InputTimeS time, const Camera& cam) {
  // position smoothing
  velocity_ += ((new_position_world - position_) / model_constants_.shape_mass);
  position_ += velocity_;
  velocity_ *= (1 - model_constants_.shape_drag);

  // update speed
  auto world_dist = geometry::Distance(position_, new_position_world);
  auto screen_dist = cam.ConvertDistance(world_dist, DistanceType::kWorld,
                                         DistanceType::kScreen);
  UpdateSpeed(time, world_dist, new_position_world, cam);
  time_ = time;

  vec2 pos = (kDebugRawInput || model_constants_.no_position_modeling)
                 ? new_position_world
                 : position_;
  input::StylusState stylus_state = stylus_modeler_.Query(pos);

  TipState tip_state =
      TipState{pos, GetTipSize(stylus_state, screen_dist, cam), stylus_state};
  tip_size_ = tip_state.tip_size;
  SLOG(SLOG_INPUT, "radius: $0, pressure: $1", tip_size_.radius,
       stylus_state.pressure);
  return tip_state;
}

TipSizeWorld TipDynamics::GetTipSize(input::StylusState stylus_state,
                                     float screen_dist, const Camera& cam) {
  switch (params_.radius_behavior) {
    case BrushParams::TipShapeParams::RadiusBehavior::PRESSURE:
      return TipSizeWorld::FromPressure(params_.GetRadius(size_),
                                        stylus_state.pressure);
    case BrushParams::TipShapeParams::RadiusBehavior::SPEED:
      return TipSizeWorld::FromSpeedWithDrag(
          params_.GetRadius(size_), tip_size_, params_.speed_limit,
          params_.base_speed, speed_, screen_dist, cam);
    case BrushParams::TipShapeParams::RadiusBehavior::TILT:
      return TipSizeWorld::FromTilt(params_.GetRadius(size_),
                                    stylus_state.tilt);
    case BrushParams::TipShapeParams::RadiusBehavior::ORIENTATION:
      return TipSizeWorld::FromOrientation(params_.GetRadius(size_),
                                           VectorAngle(velocity_),
                                           stylus_state.orientation);
    case BrushParams::TipShapeParams::RadiusBehavior::FIXED:
    default:
      return tip_size_;
  }
}

}  // namespace ink
