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

#include <emscripten/bind.h>

#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_brush_coat_bindings) {
  emscripten::class_<BrushCoat>("BrushCoat")
      .constructor<>()
      .constructor(+[](const BrushPaint& paint) {
        return BrushCoat{
            .tip = BrushTip{},
            .paint_preferences = {paint},
        };
      })
      .function(
          "getPaintPreferenceCount",
          +[](const BrushCoat& coat) {
            return static_cast<int>(coat.paint_preferences.size());
          })
      .function(
          "getPaintPreference",
          +[](const BrushCoat& coat, int index) {
            return coat.paint_preferences.at(index);
          })
      .function(
          "addPaintPreference", +[](BrushCoat& coat, const BrushPaint& paint) {
            coat.paint_preferences.push_back(paint);
          });
}

}  // namespace
}  // namespace ink::web
