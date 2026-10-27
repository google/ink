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

#include "ink/geometry/vec.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_vec_bindings) {
  emscripten::class_<Vec>("Vec")
      .constructor<>()
      .constructor(+[](float x, float y) { return Vec{x, y}; })
      .property("x", &Vec::x)
      .property("y", &Vec::y)
      .function("magnitude", &Vec::Magnitude);
}

}  // namespace
}  // namespace ink::web
