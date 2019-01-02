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

#include "ink/engine/geometry/primitives/angle_utils.h"

#include "ink/engine/util/funcs/utils.h"

namespace ink {

void FixAngles(float *start_ang, float *end_ang, bool make_clockwise) {
  // Normalize the angles.
  *start_ang = util::Mod(*start_ang, M_TAU);
  *end_ang = util::Mod(*end_ang, M_TAU);

  // Clockwise angles are negative, counterclockwise angles are positive.
  // This means that the starting angle should be larger when we're clockwise,
  // so that endAng - startAng < 0, and vice versa.

  // If we're clockwise, ensure that startAng >= endAng
  if (make_clockwise && *start_ang < *end_ang) {
    *start_ang += M_TAU;
  }

  // If we're counterclockwise, ensure that endAng <= startAng
  if (!make_clockwise && *start_ang > *end_ang) {
    *end_ang += M_TAU;
  }
}

}  // namespace ink
