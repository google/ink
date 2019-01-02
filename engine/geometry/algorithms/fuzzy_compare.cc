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

#include "ink/engine/geometry/algorithms/intersect.h"

namespace ink {
namespace geometry {

bool Equivalent(const Rect &rect1, const Rect &rect2, const float tolerance) {
  if (tolerance >= 1.0) return true;

  Rect intersection;
  if (!Intersection(rect1, rect2, &intersection)) return false;
  const float overlap = intersection.Area();
  if (overlap == 0.0) return false;

  return 1.0 - overlap / rect1.Area() <= tolerance &&
         1.0 - overlap / rect2.Area() <= tolerance;
}

}  // namespace geometry
}  // namespace ink
