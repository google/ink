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

#include <utility>

#include "absl/log/absl_check.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/color/color.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_brush_bindings) {
  emscripten::class_<Brush>("Brush")
      .constructor<>()
      .class_function(
          "create",
          +[](const BrushFamily& family, const Color& color, float size,
              float epsilon) {
            auto status_or = Brush::Create(family, color, size, epsilon);
            ABSL_CHECK_OK(status_or.status());
            return *std::move(status_or);
          })
      .function(
          "getFamily", +[](const Brush& brush) { return brush.GetFamily(); })
      .function(
          "getColor", +[](const Brush& brush) { return brush.GetColor(); })
      .function("getSize", &Brush::GetSize)
      .function("getEpsilon", &Brush::GetEpsilon)
      .function(
          "coatCount", +[](const Brush& brush) {
            return static_cast<int>(brush.CoatCount());
          });
}

}  // namespace
}  // namespace ink::web
