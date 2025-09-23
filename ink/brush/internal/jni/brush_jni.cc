// Copyright 2024-2025 Google LLC
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

namespace {

using ::ink::Brush;
using ::ink::BrushFamily;
using ::ink::Color;
using ::ink::jni::CastToBrush;
using ::ink::jni::CastToBrushFamily;
using ::ink::jni::ComputeColorLong;
using ::ink::jni::DeleteNativeBrush;
using ::ink::jni::JIntToColorSpace;
using ::ink::jni::NewNativeBrush;
using ::ink::jni::NewNativeBrushFamily;
using ::ink::jni::ThrowExceptionFromStatus;

}  // namespace

extern "C" {

// Construct a native Brush and return a pointer to it as a long.
JNI_METHOD(brush, BrushNative, jlong, create)
(JNIEnv* env, jobject object, jlong family_native_pointer, jfloat color_red,
 jfloat color_green, jfloat color_blue, jfloat color_alpha, jint color_space_id,
 jfloat size, jfloat epsilon) {
  const BrushFamily& family = CastToBrushFamily(family_native_pointer);

  const Color color = Color::FromFloat(
      color_red, color_green, color_blue, color_alpha,
      Color::Format::kGammaEncoded, JIntToColorSpace(color_space_id));

  auto brush = Brush::Create(family, color, size, epsilon);
  if (!brush.ok()) {
    ThrowExceptionFromStatus(env, brush.status());
    return -1;  // Unused return value.
  }

  return NewNativeBrush(*std::move(brush));
}

JNI_METHOD(brush, BrushNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  DeleteNativeBrush(native_pointer);
}

JNI_METHOD(brush, BrushNative, jlong, computeComposeColorLong)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return ComputeColorLong(env, CastToBrush(native_pointer).GetColor());
}

JNI_METHOD(brush, BrushNative, jfloat, getSize)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return CastToBrush(native_pointer).GetSize();
}

JNI_METHOD(brush, BrushNative, jfloat, getEpsilon)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return CastToBrush(native_pointer).GetEpsilon();
}

JNI_METHOD(brush, BrushNative, jlong, newCopyOfBrushFamily)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return NewNativeBrushFamily(CastToBrush(native_pointer).GetFamily());
}

}  // extern "C"
