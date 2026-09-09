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

// LINT.IfChange(hcl_transform)
Color ColorFunction::HueOffset::operator()(const Color& color) const {
  Color::OklabFloat oklab = color.AsOklab();
  Point ok_ab =
      AffineTransform::Rotate(offset).Apply(Point{oklab.ok_a, oklab.ok_b});
  oklab.ok_a = ok_ab.x;
  oklab.ok_b = ok_ab.y;
  return Color::FromOklab(oklab, color.GetColorSpace());
}

Color ColorFunction::ChromaMultiplier::operator()(const Color& color) const {
  Color::OklabFloat oklab = color.AsOklab();
  oklab.ok_a *= multiplier;
  oklab.ok_b *= multiplier;
  return Color::FromOklab(oklab, color.GetColorSpace());
}

Color ColorFunction::LightnessOffset::operator()(const Color& color) const {
  Color::OklabFloat oklab = color.AsOklab();
  oklab.ok_L += offset;
  return Color::FromOklab(oklab, color.GetColorSpace());
}
// LINT.ThenChange(
//   ../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hcl_and_opacity_shift
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
    const ColorFunction::ChromaMultiplier& chroma) {
  if (!std::isfinite(chroma.multiplier) || chroma.multiplier < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("`ColorFunction::ChromaMultiplier::multiplier` must "
                     "be finite and non-negative, got: ",
                     chroma.multiplier));
  }
  return absl::OkStatus();
}

absl::Status ValidateColorFunctionParameters(
    const ColorFunction::LightnessOffset& lightness) {
  if (!std::isfinite(lightness.offset)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`ColorFunction::LightnessOffset::offset` must be finite, got: ",
        lightness.offset));
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
    const ColorFunction::ChromaMultiplier& chroma) {
  return Version::kDevelopment();
}

Version CalculateMinimumRequiredVersion(
    const ColorFunction::LightnessOffset& lightness) {
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

std::string ToFormattedString(const ColorFunction::ChromaMultiplier& chroma) {
  return absl::StrCat("ChromaMultiplier{", chroma.multiplier, "}");
}

std::string ToFormattedString(const ColorFunction::LightnessOffset& lightness) {
  return absl::StrCat("LightnessOffset{", lightness.offset, "}");
}

std::string ToFormattedString(const ColorFunction::ReplaceColor& replace) {
  return absl::StrCat("ReplaceColor{", replace.color, "}");
}

}  // namespace brush_internal
}  // namespace ink
