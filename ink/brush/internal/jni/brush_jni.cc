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

#include <utility>

#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/color/color.h"
#include "ink/color/internal/jni/color_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"

extern "C" {

// Construct a native Brush and return a pointer to it as a long.
JNI_METHOD(brush, Brush, jlong, nativeCreateBrush)
(JNIEnv* env, jobject thiz, jlong family_native_pointer, jfloat color_red,
 jfloat color_green, jfloat color_blue, jfloat color_alpha, jint color_space_id,
 jfloat size, jfloat epsilon) {
  const ink::BrushFamily& family =
      ink::CastToBrushFamily(family_native_pointer);

  const ink::Color color = ink::Color::FromFloat(
      color_red, color_green, color_blue, color_alpha,
      ink::Color::Format::kGammaEncoded, ink::JIntToColorSpace(color_space_id));

  auto brush = ink::Brush::Create(family, color, size, epsilon);
  if (!brush.ok()) {
    ink::jni::ThrowExceptionFromStatus(env, brush.status());
    return -1;  // Unused return value.
  }

  return reinterpret_cast<jlong>(new ink::Brush(*std::move(brush)));
}

JNI_METHOD(brush, Brush, void, nativeFreeBrush)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<ink::Brush*>(native_pointer);
}
}
