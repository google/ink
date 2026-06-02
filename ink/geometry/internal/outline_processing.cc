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
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/point.h"

namespace ink::geometry_internal {

// TODO(b/509625875): Improve numerical robustness -- it's common
// in line-sweep algorithms to quantize points to a grid to avoid precision
// instability.
// TODO(b/509625875): Handle degenerate intersections, including overlapping
// segments, T-junctions, simultaneous intersections of more than two segments,
// etc.

namespace {

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
      if (IsLeftOrBelow(u, v))
        segments.push_back({u, v, 1});
      else
        segments.push_back({v, u, -1});
      u = v;
    }
  }

  std::sort(segments.begin(), segments.end());

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
    std::sort(cuts.begin(), cuts.end(), IsLeftOrBelow);
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
// extracts the geometric boundary of the polygon (the region where the winding
// number w > 0) as a set of x-monotone chains oriented counter-clockwise.
std::vector<std::vector<Point>> ExtractChains(
    const std::vector<BoundarySegment>& segments) {
  if (segments.empty()) return {};

  absl::InlinedVector<const BoundarySegment*, 16> active_segments;

  // A priority queue of ends of the active_segments.
  std::priority_queue<float, std::vector<float>, std::greater<>> end_xs;

  // For simplicity, we'll construct and store the
  // negatively oriented (the decreasing chains) left-to-right and reverse
  // them at the end.
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
    std::sort(active_segments.begin(), active_segments.end(),
              [&](const BoundarySegment* a, const BoundarySegment* b) {
                float y_a = ComputeY(*a, mid_x);
                float y_b = ComputeY(*b, mid_x);
                return y_a < y_b;
              });

    current_x = next_x;
  }

  // Move pos chains directly to result, reverse neg chains and move to result.
  std::vector<std::vector<Point>> result;
  result.reserve(pos_chains.size() + neg_chains.size());
  for (auto& chain : pos_chains) result.push_back(std::move(chain));
  for (auto& chain : neg_chains) {
    std::reverse(chain.begin(), chain.end());
    result.push_back(std::move(chain));
  }
  return result;
}

}  // namespace

std::vector<std::vector<Point>> ComputeMonotoneBoundaryChains(
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

}  // namespace ink::geometry_internal
