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

#include "ink/engine/geometry/line/tip/chisel_tip_model.h"

#include "third_party/glm/glm/gtx/norm.hpp"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

void ChiselTipModel::Clear() {
  left_.Clear();
  right_.Clear();
}

void ChiselTipModel::AddTurnPoints(
    const MidPoint& start, const MidPoint& middle, const MidPoint& end,
    int turn_vertices, std::function<void(glm::vec2 pt)> add_left,
    std::function<void(glm::vec2 pt)> add_right) {
  glm::vec2 chisel_tail = GetTailCenter(middle);
  float midpoint_turn = TurnAngle(start.screen_position, middle.screen_position,
                                  end.screen_position);
  // The angle from the line direction to the tail of the chisel shape.
  float turn_to_tail =
      TurnAngle(start.screen_position, middle.screen_position, chisel_tail);
  // The angle from the line direction to the head of the chisel shape.
  float turn_to_head = NormalizeAngle(turn_to_tail + M_PI);

  float radius = middle.tip_size.radius_minor;

  // Determine which side of the line the two points go on.
  bool head_goes_left = midpoint_turn < turn_to_head;
  bool tail_goes_left = midpoint_turn < turn_to_tail;


  if (head_goes_left == tail_goes_left) {
    // They're both going to the same side
    bool middle_first =
        AddBothToSide(middle.screen_position, chisel_tail, radius,
                      turn_vertices, head_goes_left ? &left_ : &right_,
                      head_goes_left ? add_left : add_right);
    // Inform the other side of the turn points so it can add the inner
    // intersection point if needed.
    if (middle_first) {
      ((head_goes_left) ? right_ : left_)
          .SetTurnPoints({middle.screen_position, radius},
                         {chisel_tail, radius});
    } else {
      ((head_goes_left) ? right_ : left_)
          .SetTurnPoints({chisel_tail, radius},
                         {middle.screen_position, radius});
    }
  } else {
    // Head and tail go to opposite sides.
    if (head_goes_left) {
      AddToSide(middle.screen_position, radius, turn_vertices, &left_,
                add_left);
      AddToSide(chisel_tail, radius, turn_vertices, &right_, add_right);
    } else {
      AddToSide(middle.screen_position, radius, turn_vertices, &right_,
                add_right);
      AddToSide(chisel_tail, radius, turn_vertices, &left_, add_left);
    }
  }
}

void ChiselTipModel::Side::Push(SidePoint point) {
  EXPECT(point.radius >= 0);
  previous_ = current_;
  current_ = point;
}

void ChiselTipModel::Side::Clear() {
  previous_.radius = -1;
  current_.radius = -1;
  ClearTurn();
}

void ChiselTipModel::Side::SetTurnPoints(SidePoint first, SidePoint second) {
  first_turn_ = first;
  second_turn_ = second;
}

void ChiselTipModel::Side::ClearTurn() {
  first_turn_.radius = -1;
  second_turn_.radius = -1;
}

void ChiselTipModel::AddToSide(glm::vec2 v, float radius, int turn_vertices,
                               Side* side,
                               std::function<void(glm::vec2 pt)> add) {
  SidePoint next = {v, radius};
  if (side->IsPopulated()) {
    SidePoint current = side->Current();
    SidePoint previous = side->Previous();
    SidePoint first_turn = side->FirstTurn();
    SidePoint second_turn = side->SecondTurn();

    if (first_turn.radius >= 0 && second_turn.radius >= 0) {
      auto first_tangent = FindLineTangent(current.position, current.radius,
                                           second_turn.position,
                                           second_turn.radius, side->IsLeft());
      auto second_tangent = FindLineTangent(
          first_turn.position, first_turn.radius, v, radius, side->IsLeft());

      side->ClearTurn();
      glm::vec2 intersection{0, 0};
      if (geometry::Intersection(first_tangent, second_tangent,
                                 &intersection)) {
        SidePoint intersect = {intersection, 0};
        side->Push(intersect);

        AddTangents(previous, current, intersect, side->IsLeft(), turn_vertices,
                    add);
        previous = current;
        current = intersect;
      }
    }

    AddTangents(previous, current, next, side->IsLeft(), turn_vertices, add);
  }

  side->Push(next);
}

void ChiselTipModel::AddTangents(SidePoint start, SidePoint middle,
                                 SidePoint end, bool is_left, int turn_vertices,
                                 std::function<void(glm::vec2 pt)> add) {
  auto in_tangent = FindLineTangent(start.position, start.radius,
                                    middle.position, middle.radius, is_left);
  auto out_tangent = FindLineTangent(middle.position, middle.radius,
                                     end.position, end.radius, is_left);
  float turn_angle = TurnAngle(start.position, middle.position, end.position);
  JoinLineTangents(middle.position, middle.radius, in_tangent, out_tangent,
                   turn_angle, is_left, turn_vertices, add);
}

bool ChiselTipModel::AddBothToSide(glm::vec2 first, glm::vec2 second,
                                   float radius, int turn_vertices, Side* side,
                                   std::function<void(glm::vec2 pt)> add) {
  SidePoint current = side->Current();
  if (current.radius <= 0 || glm::distance2(first, current.position) <
                                 glm::distance2(second, current.position)) {
    // Add the nearest one first.
    AddToSide(first, radius, turn_vertices, side, add);
    AddToSide(second, radius, turn_vertices, side, add);
    return true;
  } else {
    AddToSide(second, radius, turn_vertices, side, add);
    AddToSide(first, radius, turn_vertices, side, add);
    return false;
  }
}

Segment ChiselTipModel::FindLineTangent(glm::vec2 start, float start_radius,
                                        glm::vec2 end, float end_radius,
                                        bool left) {
  CircleTangents tangents =
      FindLineTangents(start, start_radius, end, end_radius);
  return left ? tangents.left : tangents.right;
}

glm::vec2 ChiselTipModel::GetTailCenter(const MidPoint& point) {

  float angle = point.stylus_state.orientation;
  glm::vec2 offset(cos(angle), sin(angle));
  offset *= 2.0f * point.tip_size.radius;
  return glm::vec2(point.screen_position + offset);
}

}  // namespace ink
