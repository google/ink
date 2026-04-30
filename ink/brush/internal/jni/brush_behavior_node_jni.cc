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

#include "ink/brush/internal/jni/brush_behavior_node_native.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"

using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

// Functions for dealing with BrushBehavior::Nodes. Note that on the C++ side,
// BrushBehavior::Node is a variant, so all of the native pointers are pointers
// to the same type (the variant BrushBehavior::Node).

JNI_METHOD(brush_behavior, SourceNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint source, jfloat source_value_start,
 jfloat source_value_end, jint source_out_of_range_behavior) {
  return SourceNodeNative_create(env, source, source_value_start,
                                 source_value_end, source_out_of_range_behavior,
                                 &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, ConstantNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jfloat value) {
  return ConstantNodeNative_create(env, value,
                                   &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, NoiseNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint seed, jint vary_over, jfloat base_period) {
  return NoiseNodeNative_create(env, seed, vary_over, base_period,
                                &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, ToolTypeFilterNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jboolean mouse_enabled, jboolean touch_enabled,
 jboolean stylus_enabled, jboolean unknown_enabled) {
  return ToolTypeFilterNodeNative_create(env, mouse_enabled, touch_enabled,
                                         stylus_enabled, unknown_enabled,
                                         &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, DampingNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint damping_source, jfloat damping_gap) {
  return DampingNodeNative_create(env, damping_source, damping_gap,
                                  &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, ResponseNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jlong easing_function_native_pointer) {
  return ResponseNodeNative_create(env, easing_function_native_pointer,
                                   &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, IntegralNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint integrate_over, jfloat integral_value_start,
 jfloat integral_value_end, jint integral_out_of_range_behavior) {
  return IntegralNodeNative_create(
      env, integrate_over, integral_value_start, integral_value_end,
      integral_out_of_range_behavior, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, BinaryOpNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint operation) {
  return BinaryOpNodeNative_create(env, operation,
                                   &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, InterpolationNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint interpolation) {
  return InterpolationNodeNative_create(env, interpolation,
                                        &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, TargetNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint target, jfloat target_modifier_start,
 jfloat target_modifier_end) {
  return TargetNodeNative_create(env, target, target_modifier_start,
                                 target_modifier_end,
                                 &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jlong, create)
(JNIEnv* env, jobject thiz, jint polar_target, jfloat angle_range_start,
 jfloat angle_range_end, jfloat magnitude_range_start,
 jfloat magnitude_range_end) {
  return PolarTargetNodeNative_create(env, polar_target, angle_range_start,
                                      angle_range_end, magnitude_range_start,
                                      magnitude_range_end,
                                      &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush_behavior, NodeNative, void, free)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  NodeNative_free(node_native_pointer);
}

// SourceNode accessors:

JNI_METHOD(brush_behavior, SourceNodeNative, jint, getSourceInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return SourceNodeNative_getSourceInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, SourceNodeNative, jfloat, getValueRangeStart)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return SourceNodeNative_getValueRangeStart(node_native_pointer);
}

JNI_METHOD(brush_behavior, SourceNodeNative, jfloat, getValueRangeEnd)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return SourceNodeNative_getValueRangeEnd(node_native_pointer);
}

JNI_METHOD(brush_behavior, SourceNodeNative, jint, getOutOfRangeBehaviorInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return SourceNodeNative_getOutOfRangeBehaviorInt(node_native_pointer);
}

// ConstantNode accessors:

JNI_METHOD(brush_behavior, ConstantNodeNative, jfloat, getValue)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ConstantNodeNative_getValue(node_native_pointer);
}

// NoiseNode accessors:

JNI_METHOD(brush_behavior, NoiseNodeNative, jint, getSeed)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return NoiseNodeNative_getSeed(node_native_pointer);
}

JNI_METHOD(brush_behavior, NoiseNodeNative, jint, getVaryOverInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return NoiseNodeNative_getVaryOverInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, NoiseNodeNative, jfloat, getBasePeriod)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return NoiseNodeNative_getBasePeriod(node_native_pointer);
}

// ToolTypeFilterNode accessors:

JNI_METHOD(brush_behavior, ToolTypeFilterNodeNative, jboolean, getMouseEnabled)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ToolTypeFilterNodeNative_getMouseEnabled(node_native_pointer);
}

JNI_METHOD(brush_behavior, ToolTypeFilterNodeNative, jboolean, getTouchEnabled)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ToolTypeFilterNodeNative_getTouchEnabled(node_native_pointer);
}

JNI_METHOD(brush_behavior, ToolTypeFilterNodeNative, jboolean, getStylusEnabled)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ToolTypeFilterNodeNative_getStylusEnabled(node_native_pointer);
}

JNI_METHOD(brush_behavior, ToolTypeFilterNodeNative, jboolean,
           getUnknownEnabled)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ToolTypeFilterNodeNative_getUnknownEnabled(node_native_pointer);
}

// DampingNode accessors:

JNI_METHOD(brush_behavior, DampingNodeNative, jint, getDampingSourceInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return DampingNodeNative_getDampingSourceInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, DampingNodeNative, jfloat, getDampingGap)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return DampingNodeNative_getDampingGap(node_native_pointer);
}

// ResponseNode accessors:

JNI_METHOD(brush_behavior, ResponseNodeNative, jlong, getResponseCurvePointer)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return ResponseNodeNative_getResponseCurvePointer(node_native_pointer);
}

// IntegralNode accessors:

JNI_METHOD(brush_behavior, IntegralNodeNative, jint, getIntegrateOverInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return IntegralNodeNative_getIntegrateOverInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, IntegralNodeNative, jfloat, getValueRangeStart)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return IntegralNodeNative_getValueRangeStart(node_native_pointer);
}

JNI_METHOD(brush_behavior, IntegralNodeNative, jfloat, getValueRangeEnd)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return IntegralNodeNative_getValueRangeEnd(node_native_pointer);
}

JNI_METHOD(brush_behavior, IntegralNodeNative, jint, getOutOfRangeBehaviorInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return IntegralNodeNative_getOutOfRangeBehaviorInt(node_native_pointer);
}

// BinaryOpNode accessors:

JNI_METHOD(brush_behavior, BinaryOpNodeNative, jint, getOperationInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return BinaryOpNodeNative_getOperationInt(node_native_pointer);
}

// InterpolationNode accessors:

JNI_METHOD(brush_behavior, InterpolationNodeNative, jint, getInterpolationInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return InterpolationNodeNative_getInterpolationInt(node_native_pointer);
}

// TargetNode accessors:

JNI_METHOD(brush_behavior, TargetNodeNative, jint, getTargetInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return TargetNodeNative_getTargetInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, TargetNodeNative, jfloat, getModifierRangeStart)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return TargetNodeNative_getModifierRangeStart(node_native_pointer);
}

JNI_METHOD(brush_behavior, TargetNodeNative, jfloat, getModifierRangeEnd)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return TargetNodeNative_getModifierRangeEnd(node_native_pointer);
}

// PolarTargetNode accessors:

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jint, getTargetInt)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return PolarTargetNodeNative_getTargetInt(node_native_pointer);
}

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jfloat, getAngleRangeStart)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return PolarTargetNodeNative_getAngleRangeStart(node_native_pointer);
}

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jfloat, getAngleRangeEnd)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return PolarTargetNodeNative_getAngleRangeEnd(node_native_pointer);
}

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jfloat,
           getMagnitudeRangeStart)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return PolarTargetNodeNative_getMagnitudeRangeStart(node_native_pointer);
}

JNI_METHOD(brush_behavior, PolarTargetNodeNative, jfloat, getMagnitudeRangeEnd)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  return PolarTargetNodeNative_getMagnitudeRangeEnd(node_native_pointer);
}

}  // extern "C"
