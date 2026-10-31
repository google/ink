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
#include <emscripten/val.h>

#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_brush_family_bindings) {
  emscripten::class_<BrushFamily>("BrushFamily")
      .constructor<>()
      .class_function(
          "fromCoat",
          +[](const BrushCoat& coat) {
            auto status_or = BrushFamily::Create(absl::MakeConstSpan(&coat, 1));
            ABSL_CHECK_OK(status_or.status());
            return *std::move(status_or);
          })
      .class_function(
          "fromCoats",
          +[](emscripten::val js_coats) {
            int length = js_coats["length"].as<int>();
            std::vector<BrushCoat> coats;
            coats.reserve(length);
            for (int i = 0; i < length; ++i) {
              coats.push_back(js_coats[i].as<BrushCoat>());
            }
            auto status_or = BrushFamily::Create(coats);
            ABSL_CHECK_OK(status_or.status());
            return *std::move(status_or);
          })
      .function(
          "getCoatCount",
          +[](const BrushFamily& family) {
            return static_cast<int>(family.GetCoats().size());
          })
      .function(
          "getCoat", +[](const BrushFamily& family, int index) {
            return family.GetCoats().at(index);
          });
}

}  // namespace
}  // namespace ink::web
