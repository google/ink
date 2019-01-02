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

#include "ink/engine/geometry/algorithms/distance.h"

#include <algorithm>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/intersect.h"

namespace ink {
namespace geometry {

float Distance(const Segment &seg, glm::vec2 point) {
  if (seg.from == seg.to) return glm::distance(seg.from, point);

  glm::vec2 v = seg.to - seg.from;
  glm::vec2 u = point - seg.from;
  float v_length_squared = glm::dot(v, v);

  // The segment length parameter at which the closest point lies. Recall that
  // dot(v, u) = length(v) * length(u) * cos(theta), where theta is the angle
  // between v and u.
  float t = glm::dot(v, u) / v_length_squared;
  if (t < 0) {
    return Distance(point, seg.from);
  } else if (t > 1) {
    return Distance(point, seg.to);
  } else {
    // Recall that determinant(v, u) = length(v) * length(u) * sin(theta), where
    // theta is the signed angle between v and u.
    float determinant = v.x * u.y - v.y * u.x;
    return std::abs(determinant) / std::sqrt(v_length_squared);
  }
}

float Distance(const std::vector<glm::vec2> &polyline, glm::vec2 point) {
  float min_distance = std::numeric_limits<float>::infinity();
  for (int i = 0; i < polyline.size() - 1; ++i) {
    min_distance = std::min(
        min_distance, Distance(point, Segment(polyline[i], polyline[i + 1])));
  }
  return min_distance;
}

float Distance(const Rect& a, const Rect& b) {
  Rect intersection;
  if (Intersection(a, b, &intersection)) {
    return -std::sqrt(intersection.Area());
  }
  return glm::length(glm::vec2{
      std::max(0.0f, std::max(a.Left() - b.Right(), b.Left() - a.Right())),
      std::max(0.0f, std::max(a.Bottom() - b.Top(), b.Bottom() - a.Top()))});
}

}  // namespace geometry
}  // namespace ink
