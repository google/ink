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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_POLYGON_UTILS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_POLYGON_UTILS_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {
namespace geometry {

// A polygon, defined by an ordered list of points. This is a general polygon,
// i.e. one that makes no guarantees with regards to convexity, regularity,
// self-intersections etc.
class Polygon {
 public:
  Polygon() {}
  explicit Polygon(std::vector<glm::vec2> points)
      : points_(std::move(points)) {}

  // Returns the segment that starts a point idx and ends at point
  // (idx + 1) % Size().
  Segment GetSegment(std::size_t idx) const {
    ASSERT(idx >= 0 && idx < Size());
    return {points_[idx], points_[(idx + 1) % Size()]};
  }

  // Removes sequential points that at the same location, returning the number
  // of points removed.
  // E.g.: (4, 4), (4, 4), (3, 5), (4, 4), (6, 10), (6, 10), (6, 10), (7, 6)
  // becomes (4, 4), (3, 5), (4, 4), (6, 10), (7, 6), and returns 3.
  int RemoveDuplicatePoints();

  // Find the winding number of the polygon around the given point. Positive
  // indicates a counter-clockwise wind, negative indicates clockwise. Note that
  // the polygon may contain self-intersections, which may result in it winding
  // around a point multiple times.
  //
  // The winding number can be used to test whether the point lies inside the
  // polygon (see go/wiki/Winding_number and go/wiki/Point_in_polygon).
  int WindingNumber(glm::vec2 point) const;

  // Returns the signed area of the polygon. A counter-clockwise simple (i.e.
  // non-self-intersecting) polygon will have a positive area, while a clockwise
  // simple polygon will have a negative area.
  float SignedArea() const;

  // Returns a copy of the polygon with the order of the points reversed.
  Polygon Reversed() const {
    auto points_copy = points_;
    std::reverse(points_copy.begin(), points_copy.end());
    return Polygon(std::move(points_copy));
  }

  // Returns a copy of the polygon that has been circular shifted
  // (go/wiki/Circular_shift) by the given amount, i.e. the Nth value of the
  // returned polygon is the ((N + amount) % Size())th value of the original.
  Polygon CircularShift(int amount) const {
    amount %= static_cast<int>(Size());
    if (amount < 0) amount += Size();
    auto points_copy = points_;
    std::rotate(points_copy.begin(), points_copy.begin() + amount,
                points_copy.end());
    return Polygon(points_copy);
  }

  std::size_t Size() const { return points_.size(); }
  bool Empty() const { return points_.empty(); }
  glm::vec2 &operator[](std::size_t idx) { return points_[idx]; }
  const glm::vec2 &operator[](std::size_t idx) const { return points_[idx]; }

  const std::vector<glm::vec2> &Points() const { return points_; }

  std::string ToString() const { return Substitute("<Polygon $0>", points_); }

 private:
  std::vector<glm::vec2> points_;
};

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_POLYGON_UTILS_H_
