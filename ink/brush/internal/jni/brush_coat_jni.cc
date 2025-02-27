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
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"

namespace {

using ::ink::BrushCoat;
using ::ink::BrushPaint;
using ::ink::BrushTip;
using ::ink::jni::CastToBrushCoat;
using ::ink::jni::CastToBrushPaint;
using ::ink::jni::CastToBrushTip;

}  // namespace

extern "C" {

// Construct a native BrushCoat and return a pointer to it as a long.
JNI_METHOD(brush, BrushCoatNative, jlong, create)
(JNIEnv* env, jobject thiz, jlongArray tip_native_pointer_array,
 jlong paint_native_pointer) {
  std::vector<BrushTip> tips;
  const jsize num_tips = env->GetArrayLength(tip_native_pointer_array);
  tips.reserve(num_tips);
  jlong* tip_pointers =
      env->GetLongArrayElements(tip_native_pointer_array, nullptr);
  ABSL_CHECK(tip_pointers != nullptr);
  for (jsize i = 0; i < num_tips; ++i) {
    tips.push_back(CastToBrushTip(tip_pointers[i]));
  }
  env->ReleaseLongArrayElements(
      tip_native_pointer_array, tip_pointers,
      // No need to copy back the array, which is not modified.
      JNI_ABORT);

  const BrushPaint& paint = CastToBrushPaint(paint_native_pointer);

  return reinterpret_cast<jlong>(new BrushCoat{
      .tips = std::move(tips),
      .paint = paint,
  });
}

JNI_METHOD(brush, BrushCoatNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  delete reinterpret_cast<BrushCoat*>(native_pointer);
}

JNI_METHOD(brush, BrushCoatNative, jint, getBrushTipCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToBrushCoat(native_pointer).tips.size();
}

JNI_METHOD(brush, BrushCoatNative, jlong, newCopyOfBrushTip)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint tip_index) {
  const BrushCoat& coat = CastToBrushCoat(native_pointer);
  return reinterpret_cast<jlong>(new BrushTip(coat.tips[tip_index]));
}

JNI_METHOD(brush, BrushCoatNative, jlong, newCopyOfBrushPaint)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  const BrushCoat& coat = CastToBrushCoat(native_pointer);
  return reinterpret_cast<jlong>(new BrushPaint(coat.paint));
}

}  // extern "C"
