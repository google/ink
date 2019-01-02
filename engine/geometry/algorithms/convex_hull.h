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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_CONVEX_HULL_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_CONVEX_HULL_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"

namespace ink {
namespace geometry {

// Computes the convex hull of the given points, using Graham's algorithm
// (go/wiki/Graham_scan). The first point in the returned polygon will be the
// one with the lowest y-coordinate, with the lowest x-coordinate breaking ties.
// From there, the it continues in counter-clockwise order. Coincident points
// are removed, and colinear segments are joined.
//
// Note: There are algorithms that are theoretically faster. Given N inputs,
// Graham's algorithm runs in O(NlogN). There are algorithms that run in
// O(NlogH), where H is the number of points in the result. However, for the
// input sizes that we're expecting, this is unlikely to make much of a
// difference.
std::vector<glm::vec2> ConvexHull(const std::vector<glm::vec2> &points);

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_CONVEX_HULL_H_
