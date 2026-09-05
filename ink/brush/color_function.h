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
#include <type_traits>
#include <utility>
#include <variant>

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "ink/brush/version.h"
#include "ink/color/color.h"
#include "ink/geometry/angle.h"

namespace ink {

// A `ColorFunction` defines a mapping over colors. This is used by `BrushPaint`
// to transform the brush color for a given coat of paint, for example to apply
// opacity for one of the brush's coats, or to force one coat to a specific
// color.
//
// A default-constructed ColorFunction specifies an identity mapping that leaves
// the input color unchanged.
struct ColorFunction {
  static Color ApplyAll(absl::Span<const ColorFunction> functions,
                        const Color& color);

  // Applies a multiplier to the color's opacity.  The multiplier must be finite
  // and nonnegative.  Typical multiplier values range between 0 and 1.
  struct OpacityMultiplier {
    float multiplier = 1;

    Color operator()(const Color& color) const;
    friend bool operator==(const OpacityMultiplier&,
                           const OpacityMultiplier&) = default;
  };

  // Applies an additive offset to the color's hue angle, while maintaining the
  // same level of perceived lightness.  The offset angle must be finite, but
  // can be positive or negative (or zero).
  struct HueOffset {
    Angle offset;

    Color operator()(const Color& color) const;
    friend bool operator==(const HueOffset&, const HueOffset&) = default;
  };

  // Applies a multiplier to the color's chroma.  A value greater than 1 makes
  // the color more saturated, a value less than 1 makes the color less
  // saturated, and a value of 0 makes the color grayscale.  The multiplier must
  // be finite and nonnegative.  Typical multiplier values range between 0 and
  // 1.
  struct ChromaMultiplier {
    float multiplier = 1;

    Color operator()(const Color& color) const;
    friend bool operator==(const ChromaMultiplier&,
                           const ChromaMultiplier&) = default;
  };

  // Applies an additive offset to the color's perceived lightness.  The offset
  // must be finite, but can be positive or negative (or zero).  Typical offset
  // values range between -1 and 1.
  struct LightnessOffset {
    float offset = 0;

    Color operator()(const Color& color) const;
    friend bool operator==(const LightnessOffset&,
                           const LightnessOffset&) = default;
  };

  // Ignores the original color and replaces it with a fixed color.
  struct ReplaceColor {
    Color color;

    Color operator()(const Color& color) const;
    friend bool operator==(const ReplaceColor&, const ReplaceColor&) = default;
  };

  // Union of possible color function parameters.
  // LINT.IfChange(color_function_mapping)
  using Parameters =
      std::variant<OpacityMultiplier, HueOffset, ChromaMultiplier,
                   LightnessOffset, ReplaceColor>;
  // LINT.ThenChange(../../java/androidx/ink/brush/main/BrushPaint.kt:color_function_mapping)
  Parameters parameters;

  Color operator()(const Color& color) const;
  friend bool operator==(const ColorFunction&, const ColorFunction&) = default;
};

namespace brush_internal {

// Determines whether the given ColorFunction struct is valid to be used in a
// BrushFamily, and returns an error if not.
absl::Status ValidateColorFunction(const ColorFunction& color_function);

// Calculates the minimum version of the Ink library that is required to use
// this color function.
Version CalculateMinimumRequiredVersion(const ColorFunction& color_function);

std::string ToFormattedString(const ColorFunction& color_function);
std::string ToFormattedString(const ColorFunction::Parameters& parameters);
std::string ToFormattedString(const ColorFunction::OpacityMultiplier& opacity);
std::string ToFormattedString(const ColorFunction::HueOffset& hue);
std::string ToFormattedString(const ColorFunction::ChromaMultiplier& chroma);
std::string ToFormattedString(const ColorFunction::LightnessOffset& lightness);
std::string ToFormattedString(const ColorFunction::ReplaceColor& replace);

}  // namespace brush_internal

template <typename Sink>
void AbslStringify(Sink& sink, const ColorFunction& color_function) {
  sink.Append(brush_internal::ToFormattedString(color_function));
}

template <typename Sink, typename P>
std::enable_if_t<std::is_convertible_v<P, ColorFunction::Parameters>, void>
AbslStringify(Sink& sink, const P& params) {
  sink.Append(brush_internal::ToFormattedString(params));
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::OpacityMultiplier& opacity) {
  return H::combine(std::move(h), opacity.multiplier);
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::HueOffset& hue) {
  return H::combine(std::move(h), hue.offset);
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::ChromaMultiplier& chroma) {
  return H::combine(std::move(h), chroma.multiplier);
}

template <typename H>
H AbslHashValue(H h, const ColorFunction::LightnessOffset& lightness) {
  return H::combine(std::move(h), lightness.offset);
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
