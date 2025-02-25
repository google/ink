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
#include "ink/brush/easing_function.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/geometry/point.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ink::BrushBehavior;
using ink::EasingFunction;
using ink::Point;
using ink::brush_internal::ValidateBrushBehavior;
using ink::jni::CastToBrushBehaviorNode;
using ink::jni::ThrowExceptionFromStatus;

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
  if (absl::Status status = ValidateBrushBehavior(behavior); !status.ok()) {
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

JNI_METHOD(brush, BrushBehaviorNative, jlong, createSourceNode)
(JNIEnv* env, jobject thiz, jint source, jfloat source_value_start,
 jfloat source_value_end, jint source_out_of_range_behavior) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::SourceNode{
          .source = static_cast<BrushBehavior::Source>(source),
          .source_out_of_range_behavior =
              static_cast<BrushBehavior::OutOfRange>(
                  source_out_of_range_behavior),
          .source_value_range = {source_value_start, source_value_end},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createConstantNode)
(JNIEnv* env, jobject thiz, jfloat value) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ConstantNode{
          .value = value,
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createNoiseNode)
(JNIEnv* env, jobject thiz, jint seed, jint vary_over, jfloat base_period) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::NoiseNode{
          .seed = static_cast<uint32_t>(seed),
          .vary_over = static_cast<BrushBehavior::DampingSource>(vary_over),
          .base_period = base_period,
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createFallbackFilterNode)
(JNIEnv* env, jobject thiz, jint is_fallback_for) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::FallbackFilterNode{
          .is_fallback_for = static_cast<BrushBehavior::OptionalInputProperty>(
              is_fallback_for),
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createToolTypeFilterNode)
(JNIEnv* env, jobject thiz, jboolean mouse_enabled, jboolean touch_enabled,
 jboolean stylus_enabled, jboolean unknown_enabled) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ToolTypeFilterNode{
          .enabled_tool_types = {.unknown = unknown_enabled ? true : false,
                                 .mouse = mouse_enabled ? true : false,
                                 .touch = touch_enabled ? true : false,
                                 .stylus = stylus_enabled ? true : false},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createDampingNode)
(JNIEnv* env, jobject thiz, jint damping_source, jfloat damping_gap) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::DampingNode{
          .damping_source =
              static_cast<BrushBehavior::DampingSource>(damping_source),
          .damping_gap = damping_gap,
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createResponseNodePredefined)
(JNIEnv* env, jobject thiz, jint predefined_response_curve) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ResponseNode{
          .response_curve = {static_cast<EasingFunction::Predefined>(
              predefined_response_curve)},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createResponseNodeCubicBezier)
(JNIEnv* env, jobject thiz, jfloat x1, jfloat y1, jfloat x2, jfloat y2) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ResponseNode{
          .response_curve = {EasingFunction::CubicBezier{
              .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2}},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createResponseNodeSteps)
(JNIEnv* env, jobject thiz, jint step_count, jint step_position) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ResponseNode{
          .response_curve = {EasingFunction::Steps{
              .step_count = step_count,
              .step_position =
                  static_cast<EasingFunction::StepPosition>(step_position),
          }},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createResponseNodeLinear)
(JNIEnv* env, jobject thiz, jfloatArray points_array) {
  jsize num_points = env->GetArrayLength(points_array) / 2;
  jfloat* points_elements = env->GetFloatArrayElements(points_array, nullptr);
  std::vector<Point> points_vector;
  points_vector.reserve(num_points);
  for (int i = 0; i < num_points; ++i) {
    float x = points_elements[2 * i];
    float y = points_elements[2 * i + 1];
    points_vector.push_back(Point{x, y});
  }
  env->ReleaseFloatArrayElements(points_array, points_elements, 0);
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::ResponseNode{
          .response_curve = {EasingFunction::Linear{
              .points = std::move(points_vector)}},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createBinaryOpNode)
(JNIEnv* env, jobject thiz, jint operation) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::BinaryOpNode{
          .operation = static_cast<BrushBehavior::BinaryOp>(operation),
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createInterpolationNode)
(JNIEnv* env, jobject thiz, jint interpolation) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::InterpolationNode{
          .interpolation =
              static_cast<BrushBehavior::Interpolation>(interpolation),
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createTargetNode)
(JNIEnv* env, jobject thiz, jint target, jfloat target_modifier_start,
 jfloat target_modifier_end) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::TargetNode{
          .target = static_cast<BrushBehavior::Target>(target),
          .target_modifier_range = {target_modifier_start, target_modifier_end},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, createPolarTargetNode)
(JNIEnv* env, jobject thiz, jint polar_target, jfloat angle_range_start,
 jfloat angle_range_end, jfloat magnitude_range_start,
 jfloat magnitude_range_end) {
  return reinterpret_cast<jlong>(
      new BrushBehavior::Node(BrushBehavior::PolarTargetNode{
          .target = static_cast<BrushBehavior::PolarTarget>(polar_target),
          .angle_range = {angle_range_start, angle_range_end},
          .magnitude_range = {magnitude_range_start, magnitude_range_end},
      }));
}

JNI_METHOD(brush, BrushBehaviorNative, void, freeNode)
(JNIEnv* env, jobject thiz, jlong node_native_pointer) {
  delete reinterpret_cast<BrushBehavior::Node*>(node_native_pointer);
}

}  // extern "C"
