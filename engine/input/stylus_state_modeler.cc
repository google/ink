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

#include "ink/engine/input/stylus_state_modeler.h"

#include "third_party/glm/glm/gtx/norm.hpp"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {
namespace input {

void StylusStateModeler::AddInputToModel(const input::InputData& input) {
  // Input type is expected to be consistent throughout the stroke (until
  // Clear()).
  if (input.type == InputType::Pen) {
    ASSERT(orientation_state_ == OrientationState::kEmpty);
    points_.push_back(input.world_pos);
    states_.push_back(
        StylusState({input.pressure, input.tilt, input.orientation}));
  } else {
    // For non-pen input, we want to determine a synthetic stylus orientation
    // based upon the first two non-identical input coordinates.
    // We only need to actually insert one point/state because the state values
    // will be identical for every point, so we just need one to match against.
    if (orientation_state_ == OrientationState::kEmpty) {
      orientation_point_ = input.world_pos;
      orientation_state_ = OrientationState::kOnePoint;
    } else if (orientation_state_ == OrientationState::kOnePoint &&
               input.world_pos != orientation_point_) {
      // Orientation of the stylus is assumed to be perpendicular to the
      // stroke direction.
      float angle = NormalizeAngle(
          VectorAngle(input.world_pos - orientation_point_) - M_PI / 2.0f);
      states_.push_back(StylusState({-1, 0, angle}));
      points_.push_back(input.world_pos);
      orientation_state_ = OrientationState::kReady;
    }
  }
}

void StylusStateModeler::Clear() {
  points_.clear();
  states_.clear();
  index_ = 0;
  orientation_state_ = OrientationState::kEmpty;
}

StylusState StylusStateModeler::Query(glm::vec2 point) {
  ASSERT(points_.size() == states_.size());

  if (points_.empty()) return kStylusStateUnknown;

  if (points_.size() == 1) {
    // Only one point in the world, so just return it.
    return states_.front();
  }

  float min_dist = SquaredDistanceToSegment(index_, point);
  for (int i = index_ + 1; i < points_.size() - 1; i++) {
    float dist = SquaredDistanceToSegment(i, point);
    if (dist <= min_dist) {
      min_dist = dist;
      index_ = i;
    }
  }

  // Find the point on points_[index]->points[index+1] that is nearest to point
  // and interpolate state values between those known values.
  float interp =
      Segment(points_[index_], points_[index_ + 1]).NearestPoint(point);

  // Handle wrapping of orientation [0, 2PI)
  float from_orientation = states_[index_].orientation;
  float to_orientation = states_[index_ + 1].orientation;
  float delta = to_orientation - from_orientation;

  // Adjust orientations if needed in order to go the shortest way around the
  // circle when interpolating.
  if (delta < -M_PI) {
    to_orientation += 2 * M_PI;
  } else if (delta > M_PI) {
    from_orientation += 2 * M_PI;
  }
  float orientation = util::Lerp(from_orientation,
                                 to_orientation, interp);
  if (orientation >= 2 * M_PI) {
    orientation -= 2 * M_PI;
  }

  return StylusState(
      {util::Lerp(states_[index_].pressure, states_[index_ + 1].pressure,
                  interp),
       util::Lerp(states_[index_].tilt, states_[index_ + 1].tilt, interp),
       orientation});
}

float StylusStateModeler::SquaredDistanceToSegment(int index, glm::vec2 point) {
  ASSERT(index < points_.size() - 1);
  Segment segment(points_[index], points_[index + 1]);
  return glm::distance2(point, segment.Eval(segment.NearestPoint(point)));
}

}  // namespace input
}  // namespace ink
