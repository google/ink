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

#ifndef INK_GEOMETRY_INTERNAL_OUTLINE_PROCESSING_H_
#define INK_GEOMETRY_INTERNAL_OUTLINE_PROCESSING_H_

#include <cstdint>
#include <vector>

#include "absl/types/span.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/triangle.h"

namespace ink::geometry_internal {

// This file provides some utilities for geometry processing with monotone
// chains.
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
// polygon's oriented boundary into a minimal set of oriented monotone
// chains, and is unique up to vertical boundary segments.

class MonotoneChain {
 public:
  MonotoneChain(std::vector<Point> vertices, int orientation);

  MonotoneChain(const MonotoneChain&) = default;
  MonotoneChain(MonotoneChain&&) = default;
  MonotoneChain& operator=(const MonotoneChain&) = default;
  MonotoneChain& operator=(MonotoneChain&&) = default;

  absl::Span<const Point> Vertices() const { return vertices_; }
  int Orientation() const { return orientation_; }
  const Rect& Bounds() const { return bounds_; }

  // Returns the first (with respect to the chain's orientation) vertex of the
  // chain.
  Point StartPoint() const;

  // Returns the last vertex of the chain.
  Point EndPoint() const;

  // Returns the point adjacent to the start point (the second vertex).
  Point StartNeighbor() const;

  // Returns the point adjacent to the end point (the second-to-last vertex).
  Point EndNeighbor() const;

  bool operator==(const MonotoneChain& other) const {
    return vertices_ == other.vertices_ && orientation_ == other.orientation_;
  }

 private:
  // The vertices of the chain, ordered by x-coordinate and then by
  // y-coordinate.
  std::vector<Point> vertices_;

  // Orientation (+1/-1) of the chain.
  int orientation_;

  Rect bounds_;
};

class ShapeOutline {
 public:
  ShapeOutline() = default;

  explicit ShapeOutline(std::vector<MonotoneChain> input_chains);

  // Computes the `ShapeOutline` (monotone boundary chain representation) for
  // the polygon defined by the given closed boundary `loops`.
  explicit ShapeOutline(const std::vector<std::vector<Point>>& loops);

  // Computes the `ShapeOutline` for the given (counter-clockwise oriented)
  // triangle.
  explicit ShapeOutline(const Triangle& triangle);

  ShapeOutline(const ShapeOutline&) = default;
  ShapeOutline(ShapeOutline&&) = default;
  ShapeOutline& operator=(const ShapeOutline&) = default;
  ShapeOutline& operator=(ShapeOutline&&) = default;

  absl::Span<const MonotoneChain> Chains() const { return chains_; }

  // Index of the predecessor chain to the given chain in an oriented boundary
  // walk.
  uint32_t PrevIndex(uint32_t chain_idx) const {
    return prev_index_[chain_idx];
  }

  // Index of the successor chain to the given chain in an oriented boundary
  // walk.
  uint32_t NextIndex(uint32_t chain_idx) const {
    return next_index_[chain_idx];
  }

  bool operator==(const ShapeOutline& other) const = default;

 private:
  // The list of chains, ordered by their lower y-bounds.
  std::vector<MonotoneChain> chains_;

  // The indices of the predecessor and successor of each chain in an oriented
  // walk along the boundary.
  std::vector<uint32_t> prev_index_;
  std::vector<uint32_t> next_index_;
};

// Intersection of a shape with a point.
bool Intersects(const ShapeOutline& shape, const Point& p);

// Intersection of a shape with an axis-aligned rectangle.
bool Intersects(const ShapeOutline& shape, const Rect& rect);

// Reconstructs the closed polygonal boundary loops of the given shape from its
// monotone chains.
std::vector<std::vector<Point>> ComputeBoundaryLoops(const ShapeOutline& shape);

// Returns a triangulation of the given shape, as a pair `{vertices,
// triangles}`, where `triangles` is a list of triples of indices into
// `vertices`.
std::pair<std::vector<Point>, std::vector<std::array<uint32_t, 3>>>
ComputeTriangulation(const ShapeOutline& shape);

// Computes the boolean difference (shape_a - shape_b) of two shapes.
ShapeOutline ComputeSubtraction(const ShapeOutline& shape_a,
                                const ShapeOutline& shape_b);

}  // namespace ink::geometry_internal

#endif  // INK_GEOMETRY_INTERNAL_OUTLINE_PROCESSING_H_
