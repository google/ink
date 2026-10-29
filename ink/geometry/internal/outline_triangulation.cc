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

#include "ink/geometry/internal/outline_triangulation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/point.h"

namespace ink::geometry_internal {
namespace {

// Computes the 2D cross product (determinant) of the vectors ab and ac, which
// is positive iff (a, b, c) is counter-clockwise.
double Determinant(const Point& a, const Point& b, const Point& c) {
  return (double{b.x} - a.x) * (double{c.y} - a.y) -
         (double{b.y} - a.y) * (double{c.x} - a.x);
}

// Triangulates a polygon bounded by the monotone `lower` and `upper` chains.
Triangulation TriangulateMonotonePolygon(absl::Span<const Point> lower,
                                         absl::Span<const Point> upper) {
  // This implements a standard sweep-line algorithm for triangulating a
  // monotone polygon in linear time. The basic idea of the algorithm is as
  // follows. Consider the first three points `a`, `b`, and `c` encountered when
  // sweeping from left-to-right. There are a few cases that occur:
  // - If `b` and `c` are on opposite chains, `b` can see `c` through the
  //   the polygon and so `abc` is a valid triangle. If `b` and `c` are on the
  //   same chain and `b` is convex (so that `c` can see `a`) then
  //   `abc` is again a valid triangle. In both cases, we cut off `abc`, and
  //   proceed inductively with the remaining polygon, which is again monotone.
  // - If `b` and `c` are on the same chain and `b` is concave (reflex) point,
  //   then we hold on to `a`, `b`, and `c` on a stack and continue sweeping,
  //   adding the encountered points to the stack until we encounter a point
  //   that can make a valid triangle with the top of the stack.

  const size_t n = lower.size() + upper.size() - 2;

  std::vector<Point> points;
  points.reserve(n);
  std::vector<std::array<uint32_t, 3>> triangles;
  triangles.reserve(n - 2);

  // Appends (a, b, c) in counter-clockwise orientation.
  auto add_triangle = [&](uint32_t a, uint32_t b, uint32_t c) {
    double det = Determinant(points[a], points[b], points[c]);
    if (det > 0) triangles.push_back({a, b, c});
    if (det < 0) triangles.push_back({a, c, b});
  };

  // The stack of reflex points. Note that by geometry the stack will always
  // contain points of one chain. `prev_side` indicates which side this is:
  // +1 for lower, -1 for upper, and 0 for common.
  absl::InlinedVector<uint32_t, 16> stack = {0};
  points.push_back(lower.front());
  int prev_side = 0;

  // Index of the next vertex on each chain.
  size_t i = 1, j = 1;

  while (i < lower.size() && j < upper.size()) {
    uint32_t v = points.size();

    // Advance the sweep line to the next vertex.

    int side;
    if (lower[i] == upper[j]) {
      // The chains meet, at the end point or at a pinch point.
      side = 0;
      points.push_back(lower[i]);
      ++i;
      ++j;
    } else if (IsLeftOrBelow(lower[i], upper[j])) {
      side = 1;
      points.push_back(lower[i++]);
    } else {
      side = -1;
      points.push_back(upper[j++]);
    }

    if (side != prev_side) {
      // Connect diagonals to all visible vertices on the stack.
      for (size_t s = stack.size() - 1; s > 0; --s) {
        add_triangle(v, stack[s], stack[s - 1]);
      }

      if (side == 0) {
        stack = {v};
      } else {
        stack = {stack.back(), v};
      }
    } else {
      // Connect diagonals for all ears.
      while (stack.size() >= 2) {
        uint32_t a = stack[stack.size() - 2], b = stack.back();
        if (side * Determinant(points[a], points[b], points[v]) <= 0) break;
        add_triangle(a, b, v);
        stack.pop_back();
      }

      stack.push_back(v);
    }
    prev_side = side;
  }

  return {std::move(points), std::move(triangles)};
}

}  // namespace

Triangulation Triangulate(const ShapeOutline& shape) {
  absl::Span<const MonotoneChain> chains = shape.Chains();

  // TODO(b/521449017): Handle general case.
  if (chains.size() != 2) return {};

  // The +1 chain runs left to right along the bottom of the polygon.
  if (chains[0].Orientation() == 1)
    return TriangulateMonotonePolygon(chains[0].Vertices(),
                                      chains[1].Vertices());
  else
    return TriangulateMonotonePolygon(chains[1].Vertices(),
                                      chains[0].Vertices());
}

}  // namespace ink::geometry_internal
