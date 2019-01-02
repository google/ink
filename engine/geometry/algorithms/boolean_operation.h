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

#ifndef INK_ENGINE_GEOMETRY_ALGORITHMS_BOOLEAN_OPERATION_H_
#define INK_ENGINE_GEOMETRY_ALGORITHMS_BOOLEAN_OPERATION_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/polygon.h"

namespace ink {
namespace geometry {

// These functions perform boolean operations on polygons, using a variation of
// the Weiler-Atherton Algorithm. The input polygons must be oriented
// counter-clockwise, and must not contain self-intersections.
// (see go/wiki/Weiler%e2%80%93Atherton_clipping_algorithm)
//
// If either polygon consists of less than three vertices, an empty list will be
// returned.

std::vector<Polygon> Intersection(const Polygon &lhs_polygon,
                                  const Polygon &rhs_polygon);
std::vector<Polygon> Difference(const Polygon &base_polygon,
                                const Polygon &cutting_polygon);

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_ALGORITHMS_BOOLEAN_OPERATION_H_
