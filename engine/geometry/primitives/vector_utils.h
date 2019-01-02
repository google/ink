/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_VECTOR_UTILS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_VECTOR_UTILS_H_

#include <cmath>
#include <cstddef>
#include <functional>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

// This struct constructs a very basic hash value for a glm::vec2, allowing it
// to be used in a std::unordered_map or std::unordered_set.
struct Vec2Hasher {
  size_t operator()(const glm::vec2& v) const {
    return std::hash<float>()(v.x) + std::hash<float>()(v.y);
  }
};

// Returns the ccw angle from the positive x-axis to "vec" in radians (-pi,pi].*
//
// *For special values of vec, i.e. (-1,-0), may return -pi, see atan2 spec.
inline float VectorAngle(glm::vec2 vec) {
  // returns the ccw offset between the positive x axis and vec
  return std::atan2(vec.y, vec.x);
}

// Returns the determinant of the given 2D vectors. For 2D vectors, the
// determinant is equal to the z-coordinate of the vectors' 3D cross product,
// and as such has the following property:
//    Determinant(u, v) = length(u) * length(v) * sin(theta)
// where theta is the angle, measured counter-clockwise from u to v.
inline float Determinant(glm::vec2 u, glm::vec2 v) {
  return u.x * v.y - u.y * v.x;
}

// Returns a vector orthogonal to the given one, found by rotating the input
// vector 90° counter-clockwise.
inline glm::vec2 Orthogonal(glm::vec2 v) { return {-v.y, v.x}; }

enum class RelativePos { kIndeterminate, kLeft, kCollinear, kRight };

inline RelativePos ReverseOrientation(RelativePos orientation) {
  if (orientation == RelativePos::kLeft) return RelativePos::kRight;
  if (orientation == RelativePos::kRight) return RelativePos::kLeft;
  return orientation;
}

// Returns the orientation of the test point w.r.t. the line through points p1
// and p2. To account for floating-point error, the test point is considered
// collinear with the line if taking the next representable values of either the
// test point or the line results in a change in sign of the determinant.
RelativePos Orientation(glm::vec2 p1, glm::vec2 p2, glm::vec2 test_point);

// Returns the orientation of the test point w.r.t. to a "turn" defined by the
// three given points. Note that the turn is considered to extend infinitely
// along its tangents
RelativePos OrientationAboutTurn(glm::vec2 turn_start, glm::vec2 turn_middle,
                                 glm::vec2 turn_end, glm::vec2 test_point);

// The return value will be in radians, and will lie in the interval [0, pi].
inline float AngleBetweenVectors(glm::vec2 a, glm::vec2 b) {
  return std::acos(
      util::Clamp(-1.0f, 1.0f, glm::dot(glm::normalize(a), glm::normalize(b))));
}

// Angle is measured counterclockwise from a to b, assuming a right-handed
// coordinate system. The return value will be in radians, and will lie in the
// interval [-pi, pi].
inline float SignedAngleBetweenVectors(glm::vec2 a, glm::vec2 b) {
  return (Determinant(a, b) > 0 ? 1 : -1) * AngleBetweenVectors(a, b);
}

// When traveling from "p1" to "p2", the angle to change directions at "p2" to
// travel towards "p3".
//
// Formal: Computes the counter-clockwise angle in radians normalized to
// [-pi,pi] between the vectors:
//   1. Starting at p2 and continuing the direction from p1 to p2,
//        i.e. (p2, p2 + (p1,p2))
//   2. (p2,p3)
inline float TurnAngle(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
  auto v_in = p2 - p1;
  auto v_out = p3 - p2;
  auto ang_in = VectorAngle(v_in);
  auto ang_out = VectorAngle(v_out);

  auto ang_in_out = NormalizeAngle(ang_out - ang_in);
  return ang_in_out;
}

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_VECTOR_UTILS_H_
