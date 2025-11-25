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

#include "ink/strokes/internal/modeled_stroke_input.h"

#include "ink/geometry/internal/lerp.h"

namespace ink::strokes_internal {

using ::ink::geometry_internal::Lerp;
using ::ink::geometry_internal::NormalizedAngleLerp;

ModeledStrokeInput Lerp(const ModeledStrokeInput& a,
                        const ModeledStrokeInput& b, float t) {
  return {
      .position = Lerp(a.position, b.position, t),
      .velocity = Lerp(a.velocity, b.velocity, t),
      .acceleration = Lerp(a.acceleration, b.acceleration, t),
      .traveled_distance = Lerp(a.traveled_distance, b.traveled_distance, t),
      .elapsed_time = Lerp(a.elapsed_time, b.elapsed_time, t),
      .pressure = Lerp(a.pressure, b.pressure, t),
      .tilt = Lerp(a.tilt, b.tilt, t),
      .orientation = NormalizedAngleLerp(a.orientation, b.orientation, t),
  };
}

}  // namespace ink::strokes_internal
