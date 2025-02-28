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

#include <cstdint>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ink::BrushBehavior;
using ink::brush_internal::ValidateBrushBehaviorNode;
using ink::brush_internal::ValidateBrushBehaviorTopLevel;
using ink::jni::CastToBrushBehaviorNode;
using ink::jni::CastToEasingFunction;
using ink::jni::ThrowExceptionFromStatus;

jlong ValidateAndHoistNodeOrThrow(BrushBehavior::Node node, JNIEnv* env) {
  if (absl::Status status = ValidateBrushBehaviorNode(node); !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushBehavior::Node(std::move(node)));
}

}  // namespace

extern "C" {

JNI_METHOD(brush, BrushBehaviorNative, jlong, createFromOrderedNodes)
(JNIEnv* env, jobject thiz, jlongArray node_native_pointer_array) {
  std::vector<BrushBehavior::Node> nodes;
  const jsize num_nodes = env->GetArrayLength(node_native_pointer_array);
  nodes.reserve(num_nodes);
  jlong* node_pointers =
      env->GetLongArrayElements(node_native_pointer_array, nullptr);
  ABSL_CHECK(node_pointers != nullptr);
  for (jsize i = 0; i < num_nodes; ++i) {
    nodes.push_back(CastToBrushBehaviorNode(node_pointers[i]));
  }
  env->ReleaseLongArrayElements(
      node_native_pointer_array, node_pointers,
      // No need to copy back the array, which is not modified.
      JNI_ABORT);
  BrushBehavior behavior{.nodes = std::move(nodes)};
  if (absl::Status status = ValidateBrushBehaviorTopLevel(behavior);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushBehavior(std::move(behavior)));
}

JNI_METHOD(brush, BrushBehaviorNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<BrushBehavior*>(native_pointer);
}

// Functions for dealing with BrushBehavior::Nodes. Note that on the C++ side,
// BrushBehavior::Node is a variant, so all of the native pointers are pointers
// to the same type (the variant BrushBehavior::Node).

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createSource)
(JNIEnv* env, jobject thiz, jint source, jfloat source_value_start,
 jfloat source_value_end, jint source_out_of_range_behavior) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::SourceNode{
          .source = static_cast<BrushBehavior::Source>(source),
          .source_out_of_range_behavior =
              static_cast<BrushBehavior::OutOfRange>(
                  source_out_of_range_behavior),
          .source_value_range = {source_value_start, source_value_end},
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createConstant)
(JNIEnv* env, jobject thiz, jfloat value) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::ConstantNode{.value = value}, env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createNoise)
(JNIEnv* env, jobject thiz, jint seed, jint vary_over, jfloat base_period) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::NoiseNode{
          .seed = static_cast<uint32_t>(seed),
          .vary_over = static_cast<BrushBehavior::DampingSource>(vary_over),
          .base_period = base_period,
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createFallbackFilter)
(JNIEnv* env, jobject thiz, jint is_fallback_for) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::FallbackFilterNode{
          .is_fallback_for = static_cast<BrushBehavior::OptionalInputProperty>(
              is_fallback_for),
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createToolTypeFilter)
(JNIEnv* env, jobject thiz, jboolean mouse_enabled, jboolean touch_enabled,
 jboolean stylus_enabled, jboolean unknown_enabled) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::ToolTypeFilterNode{
          .enabled_tool_types = {.unknown = unknown_enabled ? true : false,
                                 .mouse = mouse_enabled ? true : false,
                                 .touch = touch_enabled ? true : false,
                                 .stylus = stylus_enabled ? true : false},
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createDamping)
(JNIEnv* env, jobject thiz, jint damping_source, jfloat damping_gap) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::DampingNode{
          .damping_source =
              static_cast<BrushBehavior::DampingSource>(damping_source),
          .damping_gap = damping_gap,
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createResponse)
(JNIEnv* env, jobject thiz, jlong easing_function_native_pointer) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::ResponseNode{
          .response_curve =
              CastToEasingFunction(easing_function_native_pointer),
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createBinaryOp)
(JNIEnv* env, jobject thiz, jint operation) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::BinaryOpNode{
          .operation = static_cast<BrushBehavior::BinaryOp>(operation),
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createInterpolation)
(JNIEnv* env, jobject thiz, jint interpolation) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::InterpolationNode{
          .interpolation =
              static_cast<BrushBehavior::Interpolation>(interpolation),
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createTarget)
(JNIEnv* env, jobject thiz, jint target, jfloat target_modifier_start,
 jfloat target_modifier_end) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::TargetNode{
          .target = static_cast<BrushBehavior::Target>(target),
          .target_modifier_range = {target_modifier_start, target_modifier_end},
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, jlong, createPolarTarget)
(JNIEnv* env, jobject thiz, jint polar_target, jfloat angle_range_start,
 jfloat angle_range_end, jfloat magnitude_range_start,
 jfloat magnitude_range_end) {
  return ValidateAndHoistNodeOrThrow(
      BrushBehavior::PolarTargetNode{
          .target = static_cast<BrushBehavior::PolarTarget>(polar_target),
          .angle_range = {angle_range_start, angle_range_end},
          .magnitude_range = {magnitude_range_start, magnitude_range_end},
      },
      env);
}

JNI_METHOD(brush, BrushBehaviorNodeNative, void, free)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  delete reinterpret_cast<BrushBehavior::Node*>(node_native_pointer);
}

}  // extern "C"
