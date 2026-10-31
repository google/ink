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

#include "ink/types/physical_distance.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_physical_distance_bindings) {
  emscripten::class_<PhysicalDistance>("PhysicalDistance")
      .constructor<>()
      .class_function("zero", &PhysicalDistance::Zero)
      .class_function("centimeters", &PhysicalDistance::Centimeters)
      .class_function("meters", &PhysicalDistance::Meters)
      .class_function("inches", &PhysicalDistance::Inches)
      .function("isFinite", &PhysicalDistance::IsFinite)
      .function("toCentimeters", &PhysicalDistance::ToCentimeters)
      .function("toMeters", &PhysicalDistance::ToMeters)
      .function("toInches", &PhysicalDistance::ToInches);
}

}  // namespace
}  // namespace ink::web
