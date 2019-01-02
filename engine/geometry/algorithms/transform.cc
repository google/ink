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

#include "ink/engine/geometry/algorithms/transform.h"

#include <cmath>
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {
namespace geometry {

RotRect Transform(const RotRect &rectangle, const glm::mat4 &matrix) {
  // Transform the center and axes.
  auto center = Transform(rectangle.Center(), matrix);
  glm::vec2 original_axis{std::cos(rectangle.Rotation()),
                          std::sin(rectangle.Rotation())};
  auto axis1 = glm::normalize(glm::mat2(matrix) * original_axis);
  auto axis2 = glm::normalize(glm::mat2(matrix) * Orthogonal(original_axis));
  if (rectangle.Dim().y < 0) axis2 *= -1;

  // Determine which corner will be further from the center once transformed.
  int corner_index = glm::dot(axis1, axis2) > 0 ? 0 : 1;
  auto corner_vector =
      Transform(rectangle.Corners()[corner_index], matrix) - center;

  // Use the corner to determine the minimum dimensions w.r.t. each axis. Recall
  // that a·b = ‖a‖·‖b‖·cosθ.
  auto dim1 =
      2.0f * glm::abs(glm::vec2{glm::dot(axis1, corner_vector),
                                glm::dot(Orthogonal(axis1), corner_vector)});
  auto dim2 =
      2.0f * glm::abs(glm::vec2{glm::dot(Orthogonal(axis2), corner_vector),
                                glm::dot(axis2, corner_vector)});

  // Choose the smaller dimensions and correct the orientation.
  glm::vec2 dim{0, 0};
  float angle_radians = 0;
  if (std::abs(dim1.x * dim1.y) <= std::abs(dim2.x * dim2.y)) {
    dim = dim1;
    angle_radians = VectorAngle(axis1);
    if (Determinant(axis1, axis2) < 0) dim.y = -dim.y;
  } else {
    dim = dim2;
    angle_radians = VectorAngle(axis2);
    if (Determinant(axis1, axis2) < 0) {
      dim.y = -dim.y;
      angle_radians += .5f * M_PI;
    } else {
      angle_radians -= .5f * M_PI;
    }
  }

  return RotRect(center, dim, angle_radians);
}

}  // namespace geometry
}  // namespace ink
