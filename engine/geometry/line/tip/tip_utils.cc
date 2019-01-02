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

#include "ink/engine/geometry/line/tip/tip_utils.h"

#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {

void AddRoundTurnPoints(const MidPoint& start, const MidPoint& middle,
                        const MidPoint& end, int turn_vertices,
                        std::function<void(glm::vec2 pt)> add_left,
                        std::function<void(glm::vec2 pt)> add_right) {
  CircleTangents in_tangents = FindLineTangents(start, middle);
  CircleTangents out_tangents = FindLineTangents(middle, end);
  float turn_angle = TurnAngle(start.screen_position, middle.screen_position,
                               end.screen_position);
  JoinLineTangents(middle.screen_position, middle.tip_size.radius,
                   in_tangents.left, out_tangents.left, turn_angle, true,
                   turn_vertices, add_left);
  JoinLineTangents(middle.screen_position, middle.tip_size.radius,
                   in_tangents.right, out_tangents.right, turn_angle, false,
                   turn_vertices, add_right);
}

CircleTangents FindLineTangents(glm::vec2 start, float start_radius,
                                glm::vec2 end, float end_radius) {
  CircleTangents tangents;
  bool has_tangents =
      CommonTangents(start, start_radius, end, end_radius, &tangents);
  if (!has_tangents) {
    // Fall back to parallel lines.
    auto v_in_angle = NormalizeAngle(VectorAngle(end - start));
    tangents.right.from =
        PointOnCircle(v_in_angle - M_PI_2, start_radius, start);
    tangents.left.from =
        PointOnCircle(v_in_angle + M_PI_2, start_radius, start);
    tangents.right.to = PointOnCircle(v_in_angle - M_PI_2, end_radius, end);
    tangents.left.to = PointOnCircle(v_in_angle + M_PI_2, end_radius, end);
  }

  return tangents;
}

void JoinLineTangents(glm::vec2 turn_point, float turn_radius,
                      Segment in_tangent, Segment out_tangent, float turn_angle,
                      bool is_left_edge, uint32_t n_turn_vertices,
                      std::function<void(glm::vec2 pt)> append_function) {
  glm::vec2 intersection{0, 0};
  if (geometry::Intersection(in_tangent, out_tangent, &intersection)) {
    append_function(intersection);
  } else {
    float angle1 = NormalizeAngle(VectorAngle(in_tangent.to - turn_point));
    float angle2 = NormalizeAngle(VectorAngle(out_tangent.from - turn_point));

    // This is the condition for inserting a turn joint on this side (where
    // the side is determined by "isLeftEdge"). For example, if we're on a
    // left turn, a turn joint should be inserted on the right edge.
    if ((is_left_edge && turn_angle <= 0) ||
        (!is_left_edge && turn_angle >= 0)) {
      FixAngles(&angle1, &angle2, is_left_edge);
      // If we're not turning more than 90°, then there is no way that the
      // generated turn should be over 180°. We do this to avoid loops, which
      // can be generated for a number of reasons, including floating point
      // precision errors in commonTangents.
      if (std::abs(turn_angle) < M_PI_2) {
        if (is_left_edge && angle2 - angle1 < -M_PI) {
          angle2 += M_TAU;
        } else if (!is_left_edge && angle2 - angle1 > M_PI) {
          angle2 -= M_TAU;
        }
      }
      // At this point, we're inserting a turn on this side, so we need to
      // create an arc from the end of the "in tangent" to the start of the
      // "out tangent".
      auto turn = PointsOnCircle(turn_point, turn_radius, n_turn_vertices,
                                 angle1, angle2);
      for (auto& pt : turn) {
        append_function(pt);
      }
    } else {
      // At this point, we've decided that this side doesn't need a turn
      // inserted, and we will just insert the relevant tangent points
      // directly.
      // Currently, this case is for when the edge is not on the same side as
      // the direction of the turn.
      append_function(in_tangent.to);
      append_function(out_tangent.from);
    }
  }
}

bool IsCircleWithinCircle(const MidPoint& inner, const MidPoint& outer) {
  return glm::length(outer.screen_position - inner.screen_position) +
             inner.tip_size.radius <=
         outer.tip_size.radius;
}

}  // namespace ink
