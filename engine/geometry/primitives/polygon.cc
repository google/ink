// Copyright 2018 Google LLC
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

#include "ink/engine/geometry/primitives/polygon.h"

#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {
namespace geometry {

int Polygon::WindingNumber(glm::vec2 point) const {
  // For each segment, determine whether it crosses the ray starting at the
  // given point, and extending in the positive x-direction. If it crosses,
  // increment or decrement the winding number, based on which direction it is
  // travelling.
  int winding_number = 0;
  glm::vec2 segment_start = points_.back();
  for (const auto& segment_end : points_) {
    if (segment_start.y <= point.y) {
      if (segment_end.y > point.y &&
          Orientation(segment_start, segment_end, point) == RelativePos::kLeft)
        ++winding_number;
    } else if (segment_end.y <= point.y &&
               Orientation(segment_start, segment_end, point) ==
                   RelativePos::kRight) {
      --winding_number;
    }
    segment_start = segment_end;
  }

  return winding_number;
}

float Polygon::SignedArea() const {
  float signed_area = 0;
  for (int i = 1; i < Size() - 1; ++i)
    signed_area +=
        Triangle(points_[0], points_[i], points_[i + 1]).SignedArea();
  return signed_area;
}

}  // namespace geometry
}  // namespace ink
