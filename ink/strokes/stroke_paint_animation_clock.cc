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

#include "ink/strokes/stroke_paint_animation_clock.h"

#include <cmath>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/time/time.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"

namespace ink {
namespace {

int64_t PositiveModulo(int64_t a, int64_t b) {
  ABSL_DCHECK_GT(b, 0);
  int64_t rem = a % b;
  return rem < 0 ? rem + b : rem;
}

}  // namespace

float CalculateBasePaintAnimationPhaseForNewStroke(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis) {
  ABSL_CHECK_GE(brush_family_loop_duration_millis, 0);
  if (brush_family_loop_duration_millis == 0) {
    return 0.0f;
  }
  int64_t phase_millis =
      PositiveModulo(-(clock_state_millis % brush_family_loop_duration_millis),
                     brush_family_loop_duration_millis);
  return static_cast<float>(phase_millis) /
         static_cast<float>(brush_family_loop_duration_millis);
}

float CalculateBasePaintAnimationPhaseForNewStroke(
    absl::Duration clock_state, absl::Duration brush_family_loop_duration) {
  return CalculateBasePaintAnimationPhaseForNewStroke(
      absl::ToInt64Milliseconds(clock_state),
      absl::ToInt64Milliseconds(brush_family_loop_duration));
}

float CalculateCurrentAnimationPhaseForBrushPaint(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis,
    int64_t brush_paint_loop_duration_millis,
    float stroke_base_paint_animation_phase) {
  ABSL_CHECK_GE(brush_family_loop_duration_millis,
                brush_paint_loop_duration_millis);
  ABSL_CHECK_GE(brush_paint_loop_duration_millis, 0);
  if (brush_paint_loop_duration_millis == 0) {
    return 0.0f;
  }
  ABSL_CHECK_EQ(PositiveModulo(brush_family_loop_duration_millis,
                               brush_paint_loop_duration_millis),
                0);
  int64_t base_offset_millis =
      std::llround(static_cast<double>(stroke_base_paint_animation_phase) *
                   static_cast<double>(brush_family_loop_duration_millis));
  int64_t phase_millis = PositiveModulo(
      (clock_state_millis % brush_paint_loop_duration_millis) +
          (base_offset_millis % brush_paint_loop_duration_millis),
      brush_paint_loop_duration_millis);
  return static_cast<float>(phase_millis) /
         static_cast<float>(brush_paint_loop_duration_millis);
}

float CalculateCurrentAnimationPhaseForBrushPaint(
    absl::Duration clock_state, const BrushFamily& brush_family,
    const BrushPaint& brush_paint, float stroke_base_paint_animation_phase) {
  return CalculateCurrentAnimationPhaseForBrushPaint(
      absl::ToInt64Milliseconds(clock_state),
      absl::ToInt64Milliseconds(brush_family.GetPaintAnimationLoopDuration()),
      absl::ToInt64Milliseconds(
          brush_internal::CalculatePaintAnimationLoopDuration(brush_paint)),
      stroke_base_paint_animation_phase);
}

}  // namespace ink
