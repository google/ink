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

#include <jni.h>

#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/vec.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/types/duration.h"

extern "C" {

// Constructs a native BrushTip and returns a pointer to it as a long. Throws an
// exception if the BrushTip validation fails.
JNI_METHOD(brush, BrushTip, jlong, nativeCreateBrushTip)
(JNIEnv* env, jobject thiz, jfloat scale_x, jfloat scale_y,
 jfloat corner_rounding, jfloat slant_radians, jfloat pinch,
 jfloat rotation_radians, jfloat opacity_multiplier,
 jfloat particle_gap_distance_scale, jlong particle_gap_duration_millis,
 jint behaviors_count) {
  std::vector<ink::BrushBehavior> behaviors;
  ink::BrushTip tip =
      ink::BrushTip{.scale = ink::Vec{scale_x, scale_y},
                    .corner_rounding = corner_rounding,
                    .slant = ink::Angle::Radians(slant_radians),
                    .pinch = pinch,
                    .rotation = ink::Angle::Radians(rotation_radians),
                    .opacity_multiplier = opacity_multiplier,
                    .particle_gap_distance_scale = particle_gap_distance_scale,
                    .particle_gap_duration =
                        ink::Duration32::Millis(particle_gap_duration_millis),
                    .behaviors = behaviors};
  absl::Status status = ink::brush_internal::ValidateBrushTip(tip);

  if (!status.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, status);
    return -1;  // Unused return value.
  }

  auto* validated_tip = new ink::BrushTip(std::move(tip));
  validated_tip->behaviors.reserve(behaviors_count);
  return reinterpret_cast<jlong>(validated_tip);
}

// Appends a brush behavior to a *mutable* C++ BrushTip object as referenced by
// `tip_native_pointer`. Only call this method within kotlin init{} scope so to
// keep this BrushTip object immutable after construction and equivalent across
// Kotlin and C++.
JNI_METHOD(brush, BrushTip, void, nativeAppendBehavior)
(JNIEnv* env, jobject thiz, jlong tip_native_pointer,
 jlong behavior_native_pointer) {
  // Intentionally non-const casting to BrushTip in this rare native method
  // called during kotlin init scope.
  auto* brush_tip = reinterpret_cast<ink::BrushTip*>(tip_native_pointer);
  brush_tip->behaviors.push_back(
      *reinterpret_cast<ink::BrushBehavior*>(behavior_native_pointer));
}

JNI_METHOD(brush, BrushTip, void, nativeFreeBrushTip)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::BrushTip*>(native_pointer);
}
}
