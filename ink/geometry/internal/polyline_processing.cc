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

#include <vector>

#include "absl/types/span.h"
#include "ink/geometry/distance.h"
#include "ink/geometry/point.h"
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

}  // namespace ink::geometry_internal
