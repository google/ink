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

#ifndef INK_STROKES_STROKE_PAINT_ANIMATION_CLOCK_H_
#define INK_STROKES_STROKE_PAINT_ANIMATION_CLOCK_H_

#include <cstdint>

#include "absl/time/time.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"

namespace ink {

// Given a whole-stroke animation duration, calculates the [0, 1) base paint
// animation phase value for the stroke. This is the paint animation progress
// value that the stroke would use if the paint animation clock state were zero,
// such that the stroke is at the start of its animation at the current clock
// state.
//
// If `brush_family_loop_duration_millis` is zero, indicating that the stroke's
// `BrushFamily` contains no animated `BrushPaint`s, then this function returns
// zero.
float CalculateBasePaintAnimationPhaseForNewStroke(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis);

float CalculateBasePaintAnimationPhaseForNewStroke(
    absl::Duration clock_state, absl::Duration brush_family_loop_duration);

// Given a stroke's base paint animation phase, its `BrushFamily`'s paint
// animation loop duration, and the paint animation loop duration for a
// particular `BrushPaint` in that family, returns the [0, 1) phase value that
// `BrushPaint` should have at the given clock state.
//
// If `brush_paint_loop_duration_millis` is zero, indicating that the
// `BrushPaint` is not animated, then this function returns zero. Otherwise,
// `brush_family_loop_duration_millis` must be a non-negative multiple of
// `brush_paint_loop_duration_millis`.
float CalculateCurrentAnimationPhaseForBrushPaint(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis,
    int64_t brush_paint_loop_duration_millis,
    float stroke_base_paint_animation_phase);

float CalculateCurrentAnimationPhaseForBrushPaint(
    absl::Duration clock_state, const BrushFamily& brush_family,
    const BrushPaint& brush_paint, float stroke_base_paint_animation_phase);

}  // namespace ink

#endif  // INK_STROKES_STROKE_PAINT_ANIMATION_CLOCK_H_
