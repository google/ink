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

#include "ink/engine/geometry/primitives/vector_utils.h"

#include <limits>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/floats.h"

namespace ink {

RelativePos Orientation(glm::vec2 p1, glm::vec2 p2, glm::vec2 test_point) {
  using util::floats::NextFloat;
  using util::floats::PreviousFloat;

  if (p1 == p2)
    return RelativePos::kIndeterminate;
  else if (p1 == test_point || p2 == test_point)
    return RelativePos::kCollinear;

  auto line_vector = p2 - p1;
  float det = Determinant(line_vector, test_point - p1);
  if (det == 0) return RelativePos::kCollinear;

  // Checking whether the next representable point lies on the opposite side of
  // the line is relatively expensive, so we first check whether the test point
  // is close enough to the line. We use twice the machine epsilon of the
  // largest component-value as an approximation of "close enough" (this is
  // actually slightly larger than actual maximum "close enough" distance, which
  // is fine -- it still allows us to prune the vast majority of uninteresting
  // cases).
  //
  // We can't actually call geometry::Distance() here, as that would introduce a
  // circular dependency. However, recalling that det(a, b) = ‖a‖‖b‖sinθ, and
  // that a⋅a = ‖a‖², we can see that det(a, b)² / a⋅a = ‖b‖²sin²θ, which is the
  // squared height of the triangle formed by vectors a and b, i.e. the distance
  // from a point to a line.
  float max_distance =
      2 * std::numeric_limits<float>::epsilon() *
      std::max(std::max(std::max(std::abs(p1.x), std::abs(p1.y)),
                        std::max(std::abs(p2.x), std::abs(p2.y))),
               std::max(std::abs(test_point.x), std::abs(test_point.y)));
  if (det * det <=
      max_distance * max_distance * glm::dot(line_vector, line_vector)) {
    auto adjust_for_error = [](glm::vec2 v, glm::vec2 dir) {
      auto adjusted = v;
      if (dir.x > 0) {
        adjusted.x = NextFloat(v.x);
      } else if (dir.x < 0) {
        adjusted.x = PreviousFloat(v.x);
      }
      if (dir.y > 0) {
        adjusted.y = NextFloat(v.y);
      } else if (dir.y < 0) {
        adjusted.y = PreviousFloat(v.y);
      }
      return adjusted;
    };

    auto error_vector =
        det > 0 ? Orthogonal(line_vector) : -Orthogonal(line_vector);
    if (glm::dot(test_point, test_point) >
        std::max(glm::dot(p1, p1), glm::dot(p2, p2))) {
      test_point = adjust_for_error(test_point, -error_vector);
    } else {
      p1 = adjust_for_error(p1, error_vector);
      p2 = adjust_for_error(p2, error_vector);
    }
    if (det * Determinant(p2 - p1, test_point - p1) <= 0)
      return RelativePos::kCollinear;
  }

  return det > 0 ? RelativePos::kLeft : RelativePos::kRight;
}

RelativePos OrientationAboutTurn(glm::vec2 turn_start, glm::vec2 turn_middle,
                                 glm::vec2 turn_end, glm::vec2 test_point) {
  auto turn_orientation = Orientation(turn_start, turn_middle, turn_end);
  if (turn_orientation == RelativePos::kCollinear ||
      turn_orientation == RelativePos::kIndeterminate) {
    if (glm::dot(turn_middle - turn_start, turn_end - turn_middle) < 0) {
      // The turn folds back on itself.
      return RelativePos::kIndeterminate;
    } else {
      // The turn points are collinear, treat it as a straight line.
      return Orientation(turn_start, turn_end, test_point);
    }
  }

  auto segment1_orientation = Orientation(turn_start, turn_middle, test_point);
  auto segment2_orientation = Orientation(turn_middle, turn_end, test_point);
  ASSERT(segment1_orientation != RelativePos::kIndeterminate);
  ASSERT(segment2_orientation != RelativePos::kIndeterminate);

  if (segment1_orientation == turn_orientation &&
      segment2_orientation == turn_orientation)
    return turn_orientation;

  auto opposite_orientation = ReverseOrientation(turn_orientation);
  if (segment1_orientation == opposite_orientation ||
      segment2_orientation == opposite_orientation) {
    return opposite_orientation;
  } else {
    return RelativePos::kCollinear;
  }
}

}  // namespace ink
