// Copyright 2025 Google LLC
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

#include "ink/strokes/internal/fuzz_domains.h"

#include "fuzztest/fuzztest.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/fuzz_domains.h"
#include "ink/strokes/internal/brush_tip_state.h"

namespace ink::strokes_internal {

fuzztest::Domain<BrushTipState> ValidBrushTipState() {
  return fuzztest::StructOf<BrushTipState>(
      /*position=*/ArbitraryPoint(),
      /*width=*/fuzztest::NonNegative<float>(),
      /*height=*/fuzztest::NonNegative<float>(),
      /*percent_radius=*/fuzztest::InRange<float>(0, 1),
      /*rotation=*/NormalizedAngle(),
      /*slant=*/AngleInRange(-kHalfTurn, kHalfTurn),
      /*pinch=*/fuzztest::InRange<float>(0, 1),
      /*texture_animation_progress_offset=*/fuzztest::InRange<float>(0, 1),
      /*hue_offset_in_full_turns=*/fuzztest::InRange<float>(0, 1),
      /*saturation_multiplier=*/fuzztest::InRange<float>(0, 2),
      /*luminosity_shift=*/fuzztest::InRange<float>(-1, 1),
      /*opacity_multiplier=*/fuzztest::InRange<float>(0, 2));
}

}  // namespace ink::strokes_internal
