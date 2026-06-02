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

#include "ink/brush/internal/jni/brush_native.h"

#include <cstdint>
#include <utility>

#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/color/internal/jni/color_native_helper.h"

using ::ink::Brush;
using ::ink::BrushFamily;
using ::ink::Color;
using ::ink::ColorSpace;
using ::ink::native::CastToBrush;
using ::ink::native::CastToBrushFamily;
using ::ink::native::ComputeColorLong;
using ::ink::native::DeleteNativeBrush;
using ::ink::native::NewNativeBrush;
using ::ink::native::NewNativeBrushFamily;

extern "C" {

int64_t BrushNative_create(
    void* jni_env_pass_through, int64_t family_native_pointer, float color_red,
    float color_green, float color_blue, float color_alpha, int color_space_id,
    float size, float epsilon,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  const BrushFamily& family = CastToBrushFamily(family_native_pointer);

  const Color color = Color::FromFloat(
      color_red, color_green, color_blue, color_alpha,
      Color::Format::kGammaEncoded, static_cast<ColorSpace>(color_space_id));

  absl::StatusOr<Brush> brush = Brush::Create(family, color, size, epsilon);
  if (!brush.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(brush.status().code()),
                               brush.status().ToString().c_str());
    return 0;
  }

  return NewNativeBrush(*std::move(brush));
}

void BrushNative_free(int64_t native_pointer) {
  DeleteNativeBrush(native_pointer);
}

int64_t BrushNative_computeComposeColorLong(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t (*compose_color_long_from_components_callback)(void*, int, float,
                                                           float, float,
                                                           float)) {
  return ComputeColorLong(jni_env_pass_through,
                          CastToBrush(native_pointer).GetColor(),
                          compose_color_long_from_components_callback);
}

float BrushNative_getSize(int64_t native_pointer) {
  return CastToBrush(native_pointer).GetSize();
}

float BrushNative_getEpsilon(int64_t native_pointer) {
  return CastToBrush(native_pointer).GetEpsilon();
}

int64_t BrushNative_newCopyOfBrushFamily(int64_t native_pointer) {
  return NewNativeBrushFamily(CastToBrush(native_pointer).GetFamily());
}

}  // extern "C"
