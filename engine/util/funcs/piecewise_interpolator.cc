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

#include "ink/engine/util/funcs/piecewise_interpolator.h"

#include <algorithm>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

using glm::vec2;
using ink::util::Lerp;
using ink::util::Normalize;

PiecewiseInterpolator::PiecewiseInterpolator(const std::vector<vec2> &points) {
  ASSERT(!points.empty() && points.size() < 20);
  points_ = points;
  std::sort(points_.begin(), points_.end(),
            [](vec2 a, vec2 b) -> bool { return a.x < b.x; });
}

float PiecewiseInterpolator::GetValue(float x) const {
  if (x < points_.front().x) return points_.front().y;

  for (int i = 1; i < points_.size(); i++) {
    if (points_[i].x > x) {
      // interpolate between points_[i-1] and points_[i].
      return Lerp(points_[i - 1].y, points_[i].y,
                  Normalize(points_[i - 1].x, points_[i].x, x));
    }
  }

  return points_.back().y;
}

}  // namespace ink
