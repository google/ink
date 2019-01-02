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

#include "ink/engine/geometry/line/tip/square_tip_model.h"

#include "ink/engine/geometry/line/tip/tip_utils.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {

void SquareTipModel::AddTurnPoints(
    const MidPoint& start, const MidPoint& middle, const MidPoint& end,
    int turn_vertices, std::function<void(glm::vec2 pt)> add_left,
    std::function<void(glm::vec2 pt)> add_right) {
  AddRoundTurnPoints(start, middle, end, turn_vertices, add_left, add_right);
}

std::vector<glm::vec2> SquareTipModel::CreateStartcap(const MidPoint& first,
                                                      const MidPoint& second,
                                                      int turn_vertices) const {
  return CreateEndcap(second, first, turn_vertices);
}
std::vector<glm::vec2> SquareTipModel::CreateEndcap(
    const MidPoint& second_to_last, const MidPoint& last,
    int turn_vertices) const {
  auto vert = last.screen_position;
  float vert_radius = last.tip_size.radius;
  auto vert_in = second_to_last.screen_position;
  float angle = VectorAngle(vert - vert_in);
  float left_angle = angle - M_PI_2;
  float right_angle = angle + M_PI_2;
  auto forward_point = PointOnCircle(angle, vert_radius, vert);
  auto left_corner = PointOnCircle(left_angle, vert_radius, forward_point);
  auto right_corner = PointOnCircle(right_angle, vert_radius, forward_point);
  std::vector<glm::vec2> cap;
  cap.push_back(PointOnCircle(right_angle, vert_radius, vert));
  cap.push_back(right_corner);
  cap.push_back(left_corner);
  cap.push_back(PointOnCircle(left_angle, vert_radius, vert));
  return cap;
}

}  // namespace ink
