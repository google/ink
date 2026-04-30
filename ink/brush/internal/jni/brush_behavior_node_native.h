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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NODE_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NODE_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The native_ptr parameter of these methods contains the raw pointer to a
// C++ BrushBehavior::Node object stored in Kotlin.

// If validation fails for any create function, it calls
// throw_from_status_callback with the value passed as jni_env_pass_through,
// the error status code as an int, and the error status message.
int64_t SourceNodeNative_create(
    void* jni_env_pass_through, int source, float source_value_start,
    float source_value_end, int source_out_of_range_behavior,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t ConstantNodeNative_create(
    void* jni_env_pass_through, float value,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t NoiseNodeNative_create(void* jni_env_pass_through, int seed,
                               int vary_over, float base_period,
                               void (*throw_from_status_callback)(void*, int,
                                                                  const char*));

int64_t ToolTypeFilterNodeNative_create(
    void* jni_env_pass_through, bool mouse_enabled, bool touch_enabled,
    bool stylus_enabled, bool unknown_enabled,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t DampingNodeNative_create(
    void* jni_env_pass_through, int damping_source, float damping_gap,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t ResponseNodeNative_create(
    void* jni_env_pass_through, int64_t easing_function_native_pointer,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t IntegralNodeNative_create(
    void* jni_env_pass_through, int integrate_over, float integral_value_start,
    float integral_value_end, int integral_out_of_range_behavior,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t BinaryOpNodeNative_create(
    void* jni_env_pass_through, int operation,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t InterpolationNodeNative_create(
    void* jni_env_pass_through, int interpolation,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t TargetNodeNative_create(
    void* jni_env_pass_through, int target, float target_modifier_start,
    float target_modifier_end,
    void (*throw_from_status_callback)(void*, int, const char*));

int64_t PolarTargetNodeNative_create(
    void* jni_env_pass_through, int polar_target, float angle_range_start,
    float angle_range_end, float magnitude_range_start,
    float magnitude_range_end,
    void (*throw_from_status_callback)(void*, int, const char*));

void NodeNative_free(int64_t native_ptr);

// SourceNode accessors:
int SourceNodeNative_getSourceInt(int64_t native_ptr);
float SourceNodeNative_getValueRangeStart(int64_t native_ptr);
float SourceNodeNative_getValueRangeEnd(int64_t native_ptr);
int SourceNodeNative_getOutOfRangeBehaviorInt(int64_t native_ptr);

// ConstantNode accessors:
float ConstantNodeNative_getValue(int64_t native_ptr);

// NoiseNode accessors:
int NoiseNodeNative_getSeed(int64_t native_ptr);
int NoiseNodeNative_getVaryOverInt(int64_t native_ptr);
float NoiseNodeNative_getBasePeriod(int64_t native_ptr);

// ToolTypeFilterNode accessors:
bool ToolTypeFilterNodeNative_getMouseEnabled(int64_t native_ptr);
bool ToolTypeFilterNodeNative_getTouchEnabled(int64_t native_ptr);
bool ToolTypeFilterNodeNative_getStylusEnabled(int64_t native_ptr);
bool ToolTypeFilterNodeNative_getUnknownEnabled(int64_t native_ptr);

// DampingNode accessors:
int DampingNodeNative_getDampingSourceInt(int64_t native_ptr);
float DampingNodeNative_getDampingGap(int64_t native_ptr);

// ResponseNode accessors:
int64_t ResponseNodeNative_getResponseCurvePointer(int64_t native_ptr);

// IntegralNode accessors:
int IntegralNodeNative_getIntegrateOverInt(int64_t native_ptr);
float IntegralNodeNative_getValueRangeStart(int64_t native_ptr);
float IntegralNodeNative_getValueRangeEnd(int64_t native_ptr);
int IntegralNodeNative_getOutOfRangeBehaviorInt(int64_t native_ptr);

// BinaryOpNode accessors:
int BinaryOpNodeNative_getOperationInt(int64_t native_ptr);

// InterpolationNode accessors:
int InterpolationNodeNative_getInterpolationInt(int64_t native_ptr);

// TargetNode accessors:
int TargetNodeNative_getTargetInt(int64_t native_ptr);
float TargetNodeNative_getModifierRangeStart(int64_t native_ptr);
float TargetNodeNative_getModifierRangeEnd(int64_t native_ptr);

// PolarTargetNode accessors:
int PolarTargetNodeNative_getTargetInt(int64_t native_ptr);
float PolarTargetNodeNative_getAngleRangeStart(int64_t native_ptr);
float PolarTargetNodeNative_getAngleRangeEnd(int64_t native_ptr);
float PolarTargetNodeNative_getMagnitudeRangeStart(int64_t native_ptr);
float PolarTargetNodeNative_getMagnitudeRangeEnd(int64_t native_ptr);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NODE_NATIVE_H_
