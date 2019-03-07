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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_INTERSECT_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_INTERSECT_H_

#include <array>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/geometry/primitives/triangle.h"

namespace ink {
namespace geometry {

// These functions return whether the given geometry objects intersect, and
// populate the output with information about the intersections (this will vary
// by the types of objects).

struct SegmentIntersection {
  // The length ratio parameter intervals over which the segments are coincident
  // (see Segment::Eval()). The intervals will be ordered with respect to
  // segment1, i.e. segment1_interval[0] corresponds to segment2_interval[0],
  // and segment1_interval[0] will be less than or equal to
  // segment1_interval[1].
  // If the segments intersect at a single point, the first and second element
  // of each interval will be equal.
  std::array<float, 2> segment1_interval{{-1, -1}};
  std::array<float, 2> segment2_interval{{-1, -1}};

  // The coincident portion of the segments. This may be a degenerate segment.
  Segment intx;

  std::string ToString() const {
    return Substitute("$0, $1, $2", segment1_interval, segment2_interval, intx);
  }
};
bool Intersection(const Segment &segment1, const Segment &segment2,
                  SegmentIntersection *output);

// Convenience function to get the intersection position. If the segments are
// parallel and overlapping, output will be populated with the intersection
// position closest to segment1's start point (which may, in fact, be segment1's
// start point).
bool Intersection(const Segment &segment1, const Segment &segment2,
                  glm::vec2 *output);

bool Intersection(const Rect &rect1, const Rect &rect2, Rect *output);
// Returns true if the segment intersects the rectangle. Note that this will
// reutrn true if the segment is fully contained in the rectangle. Stores the
// intersection of the segment and rectangle in *output.
bool Intersection(const Segment &segment, const Rect &rect, Segment *output);

struct PolygonIntersection {
  // The indices of the segments at which the intersection occurs. Given a
  // polygon P with N vertices, index I refers to the segment from P[I]
  // to P[(I + 1) % N].
  std::array<int, 2> indices{{-1, -1}};

  // The segment intersection data.
  SegmentIntersection intersection;

  std::string ToString() const {
    return Substitute("$0: $1", indices, intersection);
  }
};
// Finds all intersections of a segment in polygon1 and a segment in polygon2.
// Note that the result may contain duplicates, because the end of each segment
// is the start of the next one. If an intersection occurs where one of the
// polygons has a self-intersection, the result may also contain intersections
// that are coincident -- these, however, are not true duplicates, as they occur
// at different lengths along the polygon.
// The output vector is expected be empty before this is called.
// Note: This performs an exhaustive search of the segment pairs. Given polygons
// with N and M vertices, respectively, the time complexity is O(N*M). There are
// more efficient algorithms we could use, but, currently, we expect to be
// operating on very small polygons, e.g. 5 or fewer vertices.
bool Intersection(const Polygon &polygon1, const Polygon &polygon2,
                  std::vector<PolygonIntersection> *output);

// These functions return whether the objects intersect, but don't include the
// information about the intersections. In many cases, this will be more
// efficient than the functions above.

bool Intersects(const Segment &segment1, const Segment &segment2);
bool Intersects(const Triangle &triangle1, const Triangle &triangle2);
bool Intersects(const Rect &rect1, const Rect &rect2);
bool Intersects(const RotRect &rect1, const RotRect &rect2);
// Returns true if the segment intersects the rectangle. Note that this will
// reutrn true if the segment is fully contained in the rectangle.
bool Intersects(const Segment &segment, const Rect &rect);

// The above "Intersects" definition between rectangles returns true for cases
// where the resulting intersection is a line segment or a point, having 0 area.
// This function returns true only for intersections with non-0 area.
bool IntersectsWithNonZeroOverlap(const Rect &rect1, const Rect &rect2);

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_INTERSECT_H_
