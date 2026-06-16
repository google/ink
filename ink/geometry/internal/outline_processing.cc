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

#include "ink/geometry/internal/outline_processing.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/internal/intersects_internal.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/vec.h"

namespace ink::geometry_internal {

// TODO(b/521448869): Improve numerical robustness -- it's common
// in line-sweep algorithms to quantize points to a grid to avoid precision
// instability.
// TODO(b/521449017): Handle degenerate intersections, including overlapping
// segments, T-junctions, simultaneous intersections of more than two segments,
// etc.

namespace {
// TODO(b/521448869): Use an adaptive tolerance based on the relevant scales
// and epsilons.
constexpr float kCollinearTolerance = 1e-6f;

bool IsLeftOrBelow(const Point& a, const Point& b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

// Represents a boundary segment during sweep-line processing.
struct BoundarySegment {
  // The endpoints, ordered so that start.x <= end.x.
  Point start, end;

  // Orientation (+1/-1) of the segment with respect to the order of the
  // endpoints.
  int orientation;

  // Defines a (weak) sweep-line ordering: left-to-right and vertical segments
  // before non-vertical segments.
  bool operator<(const BoundarySegment& other) const {
    if (start.x != other.start.x) return start.x < other.start.x;
    return end.x < other.end.x;
  }
};

// Returns a flattened list of all segments from the given loops, sorted
// from left-to-right based on the start-coordinate.
std::vector<BoundarySegment> GetSegments(
    const std::vector<std::vector<Point>>& loops) {
  std::vector<BoundarySegment> segments;

  size_t size = 0;
  for (const auto& loop : loops) size += loop.size();
  segments.reserve(size);

  for (const auto& outline : loops) {
    if (outline.empty()) continue;
    Point u = outline.back();
    for (const Point& v : outline) {
      segments.push_back(IsLeftOrBelow(u, v) ? BoundarySegment{u, v, 1}
                                             : BoundarySegment{v, u, -1});
      u = v;
    }
  }

  absl::c_sort(segments);
  return segments;
}

// Given a sorted list of boundary segment, returns a map from segment index to
// a list of intersection points on that segment.
absl::flat_hash_map<uint32_t, std::vector<Point>> FindIntersections(
    const std::vector<BoundarySegment>& segments) {
  absl::flat_hash_map<uint32_t, std::vector<Point>> intersections;
  absl::InlinedVector<uint32_t, 8> active;

  // Sweep the plane from left-to-right.
  for (uint32_t i = 0; i < segments.size(); ++i) {
    // First remove inactive segments from the active set.
    active.erase(std::remove_if(active.begin(), active.end(),
                                [&](uint32_t s_idx) {
                                  return segments[s_idx].end.x <=
                                         segments[i].start.x;
                                }),
                 active.end());

    auto [i_ymin, i_ymax] = std::minmax(segments[i].start.y, segments[i].end.y);

    // Check the new segments[i] against the active set for intersections.
    for (uint32_t j : active) {
      auto [j_ymin, j_ymax] =
          std::minmax(segments[j].start.y, segments[j].end.y);
      if (i_ymax < j_ymin || i_ymin > j_ymax) continue;

      if (auto ratios =
              SegmentIntersectionRatio({segments[i].start, segments[i].end},
                                       {segments[j].start, segments[j].end})) {
        if (ratios->first > 0.0f && ratios->first < 1.0f &&
            ratios->second > 0.0f && ratios->second < 1.0f) {
          Point p = segments[i].start +
                    ratios->first * (segments[i].end - segments[i].start);
          intersections[i].push_back(p);
          intersections[j].push_back(p);
        }
      }
    }

    // Add segments[i] onto the active set.
    active.push_back(i);
  }

  for (auto& [i, cuts] : intersections) {
    absl::c_sort(cuts, IsLeftOrBelow);
  }

  return intersections;
}

// Given a sorted list of segments, and a map from segment index to a list of
// intersection points on that segment, subdivides the segments at the
// intersection points.
void SubdivideSegments(
    std::vector<BoundarySegment>& segments,
    const absl::flat_hash_map<uint32_t, std::vector<Point>>& intersections) {
  size_t orig_size = segments.size();
  size_t intersection_count = 0;
  for (const auto& [i, cuts] : intersections) intersection_count += cuts.size();
  segments.reserve(orig_size + intersection_count);

  for (const auto& [seg_idx, cuts] : intersections) {
    BoundarySegment orig = segments[seg_idx];
    segments[seg_idx] = {orig.start, cuts[0], orig.orientation};
    for (size_t m = 0; m < cuts.size() - 1; ++m) {
      segments.push_back({cuts[m], cuts[m + 1], orig.orientation});
    }
    segments.push_back({cuts.back(), orig.end, orig.orientation});
  }

  // Since segments was sorted, and we expect typically only a small number of
  // intersections, sort the new segments and merge.
  std::sort(segments.begin() + orig_size, segments.end());
  std::inplace_merge(segments.begin(), segments.begin() + orig_size,
                     segments.end());
}

float ComputeY(const BoundarySegment& segment, float sweep_x) {
  double x0 = segment.start.x;
  double y0 = segment.start.y;
  double x1 = segment.end.x;
  double y1 = segment.end.y;
  double x = sweep_x;
  return static_cast<float>(y0 + (x - x0) * (y1 - y0) / (x1 - x0));
}

// Adds a boundary segment to the list of monotone chains.
// If a chain already exists and shares the start point of the segment, the
// segment is added to the chain. Otherwise, a new chain is created.
void AddBoundarySegment(const BoundarySegment& segment,
                        std::vector<std::vector<Point>>& pos_chains,
                        std::vector<std::vector<Point>>& neg_chains) {
  std::vector<std::vector<Point>>& chains =
      segment.orientation > 0 ? pos_chains : neg_chains;
  // Almost always, the last chains are the ones being constructed.
  for (auto it = chains.rbegin(); it != chains.rend(); ++it) {
    if (it->back() == segment.start) {
      it->push_back(segment.end);
      return;
    }
  }
  chains.push_back({segment.start, segment.end});
}

// Computes the winding number at a given Y-coordinate and X-coordinate,
// assuming that `active_segments` represents the set of active segments at the
// given X-coordinate, sorted along the sweep-line.
// For special handling of vertical segments only.
int GetWindingNumberAtY(
    const absl::InlinedVector<const BoundarySegment*, 16>& active_segments,
    float current_x, float test_y) {
  int w = 0;
  for (const BoundarySegment* seg : active_segments) {
    float y_seg = ComputeY(*seg, current_x);
    if (test_y < y_seg) return w;
    w += seg->orientation;
  }
  return w;
}

// Given a set of boundary segments that intersect only at their endpoints,
// extracts the geometric boundary of the polygon as a set of x-monotone chains.
std::vector<MonotoneChain> ExtractChains(
    const std::vector<BoundarySegment>& segments) {
  if (segments.empty()) return {};

  absl::InlinedVector<const BoundarySegment*, 16> active_segments;

  // A priority queue of ends of the active_segments.
  std::priority_queue<float, std::vector<float>, std::greater<>> end_xs;

  std::vector<std::vector<Point>> pos_chains, neg_chains;

  size_t s_idx = 0;
  float current_x = segments[0].start.x;

  while (s_idx < segments.size() || !end_xs.empty()) {
    // Compute winding numbers along the sweep-line, and add any ending segment
    // that lies on the boundary to the boundary chains.
    // Because the input loops are bounded, the region below the lowest active
    // segment represents the external unbounded component of the plane, where w
    // = 0.
    int w_below = 0;
    for (const BoundarySegment* seg : active_segments) {
      int w_above = w_below + seg->orientation;
      if (seg->end.x == current_x && ((w_below > 0) != (w_above > 0))) {
        AddBoundarySegment(*seg, pos_chains, neg_chains);
      }
      w_below = w_above;
    }

    // Special handling of vertical segments. Assuming all intersections are
    // transverse (no collinear overlapping boundary segments), crossing a
    // vertical segment changes the winding number by -seg.orientation. Thus, a
    // vertical segment lies on the boundary iff w_left is 0 or seg.orientation.
    while (s_idx < segments.size() && segments[s_idx].start.x == current_x &&
           segments[s_idx].start.x == segments[s_idx].end.x) {
      const BoundarySegment& seg = segments[s_idx];
      float mid_y = (seg.start.y + seg.end.y) * 0.5f;
      int w_left = GetWindingNumberAtY(active_segments, current_x, mid_y);
      int w_right = w_left - seg.orientation;
      if ((w_left > 0) != (w_right > 0)) {
        AddBoundarySegment(seg, pos_chains, neg_chains);
      }
      s_idx++;
    }

    // Remove inactive segments.
    active_segments.erase(
        std::remove_if(active_segments.begin(), active_segments.end(),
                       [&](const BoundarySegment* seg) {
                         return seg->end.x <= current_x;
                       }),
        active_segments.end());
    while (!end_xs.empty() && end_xs.top() <= current_x) end_xs.pop();

    // Add the new active segments.
    while (s_idx < segments.size() && segments[s_idx].start.x == current_x) {
      active_segments.push_back(&segments[s_idx]);
      end_xs.push(segments[s_idx].end.x);
      s_idx++;
    }

    // Compute the next x-coordinate for the sweep-line.
    float next_x = std::numeric_limits<float>::infinity();
    if (s_idx < segments.size())
      next_x = std::min(next_x, segments[s_idx].start.x);
    if (!end_xs.empty()) next_x = std::min(next_x, end_xs.top());

    // Sort the active segments by y-coordinate (they do not intersect other
    // than at endpoints, so use the midpoint to the next sweep x for
    // simplicity).
    float mid_x = (current_x + next_x) * 0.5f;
    absl::c_sort(active_segments,
                 [&](const BoundarySegment* a, const BoundarySegment* b) {
                   float y_a = ComputeY(*a, mid_x);
                   float y_b = ComputeY(*b, mid_x);
                   return y_a < y_b;
                 });

    current_x = next_x;
  }

  std::vector<MonotoneChain> result;
  result.reserve(pos_chains.size() + neg_chains.size());
  for (auto& chain : pos_chains) result.push_back({std::move(chain), 1});
  for (auto& chain : neg_chains) result.push_back({std::move(chain), -1});
  return result;
}

// Returns a pair of lists (prev_chain, next_chain) containing the indices of
// the predecessor and successor of each chain in an oriented walk along the
// shape boundary.
std::pair<absl::InlinedVector<uint32_t, 8>, absl::InlinedVector<uint32_t, 8>>
GetAdjacentChains(absl::Span<const MonotoneChain> chains) {
  auto start_pt = [](const MonotoneChain& c) {
    return c.Orientation() == 1 ? c.Vertices().front() : c.Vertices().back();
  };
  auto end_pt = [](const MonotoneChain& c) {
    return c.Orientation() == 1 ? c.Vertices().back() : c.Vertices().front();
  };

  absl::InlinedVector<uint32_t, 8> prev_chain(chains.size());
  absl::InlinedVector<uint32_t, 8> next_chain(chains.size());

  // Brute force search, since we expect that typically the number of chains is
  // small.
  for (size_t i = 0; i < chains.size(); ++i) {
    Point end_i = end_pt(chains[i]);
    Point start_i = start_pt(chains[i]);
    for (size_t j = i + 1; j < chains.size(); ++j) {
      if (start_pt(chains[j]) == end_i) {
        next_chain[i] = j;
        prev_chain[j] = i;
      }
      if (end_pt(chains[j]) == start_i) {
        next_chain[j] = i;
        prev_chain[i] = j;
      }
    }
  }
  return {prev_chain, next_chain};
}

// Returns true if no other vertex indexed by `indices` lies within the triangle
// defined by indices `i`, `j`, `k`.
bool IsEar(size_t i, size_t j, size_t k, absl::Span<const uint32_t> indices,
           const std::vector<Point>& vertices) {
  Point A = vertices[indices[j]];
  Point B = vertices[indices[i]];
  Point C = vertices[indices[k]];

  Vec vAB = B - A;
  Vec vBC = C - B;
  Vec vCA = A - C;

  if (Vec::Determinant(vAB, vBC) < kCollinearTolerance) return false;

  for (size_t v = 0; v < indices.size(); ++v) {
    if (v == j || v == i || v == k) continue;
    Point P = vertices[indices[v]];
    if (Vec::Determinant(vAB, P - A) >= 0 &&
        Vec::Determinant(vBC, P - B) >= 0 &&
        Vec::Determinant(vCA, P - C) >= 0) {
      return false;
    }
  }
  return true;
}

float Area(absl::Span<const Point> loop) {
  float area = 0.0f;
  Point prev = loop.back();
  for (Point next : loop) {
    area += Vec::Determinant(prev.Offset(), next.Offset());
    prev = next;
  }
  return 0.5f * area;
}

// Represents an intersection on a chain.
struct ChainIntersection {
  // Intersection point
  Point pos;

  // Segment containing the intersection.
  uint32_t seg;

  // Topological orientation of the intersection: +1 if the other chain
  // goes left-to-right across the intersection (from the perspective of this
  // chain), and -1 if right-to-left.
  int orientation;
};

// Finds all boundary intersections between the shapes. The result is returned
// as a pair of vectors, one for each shape, where each vector contains the
// intersections on each chain in the shape.
//
// This function is intended for when `shape_a` is very small compared to
// `shape_b` (for example, when `shape_a` is a single triangle of an ink
// stroke, and `shape_b` is an entire eraser stroke).
std::pair<absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>,
          absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>>
FindBoundaryIntersections(const ShapeOutline& shape_a,
                          const ShapeOutline& shape_b) {
  absl::Span<const MonotoneChain> chains_a = shape_a.Chains();
  absl::Span<const MonotoneChain> chains_b = shape_b.Chains();

  absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>
      intersections_a(chains_a.size());
  absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>
      intersections_b(chains_b.size());

  for (uint32_t a_idx = 0; a_idx < chains_a.size(); ++a_idx) {
    const MonotoneChain& chain_a = chains_a[a_idx];
    for (uint32_t b_idx = 0; b_idx < chains_b.size(); ++b_idx) {
      const MonotoneChain& chain_b = chains_b[b_idx];

      if (!IntersectsInternal(chain_a.Bounds(), chain_b.Bounds())) continue;

      absl::Span<const Point> verts_a = chain_a.Vertices();
      absl::Span<const Point> verts_b = chain_b.Vertices();

      // Rather than using a sweep-line approach, we instead scan through
      // verts_a and binary search in verts_b (in O(|verts_a| Log |verts_b|)
      // time) rather than a simultaneous scan (in O(|verts_a| + |verts_b|)
      // time).

      // As we scan through the chain_a, `j0` is the index of the last vertex in
      // chain_b with x-coordinate less than the current a_segment.
      uint32_t j0 = 0;

      for (uint32_t i = 0; i + 1 < verts_a.size(); ++i) {
        auto [a_ymin, a_ymax] = std::minmax(verts_a[i].y, verts_a[i + 1].y);

        j0 = std::lower_bound(verts_b.begin() + j0, verts_b.end(), verts_a[i],
                              [](Point a, Point b) { return a.x < b.x; }) -
             verts_b.begin();
        if (j0 > 0) j0 -= 1;

        for (uint32_t j = j0; j + 1 < verts_b.size(); ++j) {
          if (verts_b[j].x > verts_a[i + 1].x) break;
          auto [b_ymin, b_ymax] = std::minmax(verts_b[j].y, verts_b[j + 1].y);
          if (a_ymax < b_ymin || a_ymin > b_ymax) continue;

          if (auto ratios = geometry_internal::SegmentIntersectionRatio(
                  {verts_a[i], verts_a[i + 1]}, {verts_b[j], verts_b[j + 1]})) {
            // TODO(b/521449017): Handle degenerate intersections
            int sign = (chain_a.Orientation() * chain_b.Orientation() *
                        Vec::Determinant(verts_b[j + 1] - verts_b[j],
                                         verts_a[i + 1] - verts_a[i])) > 0.f
                           ? 1
                           : -1;
            Point cut = Segment{verts_a[i], verts_a[i + 1]}.Lerp(ratios->first);
            intersections_a[a_idx].push_back({cut, i, sign});
            intersections_b[b_idx].push_back({cut, j, -sign});
          }
        }
      }
    }
  }

  return {intersections_a, intersections_b};
}

// Helper function to slice the given chain at the given intersection points and
// extracts the subchains that lie on the boundary of the shape_a - shape_b.
// `is_subtracted` indicates whether `chain` belongs to shape_a and
// `other_shape` is shape_b, or vice-versa.
void SliceChain(const MonotoneChain& chain,
                absl::InlinedVector<ChainIntersection, 2>& intersections,
                bool is_subtracted, const ShapeOutline& other_shape,
                std::vector<MonotoneChain>& raw_chains) {
  int orientation = is_subtracted ? -chain.Orientation() : chain.Orientation();

  if (intersections.empty()) {
    // If there are no intersections, then either the entire chain is on the
    // the boundary of shape_a - shape_b or none of it is. If other_shape is
    // shape_b, then chain belongs to boundary if it is not contained in
    // shape_b. If other_shape is shape_a, then chain belongs to the boundary if
    // it is contained in other_shape.
    if (Intersects(other_shape, chain.Vertices().front()) == is_subtracted)
      raw_chains.push_back(MonotoneChain(
          std::vector<Point>(chain.Vertices().begin(), chain.Vertices().end()),
          orientation));
    return;
  }

  absl::c_sort(intersections,
               [](const ChainIntersection& c1, const ChainIntersection& c2) {
                 return IsLeftOrBelow(c1.pos, c2.pos);
               });
  absl::Span<const Point> verts = chain.Vertices();

  // The intersections split up the chain into pieces, which are alternatively
  // outside or inside other_shape. We keep track of whether the current piece
  // lies on the boundary of the difference with `is_boundary`, which
  // is initialized based on whether the first intersection is entering or
  // leaving the other shape.
  bool is_boundary = orientation == intersections[0].orientation;

  // Add a sentinel representing the end of the chain to avoid a separate
  // terminal check after the loop.
  intersections.push_back(
      {verts.back(), static_cast<uint32_t>(verts.size() - 2), 0});

  size_t curr_seg = 0;
  Point start_pos = verts.front();

  for (const ChainIntersection& cut : intersections) {
    if (is_boundary) {
      std::vector<Point> piece;
      piece.reserve(cut.seg - curr_seg + 2);
      piece.push_back(start_pos);
      piece.insert(piece.end(), verts.begin() + curr_seg + 1,
                   verts.begin() + cut.seg + 1);
      piece.push_back(cut.pos);
      raw_chains.push_back({std::move(piece), orientation});
    }
    is_boundary = !is_boundary;
    curr_seg = cut.seg;
    start_pos = cut.pos;
  }
}
// Stitches a collection of sub-chains into maximal monotone chains by merging
// adjacent chains that share endpoints and have the same orientation.
std::vector<MonotoneChain> StitchIntersectionChains(
    std::vector<MonotoneChain> raw_chains) {
  auto [prev, next] = GetAdjacentChains(raw_chains);

  for (size_t i = 0; i < raw_chains.size(); ++i) {
    if (raw_chains[i].Vertices().empty()) continue;
    int orientation = raw_chains[i].Orientation();
    auto& neighbor = orientation == 1 ? next : prev;
    uint32_t j = neighbor[i];
    while (j != i && !raw_chains[j].Vertices().empty() &&
           orientation == raw_chains[j].Orientation()) {
      std::vector<Point> merged(raw_chains[i].Vertices().begin(),
                                raw_chains[i].Vertices().end());
      absl::Span<const Point> j_verts = raw_chains[j].Vertices();
      merged.insert(merged.end(), j_verts.begin() + 1, j_verts.end());
      raw_chains[j] = MonotoneChain({}, orientation);

      raw_chains[i] = MonotoneChain(std::move(merged), orientation);
      neighbor[i] = neighbor[j];
      j = neighbor[i];
    }
  }

  raw_chains.erase(std::remove_if(raw_chains.begin(), raw_chains.end(),
                                  [](const MonotoneChain& c) {
                                    return c.Vertices().empty();
                                  }),
                   raw_chains.end());
  return raw_chains;
}

// Given two shapes and their boundary intersection points, returns a list of
// monotone chains that form the boundary of the difference of the two shapes.
std::vector<MonotoneChain> SubdivideIntersectionChains(
    const ShapeOutline& shape_a, const ShapeOutline& shape_b,
    absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>& intx_a,
    absl::InlinedVector<absl::InlinedVector<ChainIntersection, 2>, 8>& intx_b) {
  std::vector<MonotoneChain> raw_chains;
  for (uint32_t i = 0; i < shape_a.Chains().size(); ++i) {
    SliceChain(shape_a.Chains()[i], intx_a[i], false, shape_b, raw_chains);
  }
  for (uint32_t j = 0; j < shape_b.Chains().size(); ++j) {
    SliceChain(shape_b.Chains()[j], intx_b[j], true, shape_a, raw_chains);
  }

  return raw_chains;
}

}  // namespace

MonotoneChain::MonotoneChain(std::vector<Point> vertices, int orientation)
    : vertices_(std::move(vertices)),
      orientation_(orientation),
      bounds_(Envelope(vertices_).AsRect().value_or(Rect())) {
  if (!absl::c_is_sorted(vertices_, IsLeftOrBelow))
    absl::c_sort(vertices_, IsLeftOrBelow);
}

ShapeOutline::ShapeOutline(std::vector<MonotoneChain> input_chains)
    : chains_(std::move(input_chains)) {
  absl::c_sort(chains_, [](const MonotoneChain& a, const MonotoneChain& b) {
    return std::tuple(a.Bounds().YMin(), a.Bounds().XMin(), a.Bounds().YMax(),
                      a.Bounds().XMax(), a.Orientation()) <
           std::tuple(b.Bounds().YMin(), b.Bounds().XMin(), b.Bounds().YMax(),
                      b.Bounds().XMax(), b.Orientation());
  });
}

ShapeOutline ComputeShapeOutline(const std::vector<std::vector<Point>>& loops) {
  // This function conceptually implements a sweep-line approach: we sweep the
  // plane from left-to-right, pausing at every endpoint and intersection point
  // to compute the winding numbers along the sweep-line, and construct the
  // boundary of the polygon.
  //
  // For simplicity, we do this in two passes, instead of doing everything in
  // one. In a first pass, we compute all self-intersections, and subdivide
  // boundary segments to so that all intersections occur at endpoints. In
  // the second pass, we compute the winding numbers, identify the segments
  // lying on the boundary, and construct the monotone chains.

  std::vector<BoundarySegment> segments = GetSegments(loops);
  absl::flat_hash_map<uint32_t, std::vector<Point>> intersections =
      FindIntersections(segments);
  SubdivideSegments(segments, intersections);
  return ShapeOutline(ExtractChains(segments));
}

bool Intersects(const ShapeOutline& shape, const Point& p) {
  // This implements a ray-casting approach: we cast a ray from p in the
  // negative y-direction, and count the number of intersections with the
  // boundary of the polygon. The point intersects the polygon if the number of
  // intersections is odd, or if the point is on the boundary.

  int crossings = 0;

  for (const MonotoneChain& chain : shape.Chains()) {
    // Since the chains in the shape are sorted, if the point is below the
    // lower bound of this chain, we can safely skip the rest.
    if (p.y < chain.Bounds().YMin()) break;

    if (p.x < chain.Bounds().XMin() || p.x > chain.Bounds().XMax()) continue;

    if (p.y > chain.Bounds().YMax()) {
      crossings += 1;
      continue;
    }

    absl::Span<const Point> pts = chain.Vertices();

    // The first point in the chain with pR.x >= p.x
    auto it = std::lower_bound(pts.begin(), pts.end(), Point{p.x, 0},
                               [](Point a, Point b) { return a.x < b.x; });

    // In theory, since the bounding box check above guarantees that p.x is in
    // the range [XMin, XMax], `it` should never be pts.end(), but check anyway
    // to be safe.
    if (it == pts.end()) continue;
    Point pR = *it;

    if (pR.x == p.x) {
      if (p.y < pR.y) continue;
      if (p.y == pR.y) return true;

      // Special check for p lying on a vertical boundary: since pR is the first
      // point with the same x-coordinate, and the monotone chains have vertical
      // segments ordered bottom-to-top, the last point in the chain with the
      // same x-coordinate is the top of the piece of vertical boundary.
      while (it + 1 != pts.end() && (it + 1)->x == p.x) ++it;
      if (p.y <= it->y) return true;

      crossings += 1;
      continue;
    }

    // In theory, by the bounding box check, if `it` is pts.begin(), then it
    // must be true that it->x == p.x, which is handled above. But check just
    // to be safe.
    if (it == pts.begin()) continue;
    Point pL = *(it - 1);

    if (p.y < std::min(pL.y, pR.y)) continue;

    if (p.y > std::max(pL.y, pR.y)) {
      crossings += 1;
      continue;
    }

    // We tried so hard, and got so far, but in the end we actually have to do
    // some geometry.
    double det = static_cast<double>(p.x - pL.x) * (pR.y - pL.y) -
                 static_cast<double>(p.y - pL.y) * (pR.x - pL.x);
    if (det == 0) return true;
    if (det < 0) crossings += 1;
  }

  return (crossings % 2) != 0;
}

bool Intersects(const ShapeOutline& shape, const Rect& rect) {
  // A rect and shape intersect if the boundary of the shape intersects the
  // boundary of the rect, or the shape is contained in the rect, or the rect is
  // contained shape.

  // Check if the boundary of shape intersects the boundary of rect, or is
  // contained in rect.
  for (const MonotoneChain& chain : shape.Chains()) {
    // Since the chains in the shape are sorted, if the point is below the
    // lower bound of this chain, we can safely skip the rest.
    if (rect.YMax() < chain.Bounds().YMin()) break;
    if (!geometry_internal::IntersectsInternal(chain.Bounds(), rect)) continue;

    absl::Span<const Point> pts = chain.Vertices();

    // To determine if the chain intersects the rect, we find the portion of the
    // chain with x-coordinates in the range [rect.XMin(), rect.XMax()]. For
    // vertices in this range, we can check for intersection easily -- there is
    // no intersection if and only if all vertices are either strictly below
    // rect.YMin() or strictly above rect.YMax(). Then we also check the
    // incoming and leaving segments (crossing XMin and XMax) with a standard
    // segment-box intersection check.

    // Look for the first point with x-coordinate greater than or equal to
    // rect.XMin().
    auto left_it =
        std::lower_bound(pts.begin(), pts.end(), Point{rect.XMin(), 0},
                         [](Point a, Point b) { return a.x < b.x; });

    // Iterate through all points with x-coordinates in the range
    // [rect.XMin(), rect.XMax()].
    bool above = false, below = false;
    auto it = left_it;
    for (; it != pts.end(); ++it) {
      if (it->x > rect.XMax()) break;
      if (it->y > rect.YMax())
        above = true;
      else if (it->y < rect.YMin())
        below = true;
      else
        return true;
      if (above && below) return true;
    }

    // First point to the right of the rect.
    auto right_it = it;

    // Lastly, check the incoming and leaving segments.
    if (left_it != pts.begin() &&
        geometry_internal::IntersectsInternal(
            rect, Segment{*(left_it - 1), *left_it})) {
      return true;
    }
    if (right_it != pts.end() && right_it != left_it &&
        geometry_internal::IntersectsInternal(
            rect, Segment{*(right_it - 1), *right_it})) {
      return true;
    }
  }

  // If no intersections, check if rect is contained in shape.
  return Intersects(shape, rect.Center());
}

std::vector<std::vector<Point>> ComputeBoundaryLoops(
    const ShapeOutline& shape) {
  // We stitch the monotone chains into closed loops by first computing each
  // chain's "next" chain in the shape's oriented boundary. We can think of this
  // as a directed graph on the set of chains. We traverse all of cycles
  // in this graph, gluing together the chains as we go.
  auto [_, next] = GetAdjacentChains(shape.Chains());

  std::vector<std::vector<Point>> loops;
  absl::Span<const MonotoneChain> chains = shape.Chains();

  absl::InlinedVector<bool, 8> visited(chains.size(), false);
  for (size_t m = 0; m < chains.size(); ++m) {
    if (visited[m]) continue;

    if (chains[m].Vertices().size() < 2) continue;

    std::vector<Point> loop;
    uint32_t curr = m;
    do {
      visited[curr] = true;
      absl::Span<const Point> pts = chains[curr].Vertices();
      if (chains[curr].Orientation() == 1)
        loop.insert(loop.end(), pts.begin(), pts.end() - 1);
      else
        loop.insert(loop.end(), pts.rbegin(), pts.rend() - 1);
      curr = next[curr];
    } while (curr != m && !visited[curr]);

    loops.push_back(std::move(loop));
  }

  return loops;
}

std::pair<std::vector<Point>, std::vector<std::array<uint32_t, 3>>>
ComputeTriangulation(const ShapeOutline& shape) {
  // This function implements an ear-clipping approach, which is performant
  // for small and simple polygon that are the most common cases.
  // TODO(b/521449017): Implement monotone triangulation for more efficiently
  // triangulating larger or more complicated shapes.

  std::vector<Point> vertices;
  std::vector<std::array<uint32_t, 3>> triangles;

  std::vector<std::vector<Point>> loops = ComputeBoundaryLoops(shape);
  size_t total_triangles = 0;
  for (const auto& loop : loops) {
    if (loop.size() >= 3) {
      total_triangles += loop.size() - 2;
    }
  }
  triangles.reserve(total_triangles);

  // TODO(b/521449017): Handle triangulating shapes with holes. For now,
  // return empty if the shape has a hole.
  if (std::any_of(loops.begin(), loops.end(),
                  [](const auto& loop) { return Area(loop) < 0; })) {
    return {vertices, triangles};
  }

  absl::InlinedVector<uint32_t, 16> indices;

  size_t total_vertices = 0;
  size_t max_loop_size = 0;
  for (const auto& boundary : loops) {
    total_vertices += boundary.size();
    max_loop_size = std::max(boundary.size(), max_loop_size);
  }
  vertices.reserve(total_vertices);
  indices.reserve(max_loop_size);

  for (const auto& loop : loops) {
    uint32_t start = static_cast<uint32_t>(vertices.size());
    uint32_t count = static_cast<uint32_t>(loop.size());
    vertices.insert(vertices.end(), loop.begin(), loop.end());

    indices.resize(count);
    for (uint32_t i = 0; i < count; ++i) indices[i] = start + i;

    // Ear clipping loop.
    for (size_t step = 0; step < count; ++step) {
      if (indices.size() < 3) break;

      size_t n = indices.size();
      size_t prev = n - 1;
      size_t next = 1;
      for (size_t cur = 0; cur < n; ++cur) {
        if (IsEar(cur, prev, next, indices, vertices)) {
          triangles.push_back({indices[prev], indices[cur], indices[next]});
          indices.erase(indices.begin() + cur);
          break;
        }
        prev = cur;
        next = (next + 1 == n) ? 0 : next + 1;
      }
    }
  }

  return {vertices, triangles};
}

ShapeOutline ComputeSubtraction(const ShapeOutline& shape_a,
                                const ShapeOutline& shape_b) {
  if (shape_a.Chains().empty()) return ShapeOutline({});
  if (shape_b.Chains().empty()) return shape_a;

  // To compute the subtraction of shape_b from shape_a, we first compute all
  // intersection points between the boundaries of the two shapes. We then
  // subdivide the boundary chains of both shapes, and extract the fragments
  // that lie on the boundary of (shape_a - shape_b). Finally, we stitch the
  // fragments back to gether to form the chains of the final result.

  auto [intx_a, intx_b] = FindBoundaryIntersections(shape_a, shape_b);
  std::vector<MonotoneChain> raw_chains =
      SubdivideIntersectionChains(shape_a, shape_b, intx_a, intx_b);
  return ShapeOutline(StitchIntersectionChains(std::move(raw_chains)));
}

}  // namespace ink::geometry_internal
