// Copyright 2026 Google LLC
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

#ifndef INK_GEOMETRY_OUTLINE_PROCESSING_H_
#define INK_GEOMETRY_OUTLINE_PROCESSING_H_

#include <vector>

#include "ink/geometry/point.h"

namespace ink {

// Given a polygon (or set of polygons) defined by a set of closed polygonal
// chains, returns a monotone chain boundary representation of the polygon.
//
// A polygonal chain (or polyline) is defined by a sequence of points as the set
// of line segments connecting consecutive points. A closed chain is one that
// includes the closing segment from the last and first point. A set of
// closed chains defines a polygon, as the points in the plane with strictly
// positive total winding number.
//
// A monotone (or more precisely, x-monotone) chain is a polygonal chain made up
// of points with either (weakly) increasing or decreasing x-coordinate. A
// monotone chain boundary representation of a polygon is a decomposition of the
// polygon's boundary into a set of monotone chains. This function returns a
// decomposition of minimal cardinality, which is unique (up to vertical
// boundary segments).
//
// The returned chains are oriented according to the counter-clockwise
// convention, so that the interior of the polygon locally lies to the left of
// each boundary path. The order of the returned chains is arbitrary.
std::vector<std::vector<Point>> ComputeMonotoneBoundaryChains(
    const std::vector<std::vector<Point>>& loops);

}  // namespace ink

#endif  // INK_GEOMETRY_OUTLINE_PROCESSING_H_
