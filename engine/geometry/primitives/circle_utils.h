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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_CIRCLE_UTILS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_CIRCLE_UTILS_H_

#include <cmath>
#include <cstdint>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/segment.h"

namespace ink {

struct CircleTangents {
  Segment left;
  Segment right;
};

// The point at distance "r" and angle "theta" (in radians ccw) from "center".
inline glm::vec2 PointOnCircle(float theta, float r, glm::vec2 center) {
  return center + r * glm::vec2(std::cos(theta), std::sin(theta));
}

// Generates a list of points in the shape of an arc from startAng to endAng. If
// endAng < startAng, the arc will be counter-clockwise, otherwise it will be
// clockwise. The method guarantees that at least two points will be returned.
// The number of points returned is effectively verts * (arc.circumference /
// circle.circumference).
// This method has undefined behavior when abs(endAng - startAng) > 2 * PI.
//
// See also: "MakeArc"
std::vector<glm::vec2> PointsOnCircle(glm::vec2 center, float radius,
                                      uint32_t verts, float start_ang,
                                      float end_ang);

// Uses fixAngles to emulate HTML5 canvas arc behavior, where there is an
// explicit makeClockwise parameter that can be used to control the direction of
// the arc.
inline std::vector<glm::vec2> MakeArc(glm::vec2 center, float radius,
                                      uint32_t verts, float start_ang,
                                      float end_ang, bool make_clockwise) {
  FixAngles(&start_ang, &end_ang, make_clockwise);
  return PointsOnCircle(center, radius, verts, start_ang, end_ang);
}

// Compute points of external bitangent lines for two circles.
//
// Compute tangent points of two circles with their
// two external bitangent lines (lines tangent to both circles),
// if these exist (otherwise None).
//
// Order is left line (first circle, second circle),
//          right line (first circle, second circle)
// when viewed from the first circle.
//
// See "Tangent lines to two circles" at Wikipedia for further information:
// http://en.wikipedia.org/wiki/Tangent_lines_to_two_circles
//
// Code ported by grabks@ from
// keyhole/dataeng/vector/ocean/hurricanes/draw_forecast.py
bool CommonTangents(glm::vec2 center1, float radius1, glm::vec2 center2,
                    float radius2, CircleTangents *out,
                    float tolerance = 0.000001);

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_CIRCLE_UTILS_H_
