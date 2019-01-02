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

#include "ink/engine/geometry/line/distance_field.h"

#include <algorithm>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

DistanceField::DistanceField() : buffer_size_(5), min_dist_to_add_(1) {}

DistanceField::DistanceField(size_t buffer_size, float min_dist_to_add)
    : buffer_size_(buffer_size), min_dist_to_add_(min_dist_to_add) {}

void DistanceField::AddPt(glm::vec2 pt) {
  if (linepts_.empty() ||
      geometry::Distance(linepts_.back(), pt) >= min_dist_to_add_) {
    linepts_.push_back(pt);
  }
  if (linepts_.size() > buffer_size_) {
    auto oldpts =
        std::vector<glm::vec2>(linepts_.end() - buffer_size_, linepts_.end());
    linepts_ = oldpts;
  }
}

void DistanceField::Clear() { linepts_.clear(); }

float DistanceField::Distance(glm::vec2 pt) {
  if (linepts_.empty()) {
    ASSERT(false);
    return 0;
  }
  if (linepts_.size() == 1) {
    auto v = Vertex(pt);
    v.color = glm::vec4(1, 0, 1, 1);

    return geometry::Distance(pt, linepts_[0]);
  }

  auto gdist = geometry::Distance(pt, linepts_);
  return gdist;
}
}  // namespace ink
