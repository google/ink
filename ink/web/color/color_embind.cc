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

#include "ink/color/color.h"

namespace ink::web {
namespace {

Color ColorFromRgba(float r, float g, float b, float a) {
  return Color::FromFloat(r, g, b, a);
}

float GetRed(const Color& color) {
  return color.AsFloat(Color::Format::kGammaEncoded).r;
}
float GetGreen(const Color& color) {
  return color.AsFloat(Color::Format::kGammaEncoded).g;
}
float GetBlue(const Color& color) {
  return color.AsFloat(Color::Format::kGammaEncoded).b;
}
float GetAlpha(const Color& color) {
  return color.AsFloat(Color::Format::kGammaEncoded).a;
}

EMSCRIPTEN_BINDINGS(ink_color_bindings) {
  emscripten::class_<Color>("Color")
      .constructor<>()
      .class_function("fromRgba", &ColorFromRgba)
      .class_function("black", &Color::Black)
      .class_function("white", &Color::White)
      .class_function("red", &Color::Red)
      .class_function("green", &Color::Green)
      .class_function("blue", &Color::Blue)
      .class_function("transparent", &Color::Transparent)
      .function("getRed", &GetRed)
      .function("getGreen", &GetGreen)
      .function("getBlue", &GetBlue)
      .function("getAlpha", &GetAlpha);
}

}  // namespace
}  // namespace ink::web
