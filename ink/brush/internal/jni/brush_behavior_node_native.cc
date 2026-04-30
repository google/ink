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

#include "ink/brush/internal/jni/brush_behavior_node_native.h"

#include <cstdint>
#include <utility>

#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/internal/jni/brush_native_helper.h"

namespace {

using ::ink::BrushBehavior;
using ::ink::brush_internal::ValidateBrushBehaviorNode;
using ::ink::native::CastToBrushBehaviorNode;
using ::ink::native::CastToEasingFunction;
using ::ink::native::DeleteNativeBrushBehaviorNode;
using ::ink::native::NewNativeBrushBehaviorNode;

int64_t ValidateAndHoistNode(void* jni_env_pass_through,
                             BrushBehavior::Node node,
                             void (*throw_from_status_callback)(void*, int,
                                                                const char*)) {
  if (absl::Status status = ValidateBrushBehaviorNode(node); !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.message().data());
    return 0;
  }
  return NewNativeBrushBehaviorNode(std::move(node));
}

}  // namespace

extern "C" {

int64_t SourceNodeNative_create(
    void* jni_env_pass_through, int source, float source_value_start,
    float source_value_end, int source_out_of_range_behavior,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::SourceNode{
          .source = static_cast<BrushBehavior::Source>(source),
          .source_out_of_range_behavior =
              static_cast<BrushBehavior::OutOfRange>(
                  source_out_of_range_behavior),
          .source_value_range = {source_value_start, source_value_end},
      },
      throw_from_status_callback);
}

int64_t ConstantNodeNative_create(
    void* jni_env_pass_through, float value,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(jni_env_pass_through,
                              BrushBehavior::ConstantNode{.value = value},
                              throw_from_status_callback);
}

int64_t NoiseNodeNative_create(
    void* jni_env_pass_through, int seed, int vary_over, float base_period,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::NoiseNode{
          .seed = static_cast<uint32_t>(seed),
          .vary_over = static_cast<BrushBehavior::ProgressDomain>(vary_over),
          .base_period = base_period,
      },
      throw_from_status_callback);
}

int64_t ToolTypeFilterNodeNative_create(
    void* jni_env_pass_through, bool mouse_enabled, bool touch_enabled,
    bool stylus_enabled, bool unknown_enabled,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::ToolTypeFilterNode{
          .enabled_tool_types = {.unknown = unknown_enabled,
                                 .mouse = mouse_enabled,
                                 .touch = touch_enabled,
                                 .stylus = stylus_enabled},
      },
      throw_from_status_callback);
}

int64_t DampingNodeNative_create(
    void* jni_env_pass_through, int damping_source, float damping_gap,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::DampingNode{
          .damping_source =
              static_cast<BrushBehavior::ProgressDomain>(damping_source),
          .damping_gap = damping_gap,
      },
      throw_from_status_callback);
}

int64_t ResponseNodeNative_create(
    void* jni_env_pass_through, int64_t easing_function_native_pointer,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(jni_env_pass_through,
                              BrushBehavior::ResponseNode{
                                  .response_curve = CastToEasingFunction(
                                      easing_function_native_pointer),
                              },
                              throw_from_status_callback);
}

int64_t IntegralNodeNative_create(
    void* jni_env_pass_through, int integrate_over, float integral_value_start,
    float integral_value_end, int integral_out_of_range_behavior,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::IntegralNode{
          .integrate_over =
              static_cast<BrushBehavior::ProgressDomain>(integrate_over),
          .integral_out_of_range_behavior =
              static_cast<BrushBehavior::OutOfRange>(
                  integral_out_of_range_behavior),
          .integral_value_range = {integral_value_start, integral_value_end},
      },
      throw_from_status_callback);
}

int64_t BinaryOpNodeNative_create(
    void* jni_env_pass_through, int operation,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::BinaryOpNode{
          .operation = static_cast<BrushBehavior::BinaryOp>(operation),
      },
      throw_from_status_callback);
}

int64_t InterpolationNodeNative_create(
    void* jni_env_pass_through, int interpolation,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::InterpolationNode{
          .interpolation =
              static_cast<BrushBehavior::Interpolation>(interpolation),
      },
      throw_from_status_callback);
}

int64_t TargetNodeNative_create(
    void* jni_env_pass_through, int target, float target_modifier_start,
    float target_modifier_end,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::TargetNode{
          .target = static_cast<BrushBehavior::Target>(target),
          .target_modifier_range = {target_modifier_start, target_modifier_end},
      },
      throw_from_status_callback);
}

int64_t PolarTargetNodeNative_create(
    void* jni_env_pass_through, int polar_target, float angle_range_start,
    float angle_range_end, float magnitude_range_start,
    float magnitude_range_end,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistNode(
      jni_env_pass_through,
      BrushBehavior::PolarTargetNode{
          .target = static_cast<BrushBehavior::PolarTarget>(polar_target),
          .angle_range = {angle_range_start, angle_range_end},
          .magnitude_range = {magnitude_range_start, magnitude_range_end},
      },
      throw_from_status_callback);
}

void NodeNative_free(int64_t native_ptr) {
  DeleteNativeBrushBehaviorNode(native_ptr);
}

// SourceNode accessors:

int SourceNodeNative_getSourceInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::SourceNode>(CastToBrushBehaviorNode(native_ptr))
          .source);
}

float SourceNodeNative_getValueRangeStart(int64_t native_ptr) {
  return std::get<BrushBehavior::SourceNode>(
             CastToBrushBehaviorNode(native_ptr))
      .source_value_range[0];
}

float SourceNodeNative_getValueRangeEnd(int64_t native_ptr) {
  return std::get<BrushBehavior::SourceNode>(
             CastToBrushBehaviorNode(native_ptr))
      .source_value_range[1];
}

int SourceNodeNative_getOutOfRangeBehaviorInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::SourceNode>(CastToBrushBehaviorNode(native_ptr))
          .source_out_of_range_behavior);
}

// ConstantNode accessors:

float ConstantNodeNative_getValue(int64_t native_ptr) {
  return std::get<BrushBehavior::ConstantNode>(
             CastToBrushBehaviorNode(native_ptr))
      .value;
}

// NoiseNode accessors:

int NoiseNodeNative_getSeed(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::NoiseNode>(CastToBrushBehaviorNode(native_ptr))
          .seed);
}

int NoiseNodeNative_getVaryOverInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::NoiseNode>(CastToBrushBehaviorNode(native_ptr))
          .vary_over);
}

float NoiseNodeNative_getBasePeriod(int64_t native_ptr) {
  return std::get<BrushBehavior::NoiseNode>(CastToBrushBehaviorNode(native_ptr))
      .base_period;
}

// ToolTypeFilterNode accessors:

bool ToolTypeFilterNodeNative_getMouseEnabled(int64_t native_ptr) {
  return std::get<BrushBehavior::ToolTypeFilterNode>(
             CastToBrushBehaviorNode(native_ptr))
      .enabled_tool_types.mouse;
}

bool ToolTypeFilterNodeNative_getTouchEnabled(int64_t native_ptr) {
  return std::get<BrushBehavior::ToolTypeFilterNode>(
             CastToBrushBehaviorNode(native_ptr))
      .enabled_tool_types.touch;
}

bool ToolTypeFilterNodeNative_getStylusEnabled(int64_t native_ptr) {
  return std::get<BrushBehavior::ToolTypeFilterNode>(
             CastToBrushBehaviorNode(native_ptr))
      .enabled_tool_types.stylus;
}

bool ToolTypeFilterNodeNative_getUnknownEnabled(int64_t native_ptr) {
  return std::get<BrushBehavior::ToolTypeFilterNode>(
             CastToBrushBehaviorNode(native_ptr))
      .enabled_tool_types.unknown;
}

// DampingNode accessors:

int DampingNodeNative_getDampingSourceInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::DampingNode>(CastToBrushBehaviorNode(native_ptr))
          .damping_source);
}

float DampingNodeNative_getDampingGap(int64_t native_ptr) {
  return std::get<BrushBehavior::DampingNode>(
             CastToBrushBehaviorNode(native_ptr))
      .damping_gap;
}

// ResponseNode accessors:

int64_t ResponseNodeNative_getResponseCurvePointer(int64_t native_ptr) {
  return reinterpret_cast<int64_t>(&std::get<BrushBehavior::ResponseNode>(
                                        CastToBrushBehaviorNode(native_ptr))
                                        .response_curve);
}

// IntegralNode accessors:

int IntegralNodeNative_getIntegrateOverInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::IntegralNode>(CastToBrushBehaviorNode(native_ptr))
          .integrate_over);
}

float IntegralNodeNative_getValueRangeStart(int64_t native_ptr) {
  return std::get<BrushBehavior::IntegralNode>(
             CastToBrushBehaviorNode(native_ptr))
      .integral_value_range[0];
}

float IntegralNodeNative_getValueRangeEnd(int64_t native_ptr) {
  return std::get<BrushBehavior::IntegralNode>(
             CastToBrushBehaviorNode(native_ptr))
      .integral_value_range[1];
}

int IntegralNodeNative_getOutOfRangeBehaviorInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::IntegralNode>(CastToBrushBehaviorNode(native_ptr))
          .integral_out_of_range_behavior);
}

// BinaryOpNode accessors:

int BinaryOpNodeNative_getOperationInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::BinaryOpNode>(CastToBrushBehaviorNode(native_ptr))
          .operation);
}

// InterpolationNode accessors:

int InterpolationNodeNative_getInterpolationInt(int64_t native_ptr) {
  return static_cast<int>(std::get<BrushBehavior::InterpolationNode>(
                              CastToBrushBehaviorNode(native_ptr))
                              .interpolation);
}

// TargetNode accessors:

int TargetNodeNative_getTargetInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushBehavior::TargetNode>(CastToBrushBehaviorNode(native_ptr))
          .target);
}

float TargetNodeNative_getModifierRangeStart(int64_t native_ptr) {
  return std::get<BrushBehavior::TargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .target_modifier_range[0];
}

float TargetNodeNative_getModifierRangeEnd(int64_t native_ptr) {
  return std::get<BrushBehavior::TargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .target_modifier_range[1];
}

// PolarTargetNode accessors:

int PolarTargetNodeNative_getTargetInt(int64_t native_ptr) {
  return static_cast<int>(std::get<BrushBehavior::PolarTargetNode>(
                              CastToBrushBehaviorNode(native_ptr))
                              .target);
}

float PolarTargetNodeNative_getAngleRangeStart(int64_t native_ptr) {
  return std::get<BrushBehavior::PolarTargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .angle_range[0];
}

float PolarTargetNodeNative_getAngleRangeEnd(int64_t native_ptr) {
  return std::get<BrushBehavior::PolarTargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .angle_range[1];
}

float PolarTargetNodeNative_getMagnitudeRangeStart(int64_t native_ptr) {
  return std::get<BrushBehavior::PolarTargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .magnitude_range[0];
}

float PolarTargetNodeNative_getMagnitudeRangeEnd(int64_t native_ptr) {
  return std::get<BrushBehavior::PolarTargetNode>(
             CastToBrushBehaviorNode(native_ptr))
      .magnitude_range[1];
}

}  // extern "C"
