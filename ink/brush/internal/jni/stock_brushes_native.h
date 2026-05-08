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

#ifndef INK_BRUSH_INTERNAL_JNI_STOCK_BRUSHES_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_STOCK_BRUSHES_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t StockBrushesNative_createMarker(int version);

int64_t StockBrushesNative_createDashedLine(int version);

int64_t StockBrushesNative_createPressurePen(int version);

int64_t StockBrushesNative_createHighlighter(int self_overlap, int version);

int64_t StockBrushesNative_createEmojiHighlighter(const char* client_texture_id,
                                                  bool show_mini_emoji_trail,
                                                  int self_overlap,
                                                  int version);

int64_t StockBrushesNative_createPredictionFadeOutBehavior();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_STOCK_BRUSHES_NATIVE_H_
