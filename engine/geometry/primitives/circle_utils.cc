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

#include "ink/engine/geometry/primitives/circle_utils.h"

#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/math_defines.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace {

// Compute angle from parallel of external bitangent lines for two circles.
//
// Given two circles, compute the angle between the external tangent lines
// and the line connecting the centers of the two circles.
// Sign corresponds to angle of left line (viewed from first circle),
// in the usual counterclockwise direction;
// angle of the right line has the same magnitude and opposite sign.
//
// If they have equal radius, the bitangent lines are parallel to the line;
// if the second circle is bigger, the lines diverge and the angle is positive;
// if the second circle is smaller, the lines converge and the angle is
// negative.
//
// This allows one to very easily compute the tangency points,
// and also allows one to draw arcs of the circle up to the tangent point,
// so you can draw piecewise curves composed of the circles and the tangent
// lines, such as for forecast cones.
//
// External lines exist if and only if one circle is not contained within the
// other (and are not equal).
// There's a single line at the tangent point if the circles are internally
// tangent, which corresponds to a right angle.
//
// See "Tangent lines to two circles" at Wikipedia for further information:
// http://en.wikipedia.org/wiki/Tangent_lines_to_two_circles
//
// Code ported by grabks@ from
// keyhole/dataeng/vector/ocean/hurricanes/draw_forecast.py
inline bool ExternalBitangentAngle(glm::vec2 center1, float radius1,
                                   glm::vec2 center2, float radius2,
                                   float *out) {
  EXPECT(out);

  float dr = radius2 - radius1;

  // (dx, dy) is vector between centers, d is its length
  float d = glm::distance(center1, center2);
  if (d == 0 || d < std::abs(dr)) {
    // Special position
    // Circles coincide or one circle is inside the other,
    // so no external bitangent lines
    return false;
  }

  // Algebraically, because abs(dr) <= d, dividing yields
  // -1 <= dr/d <= 1, so arcsine is well-defined:
  *out = std::asin(dr / d);
  return true;
}
}  // namespace

std::vector<glm::vec2> PointsOnCircle(glm::vec2 center, float radius,
                                      uint32_t verts, float start_ang,
                                      float end_ang) {
  std::vector<glm::vec2> pts;
  uint32_t n_verts = roundf(std::abs(end_ang - start_ang) * verts / M_TAU);
  n_verts = std::max(2u, n_verts);
  auto ang_increment = (end_ang - start_ang) / (n_verts - 1);
  pts.reserve(n_verts);
  for (uint32_t i = 0; i < n_verts; i++) {
    auto ang = start_ang + i * ang_increment;
    auto pt = PointOnCircle(ang, radius, center);
    pts.push_back(pt);
  }
  return pts;
}

bool CommonTangents(glm::vec2 center1, float radius1, glm::vec2 center2,
                    float radius2, CircleTangents *out, float tolerance) {
  EXPECT(out);

  float reference_angle = VectorAngle(center2 - center1);
  float theta;
  if (!ExternalBitangentAngle(center1, radius1, center2, radius2, &theta)) {
    return false;
  }
  float delta = M_PI_2 + theta;
  // When delta is 0, this means that we're in a degenerate case where we wind
  // up with only one tangent. We don't really want to consider this case, and
  // would rather throw away this answer since it will be roughly nonsensical.
  // Unfortunately, we can't directly compare to zero, so we need some sort of
  // tolerance.
  // The current default value of tolerance (0.000001) was determined by running
  // a fairly trivial test of circles at (0, 0, 100) and (50, 0, 50). The value
  // of delta for that case was -0.00000004371139006309476826572791, so we will
  // use some value relatively close to that as our tolerance.
  float abs_delta = std::abs(delta);
  if (abs_delta < tolerance || std::abs(abs_delta - M_PI) < tolerance) {
    return false;
  }

  // if circles are on x-axis and bitangent lines parallel to line of centers,
  // then angle of bitangent points are 90 degrees and 270 degrees.
  // Starting with these, we add two adjustments:
  // * reference_angle for the relative position of the circles,
  // * theta for the angle the bitangent lines form
  float left_angle = reference_angle + delta;
  float right_angle = reference_angle - delta;
  out->left = Segment(PointOnCircle(left_angle, radius1, center1),
                      PointOnCircle(left_angle, radius2, center2));
  out->right = Segment(PointOnCircle(right_angle, radius1, center1),
                       PointOnCircle(right_angle, radius2, center2));
  return true;
}

}  // namespace ink
