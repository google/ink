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

#include <optional>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/easing_function.h"
#include "ink/geometry/point.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/types/duration.h"

extern "C" {

JNI_METHOD(brush, BrushBehavior, jlong, nativeCreateEmptyBrushBehavior)
(JNIEnv* env, jobject thiz) {
  return reinterpret_cast<jlong>(new ink::BrushBehavior{});
}

JNI_METHOD(brush, BrushBehavior, jlong, nativeValidateOrDeleteAndThrow)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  auto* brush_behavior = reinterpret_cast<ink::BrushBehavior*>(native_pointer);
  absl::Status status =
      ink::brush_internal::ValidateBrushBehavior(*brush_behavior);
  if (!status.ok()) {
    delete brush_behavior;
    ink::jni::ThrowExceptionFromStatus(env, status);
    return 0;  // unused return value
  }
  return native_pointer;
}

JNI_METHOD(brush, BrushBehavior, void, nativeFreeBrushBehavior)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::BrushBehavior*>(native_pointer);
}

JNI_METHOD_INNER(brush, BrushBehavior, SourceNode, void, nativeAppendSourceNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint source,
 jfloat source_value_lower_bound, jfloat source_value_upper_bound,
 jint source_out_of_range_behavior) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::SourceNode{
      .source = static_cast<ink::BrushBehavior::Source>(source),
      .source_out_of_range_behavior =
          static_cast<ink::BrushBehavior::OutOfRange>(
              source_out_of_range_behavior),
      .source_value_range = {source_value_lower_bound,
                             source_value_upper_bound},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ConstantNode, void,
                 nativeAppendConstantNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jfloat value) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ConstantNode{
      .value = value,
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, FallbackFilterNode, void,
                 nativeAppendFallbackFilterNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer,
 jint is_fallback_for) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::FallbackFilterNode{
      .is_fallback_for = static_cast<ink::BrushBehavior::OptionalInputProperty>(
          is_fallback_for),
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ToolTypeFilterNode, void,
                 nativeAppendToolTypeFilterNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer,
 jboolean mouse_enabled, jboolean touch_enabled, jboolean stylus_enabled,
 jboolean unknown_enabled) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ToolTypeFilterNode{
      .enabled_tool_types = {.unknown = unknown_enabled ? true : false,
                             .mouse = mouse_enabled ? true : false,
                             .touch = touch_enabled ? true : false,
                             .stylus = stylus_enabled ? true : false},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, DampingNode, void,
                 nativeAppendDampingNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint damping_source,
 jfloat damping_gap) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::DampingNode{
      .damping_source =
          static_cast<ink::BrushBehavior::DampingSource>(damping_source),
      .damping_gap = damping_gap,
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ResponseNode, void,
                 nativeAppendResponseNodePredefined)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer,
 jint predefined_response_curve) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ResponseNode{
      .response_curve = {static_cast<ink::EasingFunction::Predefined>(
          predefined_response_curve)},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ResponseNode, void,
                 nativeAppendResponseNodeCubicBezier)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jfloat x1, jfloat y1,
 jfloat x2, jfloat y2) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ResponseNode{
      .response_curve = {ink::EasingFunction::CubicBezier{
          .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2}},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ResponseNode, void,
                 nativeAppendResponseNodeSteps)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint step_count,
 jint step_position) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ResponseNode{
      .response_curve = {ink::EasingFunction::Steps{
          .step_count = step_count,
          .step_position =
              static_cast<ink::EasingFunction::StepPosition>(step_position),
      }},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, ResponseNode, void,
                 nativeAppendResponseNodeLinear)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer,
 jfloatArray points_array) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  jsize num_points = env->GetArrayLength(points_array) / 2;
  jfloat* points_elements = env->GetFloatArrayElements(points_array, nullptr);
  std::vector<ink::Point> points_vector;
  points_vector.reserve(num_points);
  for (int i = 0; i < num_points; ++i) {
    float x = points_elements[2 * i];
    float y = points_elements[2 * i + 1];
    points_vector.push_back(ink::Point{x, y});
  }
  env->ReleaseFloatArrayElements(points_array, points_elements, 0);
  brush_behavior->nodes.push_back(ink::BrushBehavior::ResponseNode{
      .response_curve = {ink::EasingFunction::Linear{
          .points = std::move(points_vector)}},
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, BinaryOpNode, void,
                 nativeAppendBinaryOpNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint operation) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::BinaryOpNode{
      .operation = static_cast<ink::BrushBehavior::BinaryOp>(operation),
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, InterpolationNode, void,
                 nativeAppendInterpolationNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint interpolation) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::InterpolationNode{
      .interpolation =
          static_cast<ink::BrushBehavior::Interpolation>(interpolation),
  });
}

JNI_METHOD_INNER(brush, BrushBehavior, TargetNode, void, nativeAppendTargetNode)
(JNIEnv* env, jobject thiz, jlong native_behavior_pointer, jint target,
 jfloat target_modifier_lower_bound, jfloat target_modifier_upper_bound) {
  auto* brush_behavior =
      reinterpret_cast<ink::BrushBehavior*>(native_behavior_pointer);
  brush_behavior->nodes.push_back(ink::BrushBehavior::TargetNode{
      .target = static_cast<ink::BrushBehavior::Target>(target),
      .target_modifier_range = {target_modifier_lower_bound,
                                target_modifier_upper_bound},
  });
}

}  // extern "C"
