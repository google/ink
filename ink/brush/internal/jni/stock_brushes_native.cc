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

#include "ink/brush/internal/jni/stock_brushes_native.h"

#include <cstdint>
#include <string>

#include "ink/brush/brush_paint.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/brush/stock_brushes.h"

using ::ink::BrushPaint;
using ::ink::native::NewNativeBrushBehavior;
using ::ink::native::NewNativeBrushFamily;
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

int64_t StockBrushesNative_createMarker(int version) {
  return NewNativeBrushFamily(Marker(MarkerVersion(version)));
}

int64_t StockBrushesNative_createDashedLine(int version) {
  return NewNativeBrushFamily(DashedLine(DashedLineVersion(version)));
}

int64_t StockBrushesNative_createPressurePen(int version) {
  return NewNativeBrushFamily(PressurePen(PressurePenVersion(version)));
}

int64_t StockBrushesNative_createHighlighter(int self_overlap, int version) {
  return NewNativeBrushFamily(
      Highlighter(static_cast<BrushPaint::SelfOverlap>(self_overlap),
                  HighlighterVersion(version)));
}

int64_t StockBrushesNative_createEmojiHighlighter(const char* client_texture_id,
                                                  bool show_mini_emoji_trail,
                                                  int self_overlap,
                                                  int version) {
  return NewNativeBrushFamily(EmojiHighlighter(
      client_texture_id == nullptr ? "" : client_texture_id,
      show_mini_emoji_trail, static_cast<BrushPaint::SelfOverlap>(self_overlap),
      EmojiHighlighterVersion(version)));
}

int64_t StockBrushesNative_createPredictionFadeOutBehavior() {
  return NewNativeBrushBehavior(PredictionFadeOutBehavior());
}

}  // extern "C"
