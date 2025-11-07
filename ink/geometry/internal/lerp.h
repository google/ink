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

#ifndef INK_GEOMETRY_INTERNAL_LERP_H_
#define INK_GEOMETRY_INTERNAL_LERP_H_

#include <array>
#include <optional>
#include <utility>

#include "ink/color/color.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/types/duration.h"

namespace ink::geometry_internal {

// Linearly interpolates between `a` and `b`. Extrapolates when `t` is not in
// [0, 1].
//
// In the case where `a` == `b` the function will return `a` for any finite
// value of `t`.
//
// Note that the `Angle` overload simply interpolates the value of the `Angle`;
// it does not have any special case logic for congruent angles. I.e., for
// `Angle`s that differ by more than 2π, this will interpolate through one (or
// more) full rotations, and for `Angle`s that differ by less than 2π, this
// may interpolate the "long way" around the unit circle. If you require that
// behavior, you can achieve it by normalizing the `Angle`s w.r.t. a reference
// `Angle` (see also `Angle::Normalized` and `Angle::NormalizedAboutZero`).
float Lerp(float a, float b, float t);
Point Lerp(Point a, Point b, float t);
Color::RgbaFloat Lerp(const Color::RgbaFloat& a, const Color::RgbaFloat& b,
                      float t);
Angle Lerp(Angle a, Angle b, float t);
Vec Lerp(Vec a, Vec b, float t);
Duration32 Lerp(Duration32 a, Duration32 b, float t);

// Linearly interpolates between `a` and `b` in the shorter direction between
// the two angles and returns a value in range [0, 2pi).
Angle NormalizedAngleLerp(Angle a, Angle b, float t);

// Linearly rescales `t` relative to `a` and `b`, such that `a` maps to 0, and
// `b` maps to 1. If `value` is between `a` and `b`, the result will lie in the
// interval [0, 1].
//
// If `a` == `b` this function will return 0, for any `value`.
float InverseLerp(float a, float b, float value);

// Linearly maps an `input_value` from an `input_range` to an`output_range` such
// that `input_range.first` maps to `output_range.first` and
// `input_range.second` maps to `output_range.second`.
float LinearMap(float input_value, std::pair<float, float> input_range,
                std::pair<float, float> output_range);

////////////////////////////////////////////////////////////////////////////////
// Inline function definitions
////////////////////////////////////////////////////////////////////////////////

inline Angle Lerp(Angle a, Angle b, float t) {
  return Angle::Radians(Lerp(a.ValueInRadians(), b.ValueInRadians(), t));
}

inline Duration32 Lerp(Duration32 a, Duration32 b, float t) {
  return Duration32::Seconds(Lerp(a.ToSeconds(), b.ToSeconds(), t));
}

inline Point Lerp(Point a, Point b, float t) {
  return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t)};
}

inline Vec Lerp(Vec a, Vec b, float t) {
  return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t)};
}

}  // namespace ink::geometry_internal

#endif  // INK_GEOMETRY_INTERNAL_LERP_H_
