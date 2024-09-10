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

#include "ink/geometry/angle.h"

#include <cmath>
#include <string>

#include "absl/strings/str_format.h"
#include "ink/types/numbers.h"

namespace ink {

Angle Angle::Normalized() const {
  float radians =
      std::fmod(ValueInRadians(), static_cast<float>(2.0 * numbers::kPi));
  // fmod always matches the sign of the first argument, so fmod(angle, 2π)
  // returns a value in the range (-2π, 2π).  We want [0, 2π), so wrap negative
  // values back to positive.
  if (radians < 0.f) {
    radians += static_cast<float>(2.0 * numbers::kPi);
    // If fmod returned a sufficiently small negative number, then adding 2π
    // will give us a result exactly equal to 2π, due to float rounding.
    // However, Angle::Normalized() promises to return a value strictly less
    // than 2π, so we should normalize to zero in this case.
    if (radians == static_cast<float>(2.0 * numbers::kPi)) {
      radians = 0.f;
    }
  }
  return Angle::Radians(radians);
}

Angle Angle::NormalizedAboutZero() const {
  float radians =
      std::fmod(ValueInRadians(), static_cast<float>(2.0 * numbers::kPi));
  return Angle::Radians(radians > static_cast<float>(numbers::kPi)
                            ? radians - static_cast<float>(2.0 * numbers::kPi)
                        : radians <= static_cast<float>(-numbers::kPi)
                            ? radians + static_cast<float>(2.0 * numbers::kPi)
                            : radians);
}

std::string Angle::ToFormattedString() const {
  return absl::StrFormat("%vπ",
                         ValueInRadians() / static_cast<float>(numbers::kPi));
}

}  // namespace ink
