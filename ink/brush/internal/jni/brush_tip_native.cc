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

#include "ink/brush/internal/jni/brush_tip_native.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/vec.h"
#include "ink/types/duration.h"

namespace {
using ::ink::Angle;
using ::ink::BrushBehavior;
using ::ink::BrushTip;
using ::ink::Duration32;
using ::ink::Vec;
using ::ink::brush_internal::ValidateBrushTip;
using ::ink::native::CastToBrushBehavior;
using ::ink::native::CastToBrushTip;
using ::ink::native::DeleteNativeBrushTip;
using ::ink::native::NewNativeBrushBehavior;
using ::ink::native::NewNativeBrushTip;
}  // namespace

extern "C" {

int64_t BrushTipNative_create(
    void* jni_env_pass_through, float scale_x, float scale_y,
    float corner_rounding, float slant_degrees, float pinch,
    float rotation_degrees, float particle_gap_distance_scale,
    int64_t particle_gap_duration_millis,
    const int64_t* behavior_native_pointers, int num_behaviors,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  std::vector<BrushBehavior> behaviors;
  behaviors.reserve(num_behaviors);
  for (int i = 0; i < num_behaviors; ++i) {
    behaviors.push_back(CastToBrushBehavior(behavior_native_pointers[i]));
  }
  BrushTip tip{
      .scale = Vec{scale_x, scale_y},
      .corner_rounding = corner_rounding,
      .slant = Angle::Degrees(slant_degrees),
      .pinch = pinch,
      .rotation = Angle::Degrees(rotation_degrees),
      .particle_gap_distance_scale = particle_gap_distance_scale,
      .particle_gap_duration = Duration32::Millis(particle_gap_duration_millis),
      .behaviors = behaviors};
  if (absl::Status status = ValidateBrushTip(tip); !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeBrushTip(std::move(tip));
}

void BrushTipNative_free(int64_t native_ptr) {
  DeleteNativeBrushTip(native_ptr);
}

float BrushTipNative_getScaleX(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).scale.x;
}

float BrushTipNative_getScaleY(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).scale.y;
}

float BrushTipNative_getCornerRounding(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).corner_rounding;
}

float BrushTipNative_getSlantDegrees(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).slant.ValueInDegrees();
}

float BrushTipNative_getPinch(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).pinch;
}

float BrushTipNative_getRotationDegrees(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).rotation.ValueInDegrees();
}

float BrushTipNative_getParticleGapDistanceScale(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).particle_gap_distance_scale;
}

int64_t BrushTipNative_getParticleGapDurationMillis(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).particle_gap_duration.ToMillis();
}

int BrushTipNative_getBehaviorCount(int64_t native_ptr) {
  return CastToBrushTip(native_ptr).behaviors.size();
}

int64_t BrushTipNative_newCopyOfBrushBehavior(int64_t native_ptr, int index) {
  return NewNativeBrushBehavior(CastToBrushTip(native_ptr).behaviors[index]);
}

}  // extern "C"
