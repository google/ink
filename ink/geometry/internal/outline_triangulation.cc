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

#include "absl/algorithm/container.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/point.h"

namespace ink::geometry_internal {
namespace {

// `Triangulate` implements a standard two-stage approach for triangulating a
// polygon by first splitting up the polygon into monotone pieces
// and then triangulating each piece efficiently. (Here a polygon is
// monotone if every vertical line intersects it in at most a single segment.)
// See https://en.wikipedia.org/wiki/Polygon_triangulation and the references
// therein for further background.
//
// To form the monotone partition of the polygon, we sweep the polygon from left
// to right, growing monotone pieces and cutting them off with diagonals
// wherever a concavity or hole opens or closes.
//
// More precisely, as the sweep line advances, its intersection with the shape's
// interior is a sequence of intervals (`Region`s), each bounded below by a
// positive chain and above by a negative chain. Each region is locally a
// monotone piece, with topological changes happening at chain endpoints
// (`Event`s of the sweep). What happens depends on the local geometry
// (`EventType`): Start (e.g. point A below), End (D), Split (G), Merge (I).
//
//         F-----------E
//        /      J      \
//       /      / \      \
//      A      G   I      D
//       \      \ /      /
//        \      H      /
//         B-----------C
//
// A polygon is monotone iff it has no Split or Merge events. When we encounter
// a Split or Merge event, we cut off a new piece by cutting along a diagonal
// (e.g. GF in the example above). The tricky case is Merge, where the diagonal
// to cut (e.g. IE or IC) hasn't yet been encountered on the sweep; we therefore
// merge the regions and hold on to the previous chains (`pending_piece`,
// `HasPendingMerge()`) until we find the diagonal.

// Event types.
enum class EventType { kEnd, kMerge, kStart, kSplit };

// A sweep event, corresponding to the common endpoint of a positive and
// negative chain of the shape.
struct Event {
  Point point;

  // Indices of the positive and negative chains in `ShapeOutline::Chains()`.
  uint32_t pos_chain;
  uint32_t neg_chain;

  EventType type;
};

// An in-progress monotone piece of the shape, given by the lower and upper
// boundaries.
struct Piece {
  // Indices of the boundary vertices in `Triangulation::vertices`.
  absl::InlinedVector<uint32_t, 8> lower;
  absl::InlinedVector<uint32_t, 8> upper;
};

// An active interval of the shape's interior along the sweep line.
struct Region {
  // Indices of the bounding chains in `ShapeOutline::Chains()`.
  uint32_t pos_chain;
  uint32_t neg_chain;

  // Indices of the next vertices to add in `pos_chain` and `neg_chain`.
  uint32_t pos_next = 1;
  uint32_t neg_next = 1;

  // The monotone piece currently being built by this region.
  Piece piece;

  // `pending_piece` is non-empty only while a merge is pending resolution, in
  // which case `piece` is the piece below the merge vertex and `pending_piece`
  // is the piece above the merge vertex.
  Piece pending_piece;

  bool HasPendingMerge() const { return !pending_piece.lower.empty(); }
};

// Computes the 2D cross product (determinant) of the vectors ab and ac, which
// is positive iff (a, b, c) is counter-clockwise.
double Determinant(const Point& a, const Point& b, const Point& c) {
  return (double{b.x} - a.x) * (double{c.y} - a.y) -
         (double{b.y} - a.y) * (double{c.x} - a.x);
}

// Returns whether `chain_a` starts below `chain_b`.
bool IsBelow(const MonotoneChain& chain_a, const MonotoneChain& chain_b) {
  Point p = chain_a.Vertices()[0];
  absl::Span<const Point> b_vertices = chain_b.Vertices();
  auto w = absl::c_upper_bound(b_vertices, p, IsLeftOrBelow);
  Point test_pt = (p == w[-1]) ? chain_a.Vertices()[1] : p;
  return Determinant(w[-1], *w, test_pt) <= 0;
}

// Triangulates a monotone polygon with boundaries `piece.lower` and
// `piece.upper` (with a common start/leftmost vertex) and rightmost vertex
// `end_vertex`, and appends the resulting triangles to
// `triangulation.triangles`. Note that all vertex indices (in `piece` and
// `end_vertex`) are indices into `triangulation.vertices`.
void TriangulateMonotonePolygon(const Piece& piece, uint32_t end_vertex,
                                Triangulation& triangulation) {
  // This implements a standard sweep-line algorithm for triangulating a
  // monotone polygon in linear time. The basic idea of the algorithm is as
  // follows. Consider the first three points `a`, `b`, and `c` encountered when
  // sweeping from left-to-right. There are a few cases that occur:
  // - If `b` and `c` are on opposite chains, `b` can see `c` through the
  //   polygon and so `abc` is a valid triangle. If `b` and `c` are on the
  //   same chain and `b` is convex (so that `c` can see `a`) then
  //   `abc` is again a valid triangle. In both cases, we cut off `abc`, and
  //   proceed inductively with the remaining polygon, which is again monotone.
  // - If `b` and `c` are on the same chain and `b` is concave (reflex) point,
  //   then we hold on to `a`, `b`, and `c` on a stack and continue sweeping,
  //   adding the encountered points to the stack until we encounter a point
  //   that can make a valid triangle with the top of the stack.

  absl::Span<const uint32_t> lower = piece.lower;
  absl::Span<const uint32_t> upper = piece.upper;
  absl::Span<const Point> points = triangulation.vertices;
  std::vector<std::array<uint32_t, 3>>& triangles = triangulation.triangles;

  // Appends (a, b, c) in counter-clockwise orientation.
  auto add_triangle = [&](uint32_t a, uint32_t b, uint32_t c) {
    double det = Determinant(points[a], points[b], points[c]);
    if (det > 0) triangles.push_back({a, b, c});
    if (det < 0) triangles.push_back({a, c, b});
  };

  // The stack of reflex points. Note that by geometry the stack will always
  // contain points of one chain. `prev_side` indicates which side this is:
  // +1 for lower, -1 for upper, and 0 for common.
  absl::InlinedVector<uint32_t, 16> stack = {lower.front()};
  int prev_side = 0;

  // Index of the next vertex on each chain.
  size_t i = 1, j = 1;

  while (i < lower.size() || j < upper.size()) {
    ABSL_DCHECK_LE(i, lower.size());
    ABSL_DCHECK_LE(j, upper.size());

    // Advance the sweep line to the next vertex.
    uint32_t v;
    int side;
    if (i < lower.size() && j < upper.size() && lower[i] == upper[j]) {
      // The chains meet at a pinch point.
      side = 0;
      v = lower[i];
      ++i;
      ++j;
    } else if (j == upper.size() ||
               (i < lower.size() &&
                IsLeftOrBelow(points[lower[i]], points[upper[j]]))) {
      side = 1;
      v = lower[i++];
    } else {
      side = -1;
      v = upper[j++];
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

  ABSL_DCHECK_EQ(i, lower.size());
  ABSL_DCHECK_EQ(j, upper.size());
  for (size_t s = stack.size() - 1; s > 0; --s) {
    add_triangle(end_vertex, stack[s], stack[s - 1]);
  }
}

// Appends `v` to `list` unless it is already at the back.
void AddVertex(absl::InlinedVector<uint32_t, 8>& list, uint32_t v) {
  if (list.empty() || list.back() != v) list.push_back(v);
}

// Helper function that appends `v` to `region`'s lower boundary, first
// completing the lower piece if a merge is pending.
void AddPosVertex(Region& region, uint32_t v, Triangulation& triangulation) {
  if (region.HasPendingMerge()) {
    TriangulateMonotonePolygon(region.piece, v, triangulation);
    region.piece = std::exchange(region.pending_piece, {});
  }
  AddVertex(region.piece.lower, v);
}

// Helper function that appends `v` to `region`'s upper boundary, first
// completing the upper piece if a merge is pending.
void AddNegVertex(Region& region, uint32_t v, Triangulation& triangulation) {
  if (region.HasPendingMerge()) {
    TriangulateMonotonePolygon(region.pending_piece, v, triangulation);
    region.pending_piece = {};
  }
  AddVertex(region.piece.upper, v);
}

// Advances `region` along `pos_chain` and `neg_chain` in sweep order up to
// `event_vertex`, excluding each chain's final endpoint.
void AddChainVertices(Region& region, uint32_t event_vertex,
                      absl::Span<const MonotoneChain> chains,
                      Triangulation& triangulation) {
  std::vector<Point>& points = triangulation.vertices;
  Point end = points[event_vertex];

  absl::Span<const Point> pos = chains[region.pos_chain].Vertices();
  absl::Span<const Point> neg = chains[region.neg_chain].Vertices();
  uint32_t& i = region.pos_next;
  uint32_t& j = region.neg_next;

  while (true) {
    bool pos_more = i + 1 < pos.size() && !IsLeftOrBelow(end, pos[i]);
    bool neg_more = j + 1 < neg.size() && !IsLeftOrBelow(end, neg[j]);
    if (!pos_more && !neg_more) return;

    bool from_pos = pos_more && (!neg_more || IsLeftOrBelow(pos[i], neg[j]));
    Point p = from_pos ? pos[i++] : neg[j++];

    // Reuse `event_vertex` if `p` coincides with `end`, or `points.back()` if
    // both chains meet at a pinch point.
    uint32_t vertex = event_vertex;
    if (p != end) {
      if (p != points.back()) points.push_back(p);
      vertex = points.size() - 1;
    }
    if (from_pos) {
      AddPosVertex(region, vertex, triangulation);
    } else {
      AddNegVertex(region, vertex, triangulation);
    }
  }
}

// Splits `region` at a split `event` (with vertex id `event_vertex`) into
// `{below, above}`, adding a diagonal from `event_vertex` to the last vertex
// added to `region`.
std::pair<Region, Region> SplitRegion(Region region, const Event& event,
                                      uint32_t event_vertex,
                                      absl::Span<const MonotoneChain> chains,
                                      Triangulation& triangulation) {
  ABSL_DCHECK_EQ(event.type, EventType::kSplit);

  AddChainVertices(region, event_vertex, chains, triangulation);

  Region below = {.pos_chain = region.pos_chain,
                  .neg_chain = event.neg_chain,
                  .pos_next = region.pos_next};
  Region above = {.pos_chain = event.pos_chain,
                  .neg_chain = region.neg_chain,
                  .neg_next = region.neg_next};

  if (region.HasPendingMerge()) {
    // The diagonal from the merge vertex to `event_vertex` separates the two
    // pending pieces into `below` and `above`.
    below.piece = std::move(region.piece);
    above.piece = std::move(region.pending_piece);
  } else {
    // The diagonal connects `event_vertex` to whichever boundary's last vertex
    // is further along in sweep order, starting a new piece on that side.
    uint32_t pos_last = region.piece.lower.back();
    uint32_t neg_last = region.piece.upper.back();
    if (IsLeftOrBelow(triangulation.vertices[neg_last],
                      triangulation.vertices[pos_last])) {
      below.piece = {{pos_last}, {pos_last}};
      above.piece = std::move(region.piece);
    } else {
      below.piece = std::move(region.piece);
      above.piece = {{neg_last}, {neg_last}};
    }
  }
  AddVertex(below.piece.upper, event_vertex);
  AddVertex(above.piece.lower, event_vertex);
  return {std::move(below), std::move(above)};
}

// Merges `top` into the region `bottom` directly below it at the merge vertex
// `event_vertex`, keeping both pieces in `bottom` until its next vertex.
void MergeRegions(Region& bottom, Region& top, uint32_t event_vertex,
                  absl::Span<const MonotoneChain> chains,
                  Triangulation& triangulation) {
  AddChainVertices(bottom, event_vertex, chains, triangulation);
  AddChainVertices(top, event_vertex, chains, triangulation);
  AddNegVertex(bottom, event_vertex, triangulation);
  AddPosVertex(top, event_vertex, triangulation);

  bottom.neg_chain = top.neg_chain;
  bottom.neg_next = top.neg_next;
  bottom.pending_piece = std::move(top.piece);
}

// Returns the sweep events at chain endpoints, sorted in sweep order and
// tie-broken by `EventType`.
absl::InlinedVector<Event, 8> GetEvents(const ShapeOutline& shape) {
  absl::Span<const MonotoneChain> chains = shape.Chains();
  absl::InlinedVector<Event, 8> events;
  events.reserve(chains.size());

  for (uint32_t idx = 0; idx < chains.size(); ++idx) {
    const MonotoneChain& chain = chains[idx];
    // Chain `idx` ends where `next_idx` starts (+1 chains end on the right;
    // -1 chains end on the left). The shared vertex is convex at a start or
    // end, and reflex at a split or merge.
    uint32_t next_idx = shape.NextIndex(idx);
    Point point = chain.EndPoint();
    bool convex = Determinant(chain.EndNeighbor(), point,
                              chains[next_idx].StartNeighbor()) > 0;
    if (chain.Orientation() == 1) {
      if (convex) {
        events.push_back({point, idx, next_idx, EventType::kEnd});
      } else {
        events.push_back({point, idx, next_idx, EventType::kMerge});
      }
    } else {
      if (convex) {
        events.push_back({point, next_idx, idx, EventType::kStart});
      } else {
        events.push_back({point, next_idx, idx, EventType::kSplit});
      }
    }
  }

  absl::c_sort(events, [](const Event& e1, const Event& e2) {
    if (e1.point != e2.point) return IsLeftOrBelow(e1.point, e2.point);
    return e1.type < e2.type;
  });
  return events;
}

}  // namespace

Triangulation Triangulate(const ShapeOutline& shape) {
  // See the overview comment at the top of the file.

  absl::Span<const MonotoneChain> chains = shape.Chains();
  Triangulation triangulation;
  std::vector<Point>& points = triangulation.vertices;
  absl::InlinedVector<Region, 4> regions;

  uint32_t event_vertex = 0;
  for (const Event& event : GetEvents(shape)) {
    // Explicitly add the event point to the triangulation (chain endpoints are
    // skipped when the chain is added).
    if (points.empty() || points[event_vertex] != event.point) {
      event_vertex = points.size();
      points.push_back(event.point);
    }

    if (event.type == EventType::kStart || event.type == EventType::kSplit) {
      // Find the lowest region whose upper chain lies above `event.point`.
      uint32_t idx = 0;
      while (idx < regions.size() && !IsBelow(chains[event.pos_chain],
                                              chains[regions[idx].neg_chain])) {
        ++idx;
      }

      if (event.type == EventType::kStart) {
        regions.insert(regions.begin() + idx,
                       Region{.pos_chain = event.pos_chain,
                              .neg_chain = event.neg_chain,
                              .piece = {{event_vertex}, {event_vertex}}});
      } else {
        auto [below, above] = SplitRegion(std::move(regions[idx]), event,
                                          event_vertex, chains, triangulation);
        regions[idx] = std::move(below);
        regions.insert(regions.begin() + idx + 1, std::move(above));
      }
    } else {
      // Find the region whose lower chain ends at this event.
      uint32_t idx = 0;
      while (regions[idx].pos_chain != event.pos_chain) ++idx;
      Region& region = regions[idx];

      if (event.type == EventType::kEnd) {
        AddChainVertices(region, event_vertex, chains, triangulation);
        TriangulateMonotonePolygon(region.piece, event_vertex, triangulation);
        if (region.HasPendingMerge()) {
          TriangulateMonotonePolygon(region.pending_piece, event_vertex,
                                     triangulation);
        }
      } else {
        MergeRegions(regions[idx - 1], region, event_vertex, chains,
                     triangulation);
      }
      regions.erase(regions.begin() + idx);
    }
  }
  return triangulation;
}

}  // namespace ink::geometry_internal
