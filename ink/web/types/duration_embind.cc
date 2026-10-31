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

#include "ink/types/duration.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_duration_bindings) {
  emscripten::class_<Duration32>("Duration32")
      .constructor<>()
      .class_function("zero", &Duration32::Zero)
      .class_function("infinite", &Duration32::Infinite)
      .class_function("seconds", &Duration32::Seconds)
      .class_function("millis", &Duration32::Millis)
      .function("isFinite", &Duration32::IsFinite)
      .function("toSeconds", &Duration32::ToSeconds)
      .function("toMillis", &Duration32::ToMillis);
}

}  // namespace
}  // namespace ink::web
