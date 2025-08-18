// Copyright 2025 Google LLC
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

#ifndef INK_STROKES_BRUSH_COLOR_FUNCTION_H_
#define INK_STROKES_BRUSH_COLOR_FUNCTION_H_

#include <string>
#include <utility>
#include <variant>

#include "absl/status/status.h"
#include "ink/color/color.h"

namespace ink {

// A `ColorFunction` defines a mapping over colors. This is used by `BrushPaint`
// to transform the brush color for a given coat of paint, for example to apply
// opacity for one of the brush's coats, or to force one coat to a specific
// color.
//
// A default-constructed ColorFunction specifies an identity mapping that leaves
// the input color unchanged.
struct ColorFunction {
  struct OpacityMultiplier {
    float multiplier = 1;

    bool operator==(const OpacityMultiplier& other) const;
    bool operator!=(const OpacityMultiplier& other) const;
  };

  struct ReplaceColor {
    Color color;

    bool operator==(const ReplaceColor& other) const;
    bool operator!=(const ReplaceColor& other) const;
  };

  // Union of possible color function parameters.
  using Parameters = std::variant<OpacityMultiplier, ReplaceColor>;
  Parameters parameters;

  bool operator==(const ColorFunction& other) const;
  bool operator!=(const ColorFunction& other) const;
};

namespace brush_internal {

// Determines whether the given ColorFunction struct is valid to be used in a
// BrushFamily, and returns an error if not.
absl::Status ValidateColorFunction(const ColorFunction& color_function);

std::string ToFormattedString(const ColorFunction& color_function);
std::string ToFormattedString(const ColorFunction::Parameters& parameters);
std::string ToFormattedString(const ColorFunction::OpacityMultiplier& opacity);
std::string ToFormattedString(const ColorFunction::ReplaceColor& replace);

}  // namespace brush_internal

template <typename Sink>
void AbslStringify(Sink& sink, const ColorFunction& color_function) {
  sink.Append(brush_internal::ToFormattedString(color_function));
}

template <typename Sink>
void AbslStringify(Sink& sink, const ColorFunction::Parameters& parameters) {
  sink.Append(brush_internal::ToFormattedString(parameters));
}

template <typename Sink>
void AbslStringify(Sink& sink,
                   const ColorFunction::OpacityMultiplier& opacity) {
  sink.Append(brush_internal::ToFormattedString(opacity));
}

template <typename Sink>
void AbslStringify(Sink& sink, const ColorFunction::ReplaceColor& replace) {
  sink.Append(brush_internal::ToFormattedString(replace));
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::OpacityMultiplier& opacity) {
  return H::combine(std::move(h), opacity.multiplier);
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::ReplaceColor& replace) {
  return H::combine(std::move(h), replace.color);
}

template <typename H>
H AbslHashValue(H h, const ColorFunction& color_function) {
  return H::combine(std::move(h), color_function.parameters);
}

}  // namespace ink

#endif  // INK_STROKES_BRUSH_COLOR_FUNCTION_H_
