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
#include <cstdlib>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/internal/intersects_internal.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"

namespace ink::geometry_internal {

// TODO(b/521448869): Improve numerical robustness -- it's common
// in line-sweep algorithms to quantize points to a grid to avoid precision
// instability.
// TODO(b/521449017): Handle degenerate intersections, including overlapping
// segments, T-junctions, simultaneous intersections of more than two segments,
// etc.

namespace {

// Relative error margin for 32-bit floating point operations.
constexpr float kFloatTolerance = 1e-6f;

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

// A helper struct to represent an intersection event on a chain.
struct ChainIntersection {
  // Intersection point
  Point pos;

  // The index of the vertex in the chain at or immediately preceding the
  // intersection.
  uint32_t vertex;

  // The sign of the intersection: +1 if the other chain crosses from
  // left-to-right (from the perspective of this chain), and -1 if
  // right-to-left, and 0 for a non-crossing intersection.
  int sign;
};

bool IsLeftOrBelow(const Point& a, const Point& b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

// Computes the 2D cross product (determinant) of the vectors ab and ac.
double Determinant(const Point& a, const Point& b, const Point& c) {
  double ux = static_cast<double>(b.x) - a.x;
  double uy = static_cast<double>(b.y) - a.y;
  double vx = static_cast<double>(c.x) - a.x;
  double vy = static_cast<double>(c.y) - a.y;
  return ux * vy - uy * vx;
}

int Sign(double x) {
  if (x > 0.0) return 1;
  if (x < 0.0) return -1;
  return 0;
}

// Given a value `t` and an interval [`a`,`b`], clamps `t` to the interval
// and snaps it to an endpoint if the clamped value is within 'err` of
// the endpoint.
double Snap(double a, double b, double t, double err) {
  if (t - a <= err) return a;
  if (b - t <= err) return b;
  return t;
}

// Returns true if (`a`, `b`, `c`) are in clockwise order around the point `p`.
// This function assumes none of the points are collinear.
bool ClockwiseOrdered(const Point& p, const Point& a, const Point& b,
                      const Point& c) {
  // The points a and p split the plane into two half-planes, of points to the
  // left/right of the line ap. If b and c lie in opposite half-planes, then
  // whichever lies to the right of ap is first (with respect to the clockwise
  // ordering). Otherwise, (a,b,c) is clockwise ordered iff b-p and c-p are
  // clockwise ordered.
  int b_side = Sign(Determinant(p, a, b));
  int c_side = Sign(Determinant(p, a, c));
  if (b_side != c_side) return b_side < c_side;
  return Sign(Determinant(p, b, c)) < 0;
}

// Helper function that computes the intersection sign of chains
// (...`u1`,`u2`,`u3`...) and (...`v1`,`v2`...), which intersect at the vertex
// `u2` and the edge `v1``v2`. This assumes no segments are collinear.
int VertexEdgeIntersectionSign(const Point& u1, const Point& u2,
                               const Point& u3, const Point& v1,
                               const Point& v2) {
  // If u1 and u3 are on the same side of v1v2, then the intersection is
  // non-transverse and has sign 0. Otherwise, the sign is +1 if u1 is to the
  // left and u3 to the right, and -1 for vice versa.
  int u1_side = Sign(Determinant(v1, v2, u1));
  int u3_side = Sign(Determinant(v1, v2, u3));
  return u1_side == -u3_side ? u1_side : 0;
}

// Helper function that computes the intersection sign of chains
// (...`u1`,`u2`,`u3`...) and (...`v1`,`v2`,`v3`...), which intersect at the
// shared vertex `u2` = `v2`. This assumes no segments are collinear.
int VertexVertexIntersectionSign(const Point& u1, const Point& u2,
                                 const Point& u3, const Point& v1,
                                 const Point& v3) {
  // The points u1, u2, u3 define a sector in the plane, consisting of points
  // "left" of both u1u2 and u2u3. If v1 lies within the sector and v3 doesn't,
  // then the intersection is positive (the points (v1, v2, v3) cross
  // (u1, u2, u3) from the left to right), and vice versa -1. If v1 and v3
  // are both contained in the sector or both not contained, then the
  // intersection is non-transverse and therefore the sign is zero.
  if (ClockwiseOrdered(u2, u1, v1, u3) && ClockwiseOrdered(u2, u3, v3, u1))
    return -1;
  if (ClockwiseOrdered(u2, u1, v3, u3) && ClockwiseOrdered(u2, u3, v1, u1))
    return 1;
  return 0;
}

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

// Given a sorted list of boundary segments, returns a map from segment index to
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

  // Since segments were sorted, and we expect typically only a small number of
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

// Returns the vertices preceding and following the given vertex in the oriented
// boundary of `shape`.
std::pair<Point, Point> GetNeighbors(const ShapeOutline& shape,
                                     uint32_t chain_idx, uint32_t vertex_idx) {
  absl::Span<const MonotoneChain> chains = shape.Chains();
  absl::Span<const Point> vertices = chains[chain_idx].Vertices();
  int step = chains[chain_idx].Orientation();

  // If the vertex is in the middle of the chain, the neighboring vertices can
  // be found in the same chain. Otherwise, we need to find the next/previous
  // chain along the boundary to find the neighbor.
  Point u = (vertex_idx - step < vertices.size())
                ? vertices[vertex_idx - step]
                : chains[shape.PrevIndex(chain_idx)].EndNeighbor();
  Point w = (vertex_idx + step < vertices.size())
                ? vertices[vertex_idx + step]
                : chains[shape.NextIndex(chain_idx)].StartNeighbor();

  return {u, w};
}

// Returns a pair of lists (prev_chain, next_chain) containing the indices of
// the predecessor and successor of each chain in an oriented walk along the
// shape boundary.
std::pair<std::vector<uint32_t>, std::vector<uint32_t>> GetAdjacentChains(
    absl::Span<const MonotoneChain> chains) {
  std::vector<uint32_t> prev_index(chains.size());
  std::vector<uint32_t> next_index(chains.size());

  // Initialize the arrays with prev_index[i] == i and next_index[i] == i,
  // to indicate that a chain hasn't yet been visited.
  std::iota(prev_index.begin(), prev_index.end(), 0);
  std::iota(next_index.begin(), next_index.end(), 0);

  // Brute force search, since we expect that typically the number of chains is
  // small (note that for the typical Ink use-case, this function is handling
  // the result of subtracting from a single triangle, so the number of chains
  // is typically less than 10).
  // TODO(b/523326691): Check whether this approach is indeed optimal for the
  // typical case.
  for (size_t i = 0; i < chains.size(); ++i) {
    Point end_i = chains[i].EndPoint();

    for (size_t j = 0; j < chains.size(); ++j) {
      if (i == j) continue;

      // The next/previous chains of a given chain are unique. To avoid
      // unnecessary processing, skip chains that have already been claimed.
      if (prev_index[j] != j) continue;

      // The next chain always starts exactly where this chain ends.
      if (chains[j].StartPoint() != end_i) continue;

      // If we haven't set any chain as the next chain, take the first one we
      // find which starts where this chain ends.
      if (next_index[i] == i) {
        next_index[i] = j;
        continue;
      }

      // Typically, there is a single chain that starts where chain[i] ends.
      // However, at a pinch point, there can be multiple, and we need to be a
      // little careful. Locally, the shape looks like several angular sectors
      // that touch at a shared apex, with alternating incoming and outgoing
      // chains around the pinch point. For any incoming chain, the next chain
      // in the boundary walk is the first outgoing chain encountered in a
      // clockwise sweep around the vertex.
      //
      // For an example, look at point B in test case
      // `AdjacentChains.PinchPoint`.
      if (ClockwiseOrdered(end_i, chains[i].EndNeighbor(),
                           chains[j].StartNeighbor(),
                           chains[next_index[i]].StartNeighbor())) {
        next_index[i] = j;
      }
    }

    if (next_index[i] != i) prev_index[next_index[i]] = i;
  }
  return {prev_index, next_index};
}

// Returns true if no other vertex indexed by `indices` lies within the triangle
// defined by indices `i`, `j`, `k`.
bool IsEar(size_t i, size_t j, size_t k, absl::Span<const uint32_t> indices,
           const std::vector<Point>& vertices) {
  Point A = vertices[indices[j]];
  Point B = vertices[indices[i]];
  Point C = vertices[indices[k]];

  if (Sign(Determinant(A, B, C)) <= 0) return false;

  for (size_t v = 0; v < indices.size(); ++v) {
    if (v == j || v == i || v == k) continue;
    Point P = vertices[indices[v]];
    if (Sign(Determinant(A, B, P)) >= 0 && Sign(Determinant(B, C, P)) >= 0 &&
        Sign(Determinant(C, A, P)) >= 0)
      return false;
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

// Given a sorted list of points `verts`, returns the index of the last element
// in `verts` that is less than `p`. Ordering is defined by `IsLeftOrBelow`.
uint32_t GetStartIndex(absl::Span<const Point> verts, Point p) {
  uint32_t i = std::lower_bound(verts.begin(), verts.end(), p, IsLeftOrBelow) -
               verts.begin();
  return i > 0 ? i - 1 : 0;
}

// Finds any intersections between the `i`th segment of the first chain and
// the `j`th segment of the second chain, appending them to `intersections_a`
// and `intersections_b`.
void FindSegmentIntersections(
    const ShapeOutline& shape_a, uint32_t a_idx, uint32_t i,
    const ShapeOutline& shape_b, uint32_t b_idx, uint32_t j,
    absl::InlinedVector<ChainIntersection, 2>& intersections_a,
    absl::InlinedVector<ChainIntersection, 2>& intersections_b) {
  const MonotoneChain& chain_a = shape_a.Chains()[a_idx];
  const MonotoneChain& chain_b = shape_b.Chains()[b_idx];
  absl::Span<const Point> verts_a = chain_a.Vertices();
  absl::Span<const Point> verts_b = chain_b.Vertices();

  // The intersection point between the two segments is obtained by
  // finding the parametric values t and s such that:
  //  Lerp(verts_a[i], verts_a[i + 1], t) ==
  //  Lerp(verts_b[j], verts_b[j + 1], s)
  //
  // The general solution is
  //  t = (w x v) / (u x v)
  //  s = (w x u) / (u x v)
  // where
  //  u = verts_a[i + 1] - verts_a[i]
  //  v = verts_b[j + 1] - verts_b[j]
  //  w = verts_b[j] - verts_a[i]

  // The denominator determinant, u x v
  const double u_x_v = Determinant(verts_a[i], verts_a[i + 1], verts_b[j + 1]) -
                       Determinant(verts_a[i], verts_a[i + 1], verts_b[j]);

  // TODO(b/521449017): Handle collinear intersections.
  if (u_x_v == 0.0) return;

  // Normalize the denominator to be positive, for simplicity.
  const int uv_sign = Sign(u_x_v);
  const double abs_u_x_v = std::abs(u_x_v);

  // The numerators, w x v and w x u.
  const double w_x_v =
      uv_sign * Determinant(verts_a[i], verts_b[j], verts_b[j + 1]);
  const double w_x_u =
      uv_sign * Determinant(verts_a[i], verts_b[j], verts_a[i + 1]);

  // To ensure that we capture intersections at the endpoint, expand the
  // segments by a bit to account for floating point precision. Note that
  // this can result in duplicates (with an intersection from each adjacent
  // segment) or false-positives. The duplicates are explicitly handled in a
  // post processing step; false-positives will have zero intersection sign,
  // and will cause little harm.
  const double err = kFloatTolerance * abs_u_x_v;
  if (w_x_v < -err || w_x_v > abs_u_x_v + err) return;
  if (w_x_u < -err || w_x_u > abs_u_x_v + err) return;

  // Intersections close to endpoints are snapped exactly to the endpoint, so
  // that t and s are in [0,1], with values of 0 and 1 corresponding to
  // endpoints.
  const float t = Snap(0.0f, abs_u_x_v, w_x_v, err) / abs_u_x_v;
  const float s = Snap(0.0f, abs_u_x_v, w_x_u, err) / abs_u_x_v;

  // We explicitly branch on the different intersection configurations
  // of edge-edge, vertex-edge, or vertex-vertex intersection.
  if (t > 0.0f && t < 1.0f && s > 0.0f && s < 1.0f) {
    // The generic case: the segments cross each other in their interiors.
    Point p = verts_a[i] + t * (verts_a[i + 1] - verts_a[i]);
    int sign = -chain_a.Orientation() * chain_b.Orientation() * uv_sign;
    intersections_a.push_back({p, i, sign});
    intersections_b.push_back({p, j, -sign});
  } else if (0.0 < s && s < 1.0) {
    // A vertex on chain_a lies on the current segment of chain_b.
    uint32_t vert_i = (t == 1.0) ? i + 1 : i;
    Point p = verts_a[vert_i];
    auto [u_in, u_out] = GetNeighbors(shape_a, a_idx, vert_i);
    int sign =
        -chain_b.Orientation() *
        VertexEdgeIntersectionSign(u_in, p, u_out, verts_b[j], verts_b[j + 1]);
    intersections_a.push_back({p, vert_i, sign});
    intersections_b.push_back({p, j, -sign});
  } else if (0.0 < t && t < 1.0) {
    // A vertex on chain_b lies on the current segment of chain_a.
    uint32_t vert_j = (s == 1.0) ? j + 1 : j;
    Point p = verts_b[vert_j];
    auto [v_in, v_out] = GetNeighbors(shape_b, b_idx, vert_j);
    int sign =
        chain_a.Orientation() *
        VertexEdgeIntersectionSign(v_in, p, v_out, verts_a[i], verts_a[i + 1]);
    intersections_a.push_back({p, i, sign});
    intersections_b.push_back({p, vert_j, -sign});
  } else {
    // A vertex on chain_a overlaps a vertex on chain_b.
    uint32_t vert_i = (t == 1.0) ? i + 1 : i;
    uint32_t vert_j = (s == 1.0) ? j + 1 : j;
    Point p = verts_b[vert_j];
    auto [u_in, u_out] = GetNeighbors(shape_a, a_idx, vert_i);
    auto [v_in, v_out] = GetNeighbors(shape_b, b_idx, vert_j);
    int sign = -VertexVertexIntersectionSign(u_in, p, u_out, v_in, v_out);
    intersections_a.push_back({p, vert_i, sign});
    intersections_b.push_back({p, vert_j, -sign});
  }
}

// Finds all boundary intersections between the shapes. The result is returned
// as a pair of vectors, one for each shape, where each vector contains the
// intersections on each chain in order.
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
    absl::Span<const Point> verts_a = chain_a.Vertices();
    for (uint32_t b_idx = 0; b_idx < chains_b.size(); ++b_idx) {
      const MonotoneChain& chain_b = chains_b[b_idx];

      if (!IntersectsInternal(chain_a.Bounds(), chain_b.Bounds())) continue;

      absl::Span<const Point> verts_b = chain_b.Vertices();

      // We sweep across the plane to find intersections between the two
      // chains. At each step, we test the current segment of chain_a against
      // the current segment of chain_b, and then advance to the next segment
      // of either chain_a or chain_b depending on which segment ends first.

      // Find the initial i and j using binary search.
      uint32_t i = GetStartIndex(verts_a, verts_b[0]);
      uint32_t j = GetStartIndex(verts_b, verts_a[0]);

      while (i < verts_a.size() - 1 && j < verts_b.size() - 1) {
        auto [a_ymin, a_ymax] = std::minmax(verts_a[i].y, verts_a[i + 1].y);
        auto [b_ymin, b_ymax] = std::minmax(verts_b[j].y, verts_b[j + 1].y);

        // Bounding box check.
        if (!(a_ymax < b_ymin || a_ymin > b_ymax)) {
          FindSegmentIntersections(shape_a, a_idx, i, shape_b, b_idx, j,
                                   intersections_a[a_idx],
                                   intersections_b[b_idx]);
        }

        if (IsLeftOrBelow(verts_a[i + 1], verts_b[j + 1])) {
          i += 1;
        } else {
          j += 1;
        }
      }
    }
  }

  // Remove duplicate intersections from the chains.
  auto dedup = [](absl::InlinedVector<ChainIntersection, 2>& intersections) {
    absl::c_sort(intersections,
                 [](const ChainIntersection& c1, const ChainIntersection& c2) {
                   return IsLeftOrBelow(c1.pos, c2.pos);
                 });
    auto last = std::unique(
        intersections.begin(), intersections.end(),
        [](const ChainIntersection& c1, const ChainIntersection& c2) {
          return c1.pos == c2.pos;
        });
    intersections.erase(last, intersections.end());
  };

  for (auto& chain_ints : intersections_a) dedup(chain_ints);
  for (auto& chain_ints : intersections_b) dedup(chain_ints);

  return {intersections_a, intersections_b};
}

// Helper function to slice the given chain at the given intersection points and
// extract the subchains that lie on the boundary of the shape_a - shape_b.
// `is_subtracted` indicates whether `chain` belongs to shape_a and
// `other_shape` is shape_b, or vice-versa.
void SliceChain(const MonotoneChain& chain,
                absl::InlinedVector<ChainIntersection, 2>& intersections,
                bool is_subtracted, const ShapeOutline& other_shape,
                std::vector<MonotoneChain>& raw_chains) {
  // The crossings of the chain with `other_shape` split up the chain into
  // pieces, which are alternately inside or outside other_shape. If the chain
  // belongs to shape_a, we keep the pieces that are outside the subtracted
  // shape_b, and if the chain belongs to shape_b, we keep the pieces that are
  // inside shape_a. To get the pieces, we traverse the chain while keeping
  // track of whether the current piece is kept or not by flipping its value at
  // every crossing.

  absl::Span<const Point> verts = chain.Vertices();

  // The orientation of resulting pieces. The subtracted chains will have
  // opposite orientation.
  int orientation = is_subtracted ? -chain.Orientation() : chain.Orientation();

  // The first crossing determines whether the chain is initially inside or
  // outside `other_shape`.
  auto first_crossing = absl::c_find_if(
      intersections, [](const ChainIntersection& c) { return c.sign != 0; });

  bool is_kept;
  if (first_crossing != intersections.end()) {
    is_kept = orientation == first_crossing->sign;
  } else {
    // If there are no crossings, then the chain is either contained entirely
    // in `other_shape` or entirely outside. Without any crossings, we have to
    // resort to full containment test for a suitable point of the chain.

    // If there are no intersections at all, then we can test any point on the
    // chain.
    if (intersections.empty()) {
      if (Intersects(other_shape, verts.front()) == is_subtracted) {
        raw_chains.push_back(MonotoneChain(
            std::vector<Point>(verts.begin(), verts.end()), orientation));
      }
      return;
    }

    // If there are (non-crossing) intersections, then testing them tells us
    // nothing since they are on the boundary of `other_shape`, so we have to
    // make sure to find a non-intersecting point.
    // If the first vertex is not an intersection point, we can just test it.
    // Otherwise, we test the midpoint between the first vertex and either the
    // first intersection on (or the endpoint of) the first segment.
    Point test_point = verts[0];
    if (intersections[0].pos == verts[0]) {
      Point next_point =
          (intersections.size() > 1 && intersections[1].vertex == 0)
              ? intersections[1].pos
              : verts[1];
      test_point = verts[0] + 0.5f * (next_point - verts[0]);
    }
    is_kept = Intersects(other_shape, test_point) == is_subtracted;
  }

  // Add a sentinel representing the end of the chain to avoid a separate
  // terminal check after the loop.
  if (intersections.back().pos != verts.back()) {
    intersections.push_back(
        {verts.back(), static_cast<uint32_t>(verts.size() - 2), 0});
  }

  // If an intersection occurs at the very start of the chain, it does not
  // produce a chain piece, so we immediately remove it.
  if (intersections[0].pos == verts.front()) {
    if (intersections[0].sign != 0) is_kept = !is_kept;
    intersections.erase(intersections.begin());
  }

  size_t curr_vert_idx = 0;
  Point start_pos = verts.front();

  // Traverse through the intersections on the chain.
  for (const ChainIntersection& cut : intersections) {
    // At every intersection, emit the piece terminating at the intersection,
    // if it should be kept.
    if (is_kept) {
      std::vector<Point> piece;
      piece.reserve(cut.vertex - curr_vert_idx + 2);

      // Push in the previous intersection point, but only if it's not at a
      // chain vertex, to avoid duplicating it.
      if (start_pos != verts[curr_vert_idx + 1]) piece.push_back(start_pos);

      // Copy the vertices of the chain in between the previous and current
      // intersections.
      piece.insert(piece.end(), verts.begin() + curr_vert_idx + 1,
                   verts.begin() + cut.vertex + 1);

      // Push in the current intersection point.
      if (cut.pos != piece.back()) piece.push_back(cut.pos);

      raw_chains.push_back({std::move(piece), orientation});
    }

    // Switch the state: if the intersection is transverse then the chain
    // switches being inside or outside other_shape.
    if (cut.sign != 0) is_kept = !is_kept;

    curr_vert_idx = cut.vertex;
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

std::vector<MonotoneChain> ComputeChains(
    const std::vector<std::vector<Point>>& loops) {
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
  return ExtractChains(segments);
}

std::vector<MonotoneChain> ComputeChains(const Triangle& triangle) {
  Point a = triangle.p0, b = triangle.p1, c = triangle.p2;
  // Cycle the vertices of the triangle so that the first is the left-most.
  if (IsLeftOrBelow(b, a) && IsLeftOrBelow(b, c)) {
    a = triangle.p1;
    b = triangle.p2;
    c = triangle.p0;
  } else if (IsLeftOrBelow(c, a)) {
    a = triangle.p2;
    b = triangle.p0;
    c = triangle.p1;
  }

  if (IsLeftOrBelow(b, c)) return {{{a, b, c}, 1}, {{a, c}, -1}};

  return {{{a, b}, 1}, {{a, c, b}, -1}};
}

}  // namespace

MonotoneChain::MonotoneChain(std::vector<Point> vertices, int orientation)
    : vertices_(std::move(vertices)),
      orientation_(orientation),
      bounds_(Envelope(vertices_).AsRect().value_or(Rect())) {
  if (!absl::c_is_sorted(vertices_, IsLeftOrBelow))
    absl::c_sort(vertices_, IsLeftOrBelow);
}

Point MonotoneChain::StartPoint() const {
  ABSL_DCHECK(!vertices_.empty());
  return orientation_ == 1 ? vertices_.front() : vertices_.back();
}

Point MonotoneChain::EndPoint() const {
  ABSL_DCHECK(!vertices_.empty());
  return orientation_ == 1 ? vertices_.back() : vertices_.front();
}

Point MonotoneChain::StartNeighbor() const {
  ABSL_DCHECK_GE(vertices_.size(), 2);
  return orientation_ == 1 ? vertices_[1] : vertices_.rbegin()[1];
}

Point MonotoneChain::EndNeighbor() const {
  ABSL_DCHECK_GE(vertices_.size(), 2);
  return orientation_ == 1 ? vertices_.rbegin()[1] : vertices_[1];
}

ShapeOutline::ShapeOutline(std::vector<MonotoneChain> input_chains)
    : chains_(std::move(input_chains)) {
  absl::c_sort(chains_, [](const MonotoneChain& a, const MonotoneChain& b) {
    return std::tuple(a.Bounds().YMin(), a.Bounds().XMin(), a.Bounds().YMax(),
                      a.Bounds().XMax(), a.Orientation()) <
           std::tuple(b.Bounds().YMin(), b.Bounds().XMin(), b.Bounds().YMax(),
                      b.Bounds().XMax(), b.Orientation());
  });
  std::tie(prev_index_, next_index_) = GetAdjacentChains(chains_);
}

ShapeOutline::ShapeOutline(const std::vector<std::vector<Point>>& loops)
    : ShapeOutline(ComputeChains(loops)) {}

ShapeOutline::ShapeOutline(const Triangle& triangle)
    : ShapeOutline(ComputeChains(triangle)) {}

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
    int det_sign = Sign(Determinant(pL, p, pR));
    if (det_sign == 0) return true;
    if (det_sign < 0) crossings += 1;
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
    if (!IntersectsInternal(chain.Bounds(), rect)) continue;

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
        IntersectsInternal(rect, Segment{*(left_it - 1), *left_it})) {
      return true;
    }
    if (right_it != pts.end() && right_it != left_it &&
        IntersectsInternal(rect, Segment{*(right_it - 1), *right_it})) {
      return true;
    }
  }

  // If no intersections, check if rect is contained in shape.
  return Intersects(shape, rect.Center());
}

std::vector<std::vector<Point>> ComputeBoundaryLoops(
    const ShapeOutline& shape) {
  // We stitch the monotone chains into closed loops by following each chain's
  // "next" chain in the shape's oriented boundary until the loop is closed,
  // and then move on to the next closed loop.

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
      curr = shape.NextIndex(curr);
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
  // TODO(b/521449017): Handle triangulating shapes with holes. For now, ignore
  // inner holes.
  std::erase_if(loops, [](const auto& loop) { return Area(loop) < 0; });

  size_t total_triangles = 0;
  for (const auto& loop : loops) {
    if (loop.size() >= 3) {
      total_triangles += loop.size() - 2;
    }
  }
  triangles.reserve(total_triangles);

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
  if (shape_a.Chains().empty()) return ShapeOutline();
  if (shape_b.Chains().empty()) return shape_a;

  // To compute the subtraction of shape_b from shape_a, we first compute all
  // intersection points between the boundaries of the two shapes. We then
  // subdivide the boundary chains of both shapes, and extract the fragments
  // that lie on the boundary of (shape_a - shape_b). Finally, we stitch the
  // fragments back together to form the chains of the final result.

  auto [intx_a, intx_b] = FindBoundaryIntersections(shape_a, shape_b);
  std::vector<MonotoneChain> raw_chains =
      SubdivideIntersectionChains(shape_a, shape_b, intx_a, intx_b);
  return ShapeOutline(StitchIntersectionChains(std::move(raw_chains)));
}

}  // namespace ink::geometry_internal
