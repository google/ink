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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_DISTANCE_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_DISTANCE_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/segment.h"

namespace ink {
namespace geometry {

inline float Distance(float a, float b) { return std::abs(a - b); }
inline float Distance(glm::vec2 point1, glm::vec2 point2) {
  return glm::distance(point1, point2);
}
inline float Distance(glm::dvec2 point1, glm::dvec2 point2) {
  return glm::distance(point1, point2);
}
inline float Distance(const Vertex &point1, const Vertex &point2) {
  return Distance(point1.position, point2.position);
}
// If a,b don't overlap:
//   Returns the distance between the closest points on a,b.
// If a,b overlap:
//   Returns the negative sqrt of the area of the intersection.
float Distance(const Rect &a, const Rect &b);

// Distance from the given point to the nearest point on the given segment.
float Distance(const Segment &seg, glm::vec2 point);
inline float Distance(glm::vec2 point, const Segment &seg) {
  return Distance(seg, point);
}

// Distance from the given point to the nearest point on the given polyline.
float Distance(const std::vector<glm::vec2> &polyline, glm::vec2 point);
inline float Distance(glm::vec2 point, const std::vector<glm::vec2> &polyline) {
  return Distance(polyline, point);
}

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_DISTANCE_H_
