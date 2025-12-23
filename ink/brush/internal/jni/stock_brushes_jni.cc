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

#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/brush/stock_brushes.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"

namespace {

using ::ink::BrushFamily;
using ::ink::BrushPaint;
using ::ink::jni::JStringToStdString;
using ::ink::jni::NewNativeBrushBehavior;
using ::ink::jni::NewNativeBrushFamily;
using ::ink::stock_brushes::DashedLine;
using ::ink::stock_brushes::DashedLineVersion;
using ::ink::stock_brushes::EmojiHighlighter;
using ::ink::stock_brushes::EmojiHighlighterVersion;
using ::ink::stock_brushes::Highlighter;
using ::ink::stock_brushes::HighlighterVersion;
using ::ink::stock_brushes::Marker;
using ::ink::stock_brushes::MarkerVersion;
using ::ink::stock_brushes::PredictionFadeOutBehavior;
using ::ink::stock_brushes::PressurePen;
using ::ink::stock_brushes::PressurePenVersion;

extern "C" {

JNI_METHOD(brush, StockBrushesNative, jlong, marker)
(JNIEnv* env, jobject object, jint version) {
  BrushFamily family = Marker(MarkerVersion(version));
  return NewNativeBrushFamily(std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, dashedLine)
(JNIEnv* env, jobject object, jint version) {
  BrushFamily family = DashedLine(DashedLineVersion(version));
  return NewNativeBrushFamily(std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, pressurePen)
(JNIEnv* env, jobject object, jint version) {
  BrushFamily family = PressurePen(PressurePenVersion(version));
  return NewNativeBrushFamily(std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, highlighter)
(JNIEnv* env, jobject object, jint self_overlap, jint version) {
  BrushFamily family =
      Highlighter(static_cast<BrushPaint::SelfOverlap>(self_overlap),
                  HighlighterVersion(version));
  return NewNativeBrushFamily(std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, emojiHighlighter)
(JNIEnv* env, jobject object, jstring client_texture_id,
 jboolean show_mini_emoji_trail, jint self_overlap, jint version) {
  BrushFamily family = EmojiHighlighter(
      JStringToStdString(env, client_texture_id), show_mini_emoji_trail,
      static_cast<BrushPaint::SelfOverlap>(self_overlap),
      EmojiHighlighterVersion(version));
  return NewNativeBrushFamily(std::move(family));
}

JNI_METHOD(brush, StockBrushesNative, jlong, predictionFadeOutBehavior)
(JNIEnv* env, jobject object) {
  return NewNativeBrushBehavior(std::move(PredictionFadeOutBehavior()));
}
}  // extern "C"
}  // namespace
