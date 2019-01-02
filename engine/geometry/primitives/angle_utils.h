/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_ANGLE_UTILS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_ANGLE_UTILS_H_

#include "ink/engine/math_defines.h"

namespace ink {

// "Fixes" two angles such that the distance between them is < 2π and that they
// are in the right order in order to respect the makeClockwise parameter.
// This effectively adjusts the angles so that pointsOnCircle can be called with
// the result and have it be guaranteed to either be clockwise or not, depending
// on the parameter.
//
// In our case, clockwise means negative angles, and counter-clockwise means
// positive angles, so some examples (in degrees) would be:
// (0°, 90°, true) -> (360°, 90°)
// (90°, 0°, true) -> (90°, 0°)
// (0°, 90°, false) -> (0°, 90°)
// (90°, 0°, false) -> (90°, 360°)
//
// Keep in mind that this will not be the exact angles that are returned, but
// the angles will be the same mod 360°.
void FixAngles(float *start_ang, float *end_ang, bool make_clockwise);

// Returns the equivalent angle (in radians) in the interval [0, 2π).
inline float NormalizeAnglePositive(float angle) {
  constexpr float kFloatTau = M_TAU;
  while (angle < 0) angle += kFloatTau;
  while (angle >= kFloatTau) angle -= kFloatTau;
  return angle;
}

// Returns the equivalent angle (in radians) in the interval (-π, π].
inline float NormalizeAngle(float angle) {
  constexpr float kFloatPi = M_PI;
  constexpr float kFloatTau = M_TAU;
  angle = NormalizeAnglePositive(angle);
  if (angle > kFloatPi) angle -= kFloatTau;
  return angle;
}

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_ANGLE_UTILS_H_
