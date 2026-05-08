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

#include <jni.h>

#include <string>

#include "ink/brush/internal/jni/stock_brushes_native.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"

using ::ink::jni::JStringToStdString;

extern "C" {

JNI_METHOD(brush, StockBrushesNative, jlong, createMarker)
(JNIEnv* env, jobject object, jint version) {
  return StockBrushesNative_createMarker(version);
}

JNI_METHOD(brush, StockBrushesNative, jlong, createDashedLine)
(JNIEnv* env, jobject object, jint version) {
  return StockBrushesNative_createDashedLine(version);
}

JNI_METHOD(brush, StockBrushesNative, jlong, createPressurePen)
(JNIEnv* env, jobject object, jint version) {
  return StockBrushesNative_createPressurePen(version);
}

JNI_METHOD(brush, StockBrushesNative, jlong, createHighlighter)
(JNIEnv* env, jobject object, jint self_overlap, jint version) {
  return StockBrushesNative_createHighlighter(self_overlap, version);
}

JNI_METHOD(brush, StockBrushesNative, jlong, createEmojiHighlighter)
(JNIEnv* env, jobject object, jstring client_texture_id,
 jboolean show_mini_emoji_trail, jint self_overlap, jint version) {
  std::string c_texture_id = JStringToStdString(env, client_texture_id);
  return StockBrushesNative_createEmojiHighlighter(
      c_texture_id.c_str(), show_mini_emoji_trail, self_overlap, version);
}

JNI_METHOD(brush, StockBrushesNative, jlong, createPredictionFadeOutBehavior)
(JNIEnv* env, jobject object) {
  return StockBrushesNative_createPredictionFadeOutBehavior();
}

}  // extern "C"
