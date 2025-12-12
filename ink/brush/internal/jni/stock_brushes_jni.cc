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

#include "absl/status/statusor.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/brush/stock_brushes.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"

namespace {

using ::ink::BrushFamily;
using ::ink::BrushPaint;
using ::ink::DashedLineVersion;
using ::ink::EmojiHighlighterVersion;
using ::ink::HighlighterVersion;
using ::ink::MarkerVersion;
using ::ink::PressurePenVersion;
using ::ink::jni::JStringToStdString;
using ::ink::jni::NewNativeBrushBehavior;
using ::ink::jni::NewNativeBrushFamily;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::stock_brushes::dashedLine;
using ::ink::stock_brushes::emojiHighlighter;
using ::ink::stock_brushes::highlighter;
using ::ink::stock_brushes::marker;
using ::ink::stock_brushes::pencilUnstable;
using ::ink::stock_brushes::predictionFadeOutBehavior;
using ::ink::stock_brushes::pressurePen;

extern "C" {

JNI_METHOD(brush, StockBrushesNative, jlong, marker)
(JNIEnv* env, jobject object, jstring version) {
  absl::StatusOr<BrushFamily> family =
      marker(MarkerVersion(JStringToStdString(env, version)));
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, dashedLine)
(JNIEnv* env, jobject object, jstring version) {
  absl::StatusOr<BrushFamily> family =
      dashedLine(DashedLineVersion(JStringToStdString(env, version)));
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, pressurePen)
(JNIEnv* env, jobject object, jstring version) {
  absl::StatusOr<BrushFamily> family =
      pressurePen(PressurePenVersion(JStringToStdString(env, version)));
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, highlighter)
(JNIEnv* env, jobject object, jint self_overlap, jstring version) {
  absl::StatusOr<BrushFamily> family =
      highlighter(static_cast<BrushPaint::SelfOverlap>(self_overlap),
                  HighlighterVersion(JStringToStdString(env, version)));
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, emojiHighlighter)
(JNIEnv* env, jobject object, jstring client_texture_id,
 jboolean show_mini_emoji_trail, jint self_overlap, jstring version) {
  absl::StatusOr<BrushFamily> family = emojiHighlighter(
      JStringToStdString(env, client_texture_id), show_mini_emoji_trail,
      static_cast<BrushPaint::SelfOverlap>(self_overlap),
      EmojiHighlighterVersion(JStringToStdString(env, version)));
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, pencilUnstable)
(JNIEnv* env, jobject object) {
  absl::StatusOr<BrushFamily> family = pencilUnstable();
  if (!family.ok()) {
    ThrowExceptionFromStatus(env, family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, predictionFadeOutBehavior)
(JNIEnv* env, jobject object) {
  return NewNativeBrushBehavior(std::move(predictionFadeOutBehavior()));
}
}  // extern "C"
}  // namespace
