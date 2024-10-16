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

#ifndef INK_GEOMETRY_INTERNAL_POLYLINE_PROCESSING_H_
#define INK_GEOMETRY_INTERNAL_POLYLINE_PROCESSING_H_

#include <stdbool.h>

#include <vector>

#include "absl/types/span.h"
#include "ink/geometry/point.h"
#include "ink/geometry/segment.h"

namespace ink::geometry_internal {

struct SegmentBundle {
  Segment segment;
  int index;
  float length;
};

struct Intersection {
  int index_int;
  float index_fraction;
  float walk_distance;
};

struct PolylineData {
  std::vector<SegmentBundle> segments;
  Intersection first_intersection;
  Intersection last_intersection;
  Point new_first_point;
  Point new_last_point;
  bool connect_first = false;
  bool connect_last = false;
  bool has_intersection = false;
  float min_walk_distance;
  float max_connection_distance;
  float min_connection_ratio;
  float min_trimming_ratio;
};

PolylineData CreateNewPolylineData(absl::Span<const Point> points);

float WalkDistance(PolylineData& polyline, int index, float fractional_index,
                   bool walk_backwards);

}  // namespace ink::geometry_internal

#endif  // INK_GEOMETRY_INTERNAL_POLYLINE_PROCESSING_H_
