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

#include "ink/engine/geometry/primitives/triangle.h"

#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace geometry {

bool Triangle::IsDegenerate() const {
  // Due to the precision allowance in Orientation(), the triangle's vertices
  // may be considered collinear w.r.t. to one side of the triangle, but not
  // w.r.t. to another side. So, we must check the orientation w.r.t. all sides.
  for (int i = 0; i < 3; ++i) {
    auto orientation =
        Orientation(points_[i], points_[(i + 1) % 3], points_[(i + 2) % 3]);
    if (orientation == RelativePos::kCollinear ||
        orientation == RelativePos::kIndeterminate)
      return true;
  }

  return false;
}

bool Triangle::Contains(glm::vec2 test_point) const {
  if (test_point == points_[0] || test_point == points_[1] ||
      test_point == points_[2])
    return true;

  auto orientation1 = Orientation(points_[0], points_[1], test_point);
  auto orientation2 = Orientation(points_[1], points_[2], test_point);
  if (orientation1 == ReverseOrientation(orientation2)) return false;

  auto orientation3 = Orientation(points_[2], points_[0], test_point);
  return orientation1 != ReverseOrientation(orientation3) &&
         orientation2 != ReverseOrientation(orientation3);
}

}  // namespace geometry
}  // namespace ink
