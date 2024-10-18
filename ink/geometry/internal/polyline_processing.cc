// Copyright 2024 Google LLC
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

#include "ink/geometry/internal/polyline_processing.h"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "absl/types/span.h"
#include "ink/geometry/distance.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/internal/static_rtree.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"

namespace ink::geometry_internal {

float WalkDistance(PolylineData& polyline, int index, float fractional_index,
                   bool walk_backwards) {
  int start_idx;
  int end_idx;
  if (walk_backwards) {
    fractional_index = 1.0 - fractional_index;
    start_idx = index + 1;
    end_idx = polyline.segments.size();
  } else {
    start_idx = 0;
    end_idx = index;
  }

  float total_distance = polyline.segments[index].length * fractional_index;
  for (int i = start_idx; i < end_idx; ++i) {
    total_distance += polyline.segments[i].length;
  }
  return total_distance;
}

PolylineData CreateNewPolylineData(absl::Span<const Point> points) {
  PolylineData polyline;
  polyline.segments.reserve(points.size() - 1);
  for (int i = 0; i < static_cast<int>(points.size()) - 1; ++i) {
    polyline.segments.push_back(
        SegmentBundle{Segment{points[i], points[i + 1]}, i,
                      Distance(points[i], points[i + 1])});
  }
  return polyline;
}

void FindFirstAndLastIntersections(
    ink::geometry_internal::StaticRTree<SegmentBundle> rtree,
    PolylineData& polyline) {
  SegmentBundle earliest_intersected_segment;
  std::pair<float, float> earliest_intersection_ratios =
      std::make_pair(std::numeric_limits<float>::infinity(), 0.0f);
  for (int i = 0; i < static_cast<int>(polyline.segments.size()) - 2; ++i) {
    // -2 because if we reach the last 2 segments without finding an
    // Intersection then none are present as the last two segments cant
    // intersect each other.
    SegmentBundle current_segment_bundle = polyline.segments[i];
    rtree.VisitIntersectedElements(
        Rect::FromTwoPoints(current_segment_bundle.segment.start,
                            current_segment_bundle.segment.end),
        [&earliest_intersected_segment, &earliest_intersection_ratios,
         &current_segment_bundle](SegmentBundle other_segment) {
          // Only check segments that are after the next segment.
          if (other_segment.index > current_segment_bundle.index + 1) {
            if (std::optional<std::pair<float, float>> intersection_ratios =
                    SegmentIntersectionRatio(current_segment_bundle.segment,
                                             other_segment.segment);
                intersection_ratios.has_value()) {
              if (intersection_ratios->first <
                  earliest_intersection_ratios.first) {
                earliest_intersected_segment = other_segment;
                earliest_intersection_ratios = *intersection_ratios;
              }
            }
          }
          return true;
        });
    if (earliest_intersection_ratios.first <= 1.0f) {
      // We have found an Intersection at the current segment, so we can stop
      // looking and fill in polyline.first_intersection.
      polyline.first_intersection.index_int = i;
      polyline.first_intersection.index_fraction =
          earliest_intersection_ratios.first;
      polyline.first_intersection.walk_distance =
          WalkDistance(polyline, polyline.first_intersection.index_int,
                       polyline.first_intersection.index_fraction, false);
      polyline.new_first_point = current_segment_bundle.segment.Lerp(
          earliest_intersection_ratios.first);

      // This is also the last intersection we have found so far, so fill in
      // polyline.last_intersection.
      polyline.last_intersection.index_int = earliest_intersected_segment.index;
      polyline.last_intersection.index_fraction =
          earliest_intersection_ratios.second;
      polyline.last_intersection.walk_distance =
          WalkDistance(polyline, polyline.last_intersection.index_int,
                       polyline.last_intersection.index_fraction, true);
      polyline.new_last_point = polyline.new_first_point;
      polyline.has_intersection = true;
      break;
    }
  }

  // If we didn't find an Intersection, then we can return early since we don't
  // need to check again in the other direction.
  if (!polyline.has_intersection) return;

  // This is very similar to the loop above, with some modifications made as
  // we are now starting at the back and looking for the latest Intersection.
  float largest_intersection_ratio = -std::numeric_limits<float>::infinity();
  for (int i = polyline.segments.size() - 1;
       i >= polyline.last_intersection.index_int; --i) {
    // Start from the back of the polyline and only check as long as the segment
    // you are checking has an index higher than the current last intersection
    // -1.
    SegmentBundle current_segment_bundle = polyline.segments[i];
    rtree.VisitIntersectedElements(
        Rect::FromTwoPoints(current_segment_bundle.segment.start,
                            current_segment_bundle.segment.end),
        [&polyline, &largest_intersection_ratio,
         &current_segment_bundle](SegmentBundle other_segment) {
          // Only check segments that are before the preceding segment.
          // Only check segments with an index that is >= the first
          // intersection index, since we know there are no intersections before
          // the first intersection.
          if (other_segment.index < current_segment_bundle.index - 1 &&
              other_segment.index > polyline.first_intersection.index_int - 1) {
            if (std::optional<std::pair<float, float>> intersection_ratios =
                    SegmentIntersectionRatio(current_segment_bundle.segment,
                                             other_segment.segment);
                intersection_ratios.has_value()) {
              if (intersection_ratios->first > largest_intersection_ratio) {
                largest_intersection_ratio = intersection_ratios->first;
              }
            }
          }
          return true;
        });
    if (largest_intersection_ratio >= 0.0f) {
      polyline.last_intersection.index_int = i;
      polyline.last_intersection.index_fraction = largest_intersection_ratio;
      polyline.new_last_point =
          current_segment_bundle.segment.Lerp(largest_intersection_ratio);
      polyline.last_intersection.walk_distance =
          WalkDistance(polyline, polyline.last_intersection.index_int,
                       polyline.last_intersection.index_fraction, true);
      break;
    }
  }
}

}  // namespace ink::geometry_internal
