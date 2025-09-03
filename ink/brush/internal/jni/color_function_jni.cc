// Copyright 2025 Google LLC
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
#include <variant>

#include "absl/functional/overload.h"
#include "absl/status/status.h"
#include "ink/brush/color_function.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/color/color.h"
#include "ink/color/internal/jni/color_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ::ink::Color;
using ::ink::ColorFunction;
using ::ink::jni::CastToColorFunction;
using ::ink::jni::ComputeColorLong;
using ::ink::jni::DeleteNativeColorFunction;
using ::ink::jni::JIntToColorSpace;
using ::ink::jni::NewNativeColorFunction;
using ::ink::jni::ThrowExceptionFromStatus;

jlong ValidateAndHoistColorFunctionOrThrow(ColorFunction::Parameters parameters,
                                           JNIEnv* env) {
  ColorFunction color_function{.parameters = std::move(parameters)};
  if (absl::Status status =
          ink::brush_internal::ValidateColorFunction(color_function);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  return NewNativeColorFunction(std::move(color_function));
}

static constexpr int kOpacityMultiplier = 0;
static constexpr int kReplaceColor = 1;

}  // namespace

extern "C" {

JNI_METHOD(brush, ColorFunctionNative, jlong, createOpacityMultiplier)
(JNIEnv* env, jobject thiz, jfloat multiplier) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::OpacityMultiplier{.multiplier = multiplier}, env);
}

JNI_METHOD(brush, ColorFunctionNative, jlong, createReplaceColor)
(JNIEnv* env, jobject thiz, jfloat color_red, jfloat color_green,
 jfloat color_blue, jfloat color_alpha, jint color_space_id) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::ReplaceColor{
          .color = Color::FromFloat(color_red, color_green, color_blue,
                                    color_alpha, Color::Format::kGammaEncoded,
                                    JIntToColorSpace(color_space_id))},
      env);
}

JNI_METHOD(brush, ColorFunctionNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  DeleteNativeColorFunction(native_pointer);
}

JNI_METHOD(brush, ColorFunctionNative, jlong, getParametersType)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  constexpr auto visitor = absl::Overload{
      [](const ColorFunction::OpacityMultiplier&) {
        return kOpacityMultiplier;
      },
      [](const ColorFunction::ReplaceColor&) { return kReplaceColor; },
  };
  return static_cast<jint>(
      std::visit(visitor, CastToColorFunction(native_pointer).parameters));
}

JNI_METHOD(brush, ColorFunctionNative, jfloat, getOpacityMultiplier)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return std::get<ColorFunction::OpacityMultiplier>(
             CastToColorFunction(native_pointer).parameters)
      .multiplier;
}

JNI_METHOD(brush, ColorFunctionNative, jlong, computeReplaceColorLong)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return ComputeColorLong(env,
                          std::get<ColorFunction::ReplaceColor>(
                              CastToColorFunction(native_pointer).parameters)
                              .color);
}

}  // extern "C"
