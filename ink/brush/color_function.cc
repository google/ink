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

#include "ink/brush/color_function.h"

#include <array>
#include <cmath>
#include <string>
#include <variant>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "ink/brush/version.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/point.h"

namespace ink {
namespace {

// Convert between `Color` and the (unpremultiplied) Oklab color space. These
// formulae come from https://bottosson.github.io/posts/oklab/; see also
// https://en.wikipedia.org/wiki/Oklab_color_space.
//
// LINT.IfChange(oklab_transform)
std::array<float, 4> ColorToOklab(const Color& color) {
  Color::RgbaFloat rgba =
      color.InColorSpace(ColorSpace::kSrgb).AsFloat(Color::Format::kLinear);

  float l =
      rgba.r * 0.4122214708f + rgba.g * 0.5363325363f + rgba.b * 0.0514459929f;
  float m =
      rgba.r * 0.2119034982f + rgba.g * 0.6806995451f + rgba.b * 0.1073969566f;
  float s =
      rgba.r * 0.0883024619f + rgba.g * 0.2817188376f + rgba.b * 0.6299787005f;

  float l_cbrt = std::cbrt(l);
  float m_cbrt = std::cbrt(m);
  float s_cbrt = std::cbrt(s);

  float ok_L =
      l_cbrt * 0.2104542553f + m_cbrt * 0.7936177850f - s_cbrt * 0.0040720468f;
  float ok_a =
      l_cbrt * 1.9779984951f - m_cbrt * 2.4285922050f + s_cbrt * 0.4505937099f;
  float ok_b =
      l_cbrt * 0.0259040371f + m_cbrt * 0.7827717662f - s_cbrt * 0.8086757660f;
  return {ok_L, ok_a, ok_b, rgba.a};
}

Color ColorFromOklab(const std::array<float, 4>& oklab,
                     ColorSpace color_space) {
  float ok_L = oklab[0];
  float ok_a = oklab[1];
  float ok_b = oklab[2];
  float alpha = oklab[3];

  float l_cbrt = ok_L + ok_a * +0.3963377774f + ok_b * +0.2158037573f;
  float m_cbrt = ok_L + ok_a * -0.1055613458f + ok_b * -0.0638541728f;
  float s_cbrt = ok_L + ok_a * -0.0894841775f + ok_b * -1.2914855480f;

  float l = l_cbrt * l_cbrt * l_cbrt;
  float m = m_cbrt * m_cbrt * m_cbrt;
  float s = s_cbrt * s_cbrt * s_cbrt;

  float r = l * +4.0767416621f + m * -3.3077115913f + s * +0.2309699292f;
  float g = l * -1.2684380046f + m * +2.6097574011f + s * -0.3413193965f;
  float b = l * -0.0041960863f + m * -0.7034186147f + s * +1.7076147010f;

  return Color::FromFloat(r, g, b, alpha, Color::Format::kLinear,
                          ColorSpace::kSrgb)
      .InColorSpace(color_space);
}
// LINT.ThenChange(
//   ../rendering/skia/common_internal/sksl_fragment_shader_helper_functions.h:oklab_transform,
//   ../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:oklab_transform,
// )

}  // namespace

Color ColorFunction::ApplyAll(absl::Span<const ColorFunction> functions,
                              const Color& color) {
  Color result = color;
  for (const ColorFunction& function : functions) {
    result = function(result);
  }
  return result;
}

Color ColorFunction::operator()(const Color& color) const {
  return std::visit([&color](const auto& params) { return params(color); },
                    parameters);
}

Color ColorFunction::OpacityMultiplier::operator()(const Color& color) const {
  return color.WithAlphaFloat(multiplier * color.GetAlphaFloat());
}

// LINT.IfChange(hsl_transform)
Color ColorFunction::HueOffset::operator()(const Color& color) const {
  std::array<float, 4> oklab = ColorToOklab(color);
  Point ab = AffineTransform::Rotate(offset).Apply(Point{oklab[1], oklab[2]});
  oklab[1] = ab.x;
  oklab[2] = ab.y;
  return ColorFromOklab(oklab, color.GetColorSpace());
}

Color ColorFunction::SaturationMultiplier::operator()(
    const Color& color) const {
  std::array<float, 4> oklab = ColorToOklab(color);
  oklab[1] *= multiplier;
  oklab[2] *= multiplier;
  return ColorFromOklab(oklab, color.GetColorSpace());
}

Color ColorFunction::LuminosityOffset::operator()(const Color& color) const {
  std::array<float, 4> oklab = ColorToOklab(color);
  oklab[0] += offset;
  return ColorFromOklab(oklab, color.GetColorSpace());
}
// LINT.ThenChange(
//   ../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hsl_and_opacity_shift
// )

Color ColorFunction::ReplaceColor::operator()(
    const Color& ignored_original_color) const {
  return color;
}

namespace brush_internal {
namespace {

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::OpacityMultiplier& opacity) {
  if (!std::isfinite(opacity.multiplier) || opacity.multiplier < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("`ColorFunction::OpacityMultiplier::multiplier` must be "
                     "finite and non-negative, got: ",
                     opacity.multiplier));
  }
  return absl::OkStatus();
}

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::HueOffset& hue) {
  if (!std::isfinite(hue.offset.ValueInRadians())) {
    return absl::InvalidArgumentError(
        absl::StrCat("`ColorFunction::HueOffset::offset` must be finite, got: ",
                     hue.offset));
  }
  return absl::OkStatus();
}

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::SaturationMultiplier& saturation) {
  if (!std::isfinite(saturation.multiplier) || saturation.multiplier < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("`ColorFunction::SaturationMultiplier::multiplier` must "
                     "be finite and non-negative, got: ",
                     saturation.multiplier));
  }
  return absl::OkStatus();
}

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::LuminosityOffset& luminosity) {
  if (!std::isfinite(luminosity.offset)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`ColorFunction::LuminosityOffset::offset` must be finite, got: ",
        luminosity.offset));
  }
  return absl::OkStatus();
}

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::ReplaceColor& replace) {
  return absl::OkStatus();
}

}  // namespace

absl::Status ValidateColorFunction(const ColorFunction& color_function) {
  return std::visit(
      [](const auto& params) {
        return ValidateColorFunctionParameters(params);
      },
      color_function.parameters);
}

namespace {

Version CalculateMinimumRequiredVersion(
    const ColorFunction::OpacityMultiplier& opacity) {
  return Version::k0();
}

Version CalculateMinimumRequiredVersion(const ColorFunction::HueOffset& hue) {
  return Version::kDevelopment();
}

Version CalculateMinimumRequiredVersion(
    const ColorFunction::SaturationMultiplier& saturation) {
  return Version::kDevelopment();
}

Version CalculateMinimumRequiredVersion(
    const ColorFunction::LuminosityOffset& luminosity) {
  return Version::kDevelopment();
}

Version CalculateMinimumRequiredVersion(
    const ColorFunction::ReplaceColor& replace) {
  return Version::k0();
}

Version CalculateMinimumRequiredVersion(
    const ColorFunction::Parameters& parameters) {
  return std::visit(
      [](const auto& params) {
        return CalculateMinimumRequiredVersion(params);
      },
      parameters);
}
}  // namespace

Version CalculateMinimumRequiredVersion(const ColorFunction& color_function) {
  return CalculateMinimumRequiredVersion(color_function.parameters);
}

std::string ToFormattedString(const ColorFunction& color_function) {
  return ToFormattedString(color_function.parameters);
}

std::string ToFormattedString(const ColorFunction::Parameters& parameters) {
  return std::visit(
      [](const auto& params) { return ToFormattedString(params); }, parameters);
}

std::string ToFormattedString(const ColorFunction::OpacityMultiplier& opacity) {
  return absl::StrCat("OpacityMultiplier{", opacity.multiplier, "}");
}

std::string ToFormattedString(const ColorFunction::HueOffset& hue) {
  return absl::StrCat("HueOffset{", hue.offset, "}");
}

std::string ToFormattedString(
    const ColorFunction::SaturationMultiplier& saturation) {
  return absl::StrCat("SaturationMultiplier{", saturation.multiplier, "}");
}

std::string ToFormattedString(
    const ColorFunction::LuminosityOffset& luminosity) {
  return absl::StrCat("LuminosityOffset{", luminosity.offset, "}");
}

std::string ToFormattedString(const ColorFunction::ReplaceColor& replace) {
  return absl::StrCat("ReplaceColor{", replace.color, "}");
}

}  // namespace brush_internal
}  // namespace ink
