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

// LINT.IfChange(yiq_transform)
std::array<float, 4> ColorToYiqa(const Color& color) {
  Color::RgbaFloat rgba =
      color.InColorSpace(ColorSpace::kSrgb).AsFloat(Color::Format::kLinear);
  float y = rgba.r * +0.299 + rgba.g * +0.587 + rgba.b * +0.114;
  float i = rgba.r * +0.596 + rgba.g * -0.275 + rgba.b * -0.321;
  float q = rgba.r * +0.212 + rgba.g * -0.523 + rgba.b * +0.311;
  return {y, i, q, rgba.a};
}

Color ColorFromYiqa(const std::array<float, 4>& yiqa, ColorSpace color_space) {
  float y = yiqa[0];
  float i = yiqa[1];
  float q = yiqa[2];
  float a = yiqa[3];

  float r = y + i * +0.956 + q * +0.621;
  float g = y + i * -0.272 + q * -0.647;
  float b = y + i * -1.107 + q * +1.704;

  return Color::FromFloat(r, g, b, a, Color::Format::kLinear, ColorSpace::kSrgb)
      .InColorSpace(color_space);
}
// LINT.ThenChange(
//   ../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hsl_and_opacity_shift
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

Color ColorFunction::HueOffset::operator()(const Color& color) const {
  std::array<float, 4> yiqa = ColorToYiqa(color);
  Point iq = AffineTransform::Rotate(-offset).Apply(Point{yiqa[1], yiqa[2]});
  yiqa[1] = iq.x;
  yiqa[2] = iq.y;
  return ColorFromYiqa(yiqa, color.GetColorSpace());
}

Color ColorFunction::SaturationMultiplier::operator()(
    const Color& color) const {
  std::array<float, 4> yiqa = ColorToYiqa(color);
  yiqa[1] *= multiplier;
  yiqa[2] *= multiplier;
  return ColorFromYiqa(yiqa, color.GetColorSpace());
}

Color ColorFunction::LuminosityOffset::operator()(const Color& color) const {
  std::array<float, 4> yiqa = ColorToYiqa(color);
  yiqa[0] += offset;
  return ColorFromYiqa(yiqa, color.GetColorSpace());
}

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
  return Version::k0Jetpack1_0_0();
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
  return Version::k0Jetpack1_0_0();
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
