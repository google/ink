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

#include <jni.h>

#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/internal/jni/stroke_paint_animation_clock_native.h"

extern "C" {

JNI_METHOD(strokes, StrokePaintAnimationClockNative, jfloat,
           calculateBasePaintAnimationPhase)
(JNIEnv* env, jobject object, jlong clock_state_millis,
 jlong brush_family_loop_duration_millis) {
  return StrokePaintAnimationClockNative_calculateBasePaintAnimationPhase(
      clock_state_millis, brush_family_loop_duration_millis);
}

JNI_METHOD(strokes, StrokePaintAnimationClockNative, jfloat,
           calculateCurrentAnimationPhase)
(JNIEnv* env, jobject object, jlong clock_state_millis,
 jlong brush_family_loop_duration_millis,
 jlong brush_paint_loop_duration_millis,
 jfloat stroke_base_paint_animation_phase) {
  return StrokePaintAnimationClockNative_calculateCurrentAnimationPhase(
      clock_state_millis, brush_family_loop_duration_millis,
      brush_paint_loop_duration_millis, stroke_base_paint_animation_phase);
}

}  // extern "C"
