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

#include "ink/brush/internal/jni/easing_function_native.h"

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "absl/functional/overload.h"
#include "absl/status/status.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/geometry/point.h"

namespace {

using ink::EasingFunction;
using ink::Point;
using ink::native::CastToEasingFunction;
using ink::native::DeleteNativeEasingFunction;
using ink::native::NewNativeEasingFunction;

int64_t ValidateAndHoistEasingFunction(
    void* jni_env_pass_through, EasingFunction::Parameters parameters,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  EasingFunction easing_function{.parameters = std::move(parameters)};
  if (absl::Status status =
          ink::brush_internal::ValidateEasingFunction(easing_function);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.message().data());
    return 0;
  }
  return NewNativeEasingFunction(std::move(easing_function));
}

constexpr int kPredefined = 0;
constexpr int kCubicBezier = 1;
constexpr int kLinear = 2;
constexpr int kSteps = 3;

}  // namespace

extern "C" {

int64_t EasingFunctionNative_createCopyOf(int64_t other_native_ptr) {
  return NewNativeEasingFunction(CastToEasingFunction(other_native_ptr));
}

int64_t EasingFunctionNative_createPredefined(
    void* jni_env_pass_through, int value,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistEasingFunction(
      jni_env_pass_through, static_cast<EasingFunction::Predefined>(value),
      throw_from_status_callback);
}

int64_t EasingFunctionNative_createCubicBezier(
    void* jni_env_pass_through, float x1, float y1, float x2, float y2,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistEasingFunction(
      jni_env_pass_through,
      EasingFunction::CubicBezier{.x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2},
      throw_from_status_callback);
}

int64_t EasingFunctionNative_createSteps(
    void* jni_env_pass_through, int step_count, int step_position,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  return ValidateAndHoistEasingFunction(
      jni_env_pass_through,
      EasingFunction::Steps{
          .step_count = step_count,
          .step_position =
              static_cast<EasingFunction::StepPosition>(step_position),
      },
      throw_from_status_callback);
}

int64_t EasingFunctionNative_createLinear(
    void* jni_env_pass_through, const float* points, int num_points,
    void (*throw_from_status_callback)(void*, int, const char*)) {
  std::vector<Point> points_vector;
  points_vector.reserve(num_points);
  for (int i = 0; i < num_points; ++i) {
    points_vector.push_back(Point{points[2 * i], points[2 * i + 1]});
  }
  return ValidateAndHoistEasingFunction(
      jni_env_pass_through,
      EasingFunction::Linear{.points = std::move(points_vector)},
      throw_from_status_callback);
}

void EasingFunctionNative_free(int64_t native_ptr) {
  DeleteNativeEasingFunction(native_ptr);
}

int EasingFunctionNative_getParametersType(int64_t native_ptr) {
  constexpr auto visitor = absl::Overload{
      [](const EasingFunction::Predefined&) { return kPredefined; },
      [](const EasingFunction::CubicBezier&) { return kCubicBezier; },
      [](const EasingFunction::Steps&) { return kSteps; },
      [](const EasingFunction::Linear&) { return kLinear; },
  };
  return std::visit(visitor, CastToEasingFunction(native_ptr).parameters);
}

int EasingFunctionNative_getPredefinedValueInt(int64_t native_ptr) {
  return static_cast<int>(std::get<EasingFunction::Predefined>(
      CastToEasingFunction(native_ptr).parameters));
}

float EasingFunctionNative_getCubicBezierX1(int64_t native_ptr) {
  return std::get<EasingFunction::CubicBezier>(
             CastToEasingFunction(native_ptr).parameters)
      .x1;
}

float EasingFunctionNative_getCubicBezierY1(int64_t native_ptr) {
  return std::get<EasingFunction::CubicBezier>(
             CastToEasingFunction(native_ptr).parameters)
      .y1;
}

float EasingFunctionNative_getCubicBezierX2(int64_t native_ptr) {
  return std::get<EasingFunction::CubicBezier>(
             CastToEasingFunction(native_ptr).parameters)
      .x2;
}

float EasingFunctionNative_getCubicBezierY2(int64_t native_ptr) {
  return std::get<EasingFunction::CubicBezier>(
             CastToEasingFunction(native_ptr).parameters)
      .y2;
}

int EasingFunctionNative_getLinearNumPoints(int64_t native_ptr) {
  return static_cast<int>(std::get<EasingFunction::Linear>(
                              CastToEasingFunction(native_ptr).parameters)
                              .points.size());
}

float EasingFunctionNative_getLinearPointX(int64_t native_ptr, int index) {
  return std::get<EasingFunction::Linear>(
             CastToEasingFunction(native_ptr).parameters)
      .points[index]
      .x;
}

float EasingFunctionNative_getLinearPointY(int64_t native_ptr, int index) {
  return std::get<EasingFunction::Linear>(
             CastToEasingFunction(native_ptr).parameters)
      .points[index]
      .y;
}

int EasingFunctionNative_getStepsCount(int64_t native_ptr) {
  return std::get<EasingFunction::Steps>(
             CastToEasingFunction(native_ptr).parameters)
      .step_count;
}

int EasingFunctionNative_getStepsPositionInt(int64_t native_ptr) {
  return static_cast<int>(std::get<EasingFunction::Steps>(
                              CastToEasingFunction(native_ptr).parameters)
                              .step_position);
}

}  // extern "C"
