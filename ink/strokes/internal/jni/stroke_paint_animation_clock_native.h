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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_PAINT_ANIMATION_CLOCK_NATIVE_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_PAINT_ANIMATION_CLOCK_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float StrokePaintAnimationClockNative_calculateBasePaintAnimationPhase(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis);

float StrokePaintAnimationClockNative_calculateCurrentAnimationPhase(
    int64_t clock_state_millis, int64_t brush_family_loop_duration_millis,
    int64_t brush_paint_loop_duration_millis,
    float stroke_base_paint_animation_phase);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_PAINT_ANIMATION_CLOCK_NATIVE_H_
