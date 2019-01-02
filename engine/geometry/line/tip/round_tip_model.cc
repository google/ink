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

#include "ink/engine/geometry/line/tip/round_tip_model.h"

#include "ink/engine/geometry/line/tip/tip_utils.h"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {

void RoundTipModel::AddTurnPoints(const MidPoint& start, const MidPoint& middle,
                                  const MidPoint& end, int turn_vertices,
                                  std::function<void(glm::vec2 pt)> add_left,
                                  std::function<void(glm::vec2 pt)> add_right) {
  AddRoundTurnPoints(start, middle, end, turn_vertices, add_left, add_right);
}

std::vector<glm::vec2> RoundTipModel::CreateStartcap(const MidPoint& first,
                                                     const MidPoint& second,
                                                     int turn_vertices) const {
  CircleTangents tangents = FindLineTangents(first, second);
  float left_angle =
      NormalizeAngle(VectorAngle(tangents.left.from - first.screen_position));
  float right_angle =
      NormalizeAngle(VectorAngle(tangents.right.from - first.screen_position));
  return MakeArc(first.screen_position, first.tip_size.radius, turn_vertices,
                 right_angle, left_angle, true);
}

std::vector<glm::vec2> RoundTipModel::CreateEndcap(
    const MidPoint& second_to_last, const MidPoint& last,
    int turn_vertices) const {
  CircleTangents tangents = FindLineTangents(
      second_to_last.screen_position, second_to_last.tip_size.radius,
      last.screen_position, last.tip_size.radius);
  float left_angle =
      NormalizeAngle(VectorAngle(tangents.left.to - last.screen_position));
  float right_angle =
      NormalizeAngle(VectorAngle(tangents.right.to - last.screen_position));
  return MakeArc(last.screen_position, last.tip_size.radius, turn_vertices,
                 left_angle, right_angle, true);
}

bool RoundTipModel::ShouldPruneBeforeNewPoint(
    const std::vector<MidPoint>& existing_points, const MidPoint& new_point) {
  // Prune before the new point if all existing points are within the circle of
  // the new point.
  for (const MidPoint& pt : existing_points) {
    if (!IsCircleWithinCircle(pt, new_point)) {
      return false;
    }
  }
  return true;
}

bool RoundTipModel::ShouldDropNewPoint(const MidPoint& previous_point,
                                       const MidPoint& new_point) {
  // Drop the new point if it's within the circle of the old point.
  return IsCircleWithinCircle(new_point, previous_point);
}

}  // namespace ink
