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

#include "absl/log/absl_check.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_stroke_input_batch_bindings) {
  emscripten::class_<StrokeInputBatch>("StrokeInputBatch")
      .constructor<>()
      .function(
          "append",
          +[](StrokeInputBatch& batch, const StrokeInput& input) {
            ABSL_CHECK_OK(batch.Append(input));
          })
      .function("size", &StrokeInputBatch::Size)
      .function("isEmpty", &StrokeInputBatch::IsEmpty)
      .function("get", &StrokeInputBatch::Get)
      .function("clear", &StrokeInputBatch::Clear);
}

}  // namespace
}  // namespace ink::web
