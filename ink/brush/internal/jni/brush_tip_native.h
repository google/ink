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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_TIP_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_TIP_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushTipNative_create(
    void* jni_env_pass_through, float scale_x, float scale_y,
    float corner_rounding, float slant_degrees, float pinch,
    float rotation_degrees, float particle_gap_distance_scale,
    int64_t particle_gap_duration_millis,
    const int64_t* behavior_native_pointers, int num_behaviors,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void BrushTipNative_free(int64_t native_ptr);

float BrushTipNative_getScaleX(int64_t native_ptr);

float BrushTipNative_getScaleY(int64_t native_ptr);

float BrushTipNative_getCornerRounding(int64_t native_ptr);

float BrushTipNative_getSlantDegrees(int64_t native_ptr);

float BrushTipNative_getPinch(int64_t native_ptr);

float BrushTipNative_getRotationDegrees(int64_t native_ptr);

float BrushTipNative_getParticleGapDistanceScale(int64_t native_ptr);

int64_t BrushTipNative_getParticleGapDurationMillis(int64_t native_ptr);

int BrushTipNative_getBehaviorCount(int64_t native_ptr);

int64_t BrushTipNative_newCopyOfBrushBehavior(int64_t native_ptr, int index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_TIP_NATIVE_H_
