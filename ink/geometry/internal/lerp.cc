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

#include "ink/geometry/internal/lerp.h"

#include <cmath>
#include <utility>

#include "ink/color/color.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"

namespace ink::geometry_internal {

float Lerp(float a, float b, float t) {
  // TODO: b/457491215 - In libc++, `std::lerp(a, b, t)` returns NaN if `a` and
  // `b` are finite, `t` is infinite, and exactly one of `a` or `b` is
  // 0. However, the standard explicitly requires it not to return NaN in that
  // case, so it should return ±inf, depending on the signs of `b - a` and `t`.
  // See https://en.cppreference.com/w/cpp/numeric/lerp.html for spec, and
  // https://github.com/llvm/llvm-project/issues/166628 for the libc++ issue.
  //
  // We have fuzz tests that enforce that our `Lerp` behaves the way that
  // `std::lerp` is *supposed* to behave, so until `std::lerp` is fixed, this if
  // block is necessary to intercept the erroneous case.
  if (std::isinf(t)) {
    return t * (b - a);
  }
  return std::lerp(a, b, t);
}

Color::RgbaFloat Lerp(const Color::RgbaFloat& a, const Color::RgbaFloat& b,
                      float t) {
  return {.r = Lerp(a.r, b.r, t),
          .g = Lerp(a.g, b.g, t),
          .b = Lerp(a.b, b.b, t),
          .a = Lerp(a.a, b.a, t)};
}

Angle NormalizedAngleLerp(Angle a, Angle b, float t) {
  return (a + Lerp(Angle(), (b - a).NormalizedAboutZero(), t)).Normalized();
}

float InverseLerp(float a, float b, float value) {
  // If the interval between `a` and `b` is 0, there is no way to get to `t`
  // because in the other direction the value of `t` won't impact the result.
  if (b - a == 0.f) {
    return 0.f;
  }
  return (value - a) / (b - a);
}

float LinearMap(float input_value, std::pair<float, float> input_range,
                std::pair<float, float> output_range) {
  return Lerp(output_range.first, output_range.second,
              InverseLerp(input_range.first, input_range.second, input_value));
}

}  // namespace ink::geometry_internal
