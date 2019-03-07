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

#include "ink/engine/geometry/primitives/rot_rect.h"

#include <cstdlib>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtx/rotate_vector.hpp"

namespace ink {

// static
RotRect RotRect::WithCorners(glm::vec2 corner1, glm::vec2 corner2,
                             float rotation_radians) {
  glm::vec2 center = (corner1 + corner2) / 2.0f;
  glm::vec2 east_unit = glm::rotate(glm::vec2{1, 0}, rotation_radians);
  glm::vec2 north_unit = glm::rotate(glm::vec2{0, 1}, rotation_radians);
  glm::vec2 dim{std::abs(glm::dot(corner1 - corner2, east_unit)),
                std::abs(glm::dot(corner1 - corner2, north_unit))};
  return RotRect(center, dim, rotation_radians);
}

RotRect RotRect::InteriorRotRectWithAspectRatio(
    float target_aspect_ratio) const {
  float current_aspect_ratio = AspectRatio();
  float corrected_width;
  float corrected_height;
  if (target_aspect_ratio > current_aspect_ratio) {
    corrected_width = Width();
    corrected_height = Width() / target_aspect_ratio;
  } else {
    corrected_height = Height();
    corrected_width = Height() * target_aspect_ratio;
  }
  return RotRect(Center(), {corrected_width, corrected_height}, Rotation());
}

}  // namespace ink
