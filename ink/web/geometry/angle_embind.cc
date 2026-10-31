// Copyright 2026 Google LLC
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

#include <emscripten/bind.h>

#include "ink/geometry/angle.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_angle_bindings) {
  emscripten::class_<Angle>("Angle")
      .constructor<>()
      .class_function("radians", &Angle::Radians)
      .class_function("degrees", &Angle::Degrees)
      .function("valueInRadians", &Angle::ValueInRadians)
      .function("valueInDegrees", &Angle::ValueInDegrees)
      .function("normalized", &Angle::Normalized)
      .function("normalizedAboutZero", &Angle::NormalizedAboutZero);
}

}  // namespace
}  // namespace ink::web
