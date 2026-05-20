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

#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/color/color.h"
#include "ink/color/fuzz_domains.h"
#include "ink/color/type_matchers.h"
#include "ink/geometry/angle.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

static_assert(std::numeric_limits<float>::has_quiet_NaN);
constexpr float kNan = std::numeric_limits<float>::quiet_NaN();
constexpr float kInfinity = std::numeric_limits<float>::infinity();

TEST(ColorFunctionTest, SupportsAbslHash) {
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      ColorFunction{ColorFunction::OpacityMultiplier{0}},
      ColorFunction{ColorFunction::OpacityMultiplier{0.5}},
      ColorFunction{ColorFunction::OpacityMultiplier{1}},
      ColorFunction{ColorFunction::HueOffset{Angle::Degrees(-60)}},
      ColorFunction{ColorFunction::HueOffset{Angle::Degrees(0)}},
      ColorFunction{ColorFunction::HueOffset{Angle::Degrees(30)}},
      ColorFunction{ColorFunction::SaturationMultiplier{0}},
      ColorFunction{ColorFunction::SaturationMultiplier{0.5}},
      ColorFunction{ColorFunction::SaturationMultiplier{1}},
      ColorFunction{ColorFunction::LuminosityOffset{-0.5}},
      ColorFunction{ColorFunction::LuminosityOffset{0}},
      ColorFunction{ColorFunction::LuminosityOffset{0.5}},
      ColorFunction{ColorFunction::ReplaceColor{Color::Black()}},
      ColorFunction{ColorFunction::ReplaceColor{Color::Red()}},
  }));
}

TEST(ColorFunctionTest, StringifyOpacityMultiplier) {
  EXPECT_EQ(absl::StrCat(ColorFunction::OpacityMultiplier{.multiplier = 1}),
            "OpacityMultiplier{1}");
  EXPECT_EQ(absl::StrCat(ColorFunction::OpacityMultiplier{.multiplier = 0.25}),
            "OpacityMultiplier{0.25}");
}

TEST(ColorFunctionTest, StringifyHueOffset) {
  EXPECT_EQ(absl::StrCat(ColorFunction::HueOffset{.offset = Angle::Degrees(0)}),
            "HueOffset{0π}");
  EXPECT_EQ(
      absl::StrCat(ColorFunction::HueOffset{.offset = Angle::Degrees(180)}),
      "HueOffset{1π}");
}

TEST(ColorFunctionTest, StringifySaturationMultiplier) {
  EXPECT_EQ(absl::StrCat(ColorFunction::SaturationMultiplier{.multiplier = 1}),
            "SaturationMultiplier{1}");
  EXPECT_EQ(
      absl::StrCat(ColorFunction::SaturationMultiplier{.multiplier = 0.25}),
      "SaturationMultiplier{0.25}");
}

TEST(ColorFunctionTest, StringifyLuminosityOffset) {
  EXPECT_EQ(absl::StrCat(ColorFunction::LuminosityOffset{.offset = 0}),
            "LuminosityOffset{0}");
  EXPECT_EQ(absl::StrCat(ColorFunction::LuminosityOffset{.offset = -0.25}),
            "LuminosityOffset{-0.25}");
}

TEST(ColorFunctionTest, StringifyReplaceColor) {
  EXPECT_THAT(absl::StrCat(ColorFunction::ReplaceColor{Color::Black()}),
              HasSubstr("ReplaceColor"));
}

TEST(ColorFunctionTest, StringifyColorFunctionParameters) {
  EXPECT_EQ(absl::StrCat(ColorFunction::Parameters{
                ColorFunction::OpacityMultiplier{.multiplier = 0.5}}),
            "OpacityMultiplier{0.5}");
  EXPECT_THAT(absl::StrCat(ColorFunction::Parameters{
                  ColorFunction::ReplaceColor{Color::Red()}}),
              HasSubstr("ReplaceColor"));
}

TEST(ColorFunctionTest, StringifyColorFunction) {
  EXPECT_EQ(absl::StrCat(ColorFunction{
                {ColorFunction::OpacityMultiplier{.multiplier = 0.5}}}),
            "OpacityMultiplier{0.5}");
  EXPECT_THAT(
      absl::StrCat(ColorFunction{{ColorFunction::ReplaceColor{Color::Red()}}}),
      HasSubstr("ReplaceColor"));
}

TEST(ColorFunctionTest, OpacityMultiplierEqualAndNotEqual) {
  ColorFunction::OpacityMultiplier opacity_multiplier =
      ColorFunction::OpacityMultiplier{0.5};

  EXPECT_EQ(opacity_multiplier, ColorFunction::OpacityMultiplier{0.5});
  EXPECT_NE(opacity_multiplier, ColorFunction::OpacityMultiplier{0.25});
}

TEST(ColorFunctionTest, HueOffsetEqualAndNotEqual) {
  ColorFunction::HueOffset hue_offset =
      ColorFunction::HueOffset{Angle::Degrees(30)};

  EXPECT_EQ(hue_offset, ColorFunction::HueOffset{Angle::Degrees(30)});
  EXPECT_NE(hue_offset, ColorFunction::HueOffset{Angle::Degrees(60)});
}

TEST(ColorFunctionTest, SaturationMultiplierEqualAndNotEqual) {
  ColorFunction::SaturationMultiplier saturation_multiplier =
      ColorFunction::SaturationMultiplier{0.5};

  EXPECT_EQ(saturation_multiplier, ColorFunction::SaturationMultiplier{0.5});
  EXPECT_NE(saturation_multiplier, ColorFunction::SaturationMultiplier{0.25});
}

TEST(ColorFunctionTest, LuminosityOffsetEqualAndNotEqual) {
  ColorFunction::LuminosityOffset luminosity_offset =
      ColorFunction::LuminosityOffset{0.5};

  EXPECT_EQ(luminosity_offset, ColorFunction::LuminosityOffset{0.5});
  EXPECT_NE(luminosity_offset, ColorFunction::LuminosityOffset{0.25});
}

TEST(ColorFunctionTest, ReplaceColorEqualAndNotEqual) {
  ColorFunction::ReplaceColor replace_color =
      ColorFunction::ReplaceColor{Color::Red()};

  EXPECT_EQ(replace_color, ColorFunction::ReplaceColor{Color::Red()});
  EXPECT_NE(replace_color, ColorFunction::ReplaceColor{Color::Blue()});
}

TEST(ColorFunctionTest, ColorFunctionEqualAndNotEqual) {
  ColorFunction opacity_multiplier{ColorFunction::OpacityMultiplier{0.5}};
  ColorFunction replace_color{ColorFunction::ReplaceColor{Color::Green()}};

  EXPECT_EQ(opacity_multiplier,
            ColorFunction{ColorFunction::OpacityMultiplier{0.5}});
  EXPECT_EQ(replace_color,
            ColorFunction{ColorFunction::ReplaceColor{Color::Green()}});

  EXPECT_NE(opacity_multiplier, replace_color);
  EXPECT_NE(opacity_multiplier,
            ColorFunction({ColorFunction::OpacityMultiplier{0.75}}));
  EXPECT_NE(replace_color,
            ColorFunction({ColorFunction::ReplaceColor{Color::Magenta()}}));
}

TEST(ColorFunctionTest, ValidateOpacityMultiplier) {
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::OpacityMultiplier{.multiplier = 0}}),
              IsOk());
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::OpacityMultiplier{.multiplier = 2.5}}),
              IsOk());

  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::OpacityMultiplier{.multiplier = -1}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("non-negative")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::OpacityMultiplier{.multiplier = kInfinity}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::OpacityMultiplier{.multiplier = kNan}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
}

TEST(ColorFunctionTest, ValidateHueOffset) {
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::HueOffset{.offset = Angle::Degrees(0)}}),
              IsOk());
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::HueOffset{.offset = Angle::Degrees(-60)}}),
              IsOk());

  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::HueOffset{.offset = Angle::Radians(kInfinity)}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::HueOffset{.offset = Angle::Radians(kNan)}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
}

TEST(ColorFunctionTest, ValidateSaturationMultiplier) {
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::SaturationMultiplier{.multiplier = 0}}),
              IsOk());
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::SaturationMultiplier{.multiplier = 2.5}}),
              IsOk());

  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::SaturationMultiplier{.multiplier = -1}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("non-negative")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::SaturationMultiplier{.multiplier = kInfinity}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::SaturationMultiplier{.multiplier = kNan}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
}

TEST(ColorFunctionTest, ValidateLuminosityOffset) {
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::LuminosityOffset{.offset = 0}}),
              IsOk());
  EXPECT_THAT(brush_internal::ValidateColorFunction(
                  {ColorFunction::LuminosityOffset{.offset = -0.5}}),
              IsOk());

  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::LuminosityOffset{.offset = kInfinity}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
  EXPECT_THAT(
      brush_internal::ValidateColorFunction(
          {ColorFunction::LuminosityOffset{.offset = kNan}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("finite")));
}

TEST(ColorFunctionTest, ApplyOpacityMultiplier) {
  EXPECT_EQ((ColorFunction{ColorFunction::OpacityMultiplier{1}})(Color::Red()),
            Color::Red());
  EXPECT_EQ(
      (ColorFunction{ColorFunction::OpacityMultiplier{0.25}})(Color::Red()),
      Color::Red().WithAlphaFloat(0.25));
  EXPECT_EQ((ColorFunction{ColorFunction::OpacityMultiplier{0.5}})(
                Color::FromFloat(1, 1, 1, 0.75)),
            Color::FromFloat(1, 1, 1, 0.375));
}

TEST(ColorFunctionTest, ApplyHueOffset) {
  EXPECT_THAT(
      (ColorFunction{ColorFunction::HueOffset{Angle::Degrees(0)}})(Color::Red())
          .ClampedToGamut()
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({1, 0, 0, 1}, 1e-2));
  EXPECT_THAT((ColorFunction{ColorFunction::HueOffset{Angle::Degrees(-90)}})(
                  Color::Red())
                  .ClampedToGamut()
                  .AsFloat(Color::Format::kGammaEncoded),
              ChannelStructNear({0.71, 0, 1, 1}, 1e-2));
  EXPECT_THAT((ColorFunction{ColorFunction::HueOffset{Angle::Degrees(180)}})(
                  Color::Red())
                  .ClampedToGamut()
                  .AsFloat(Color::Format::kGammaEncoded),
              ChannelStructNear({0, 0.8, 0.8, 1}, 1e-2));
}

TEST(ColorFunctionTest, ApplySaturationMultiplier) {
  EXPECT_THAT(
      (ColorFunction{ColorFunction::SaturationMultiplier{1}})(Color::Red())
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({1, 0, 0, 1}, 1e-2));
  EXPECT_THAT(
      (ColorFunction{ColorFunction::SaturationMultiplier{0.5}})(Color::Red())
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({0.83, 0.42, 0.42, 1}, 1e-2));
  EXPECT_THAT(
      (ColorFunction{ColorFunction::SaturationMultiplier{0}})(Color::Red())
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({0.58, 0.58, 0.58, 1}, 1e-2));
}

TEST(ColorFunctionTest, ApplyLuminosityOffset) {
  EXPECT_THAT((ColorFunction{ColorFunction::LuminosityOffset{0}})(Color::Red())
                  .ClampedToGamut()
                  .AsFloat(Color::Format::kGammaEncoded),
              ChannelStructNear({1, 0, 0, 1}, 1e-2));
  EXPECT_THAT(
      (ColorFunction{ColorFunction::LuminosityOffset{0.5}})(Color::Red())
          .ClampedToGamut()
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({1, 0.74, 0.74, 1}, 1e-2));
  EXPECT_THAT(
      (ColorFunction{ColorFunction::LuminosityOffset{-0.5}})(Color::Red())
          .ClampedToGamut()
          .AsFloat(Color::Format::kGammaEncoded),
      ChannelStructNear({0.74, 0, 0, 1}, 1e-2));
}

TEST(ColorFunctionTest, ApplyReplaceColor) {
  EXPECT_EQ((ColorFunction{ColorFunction::ReplaceColor{Color::Green()}})(
                Color::Red()),
            Color::Green());
}

void DefaultConstructedColorFunctionIsIdentity(const Color& color) {
  ColorFunction color_function;
  EXPECT_EQ(color_function(color), color);
}
FUZZ_TEST(ColorFunctionTest, DefaultConstructedColorFunctionIsIdentity)
    .WithDomains(ArbitraryColor());

void ReplaceColorIgnoresInputColor(const Color& replace_color,
                                   const Color& input_color) {
  ColorFunction color_function{ColorFunction::ReplaceColor{replace_color}};
  EXPECT_EQ(color_function(input_color), replace_color);
}
FUZZ_TEST(ColorFunctionTest, ReplaceColorIgnoresInputColor)
    .WithDomains(ArbitraryColor(), ArbitraryColor());

void CanValidateValidColorFunction(const ColorFunction& color_function) {
  EXPECT_THAT(brush_internal::ValidateColorFunction(color_function), IsOk());
}
FUZZ_TEST(ColorFunctionTest, CanValidateValidColorFunction)
    .WithDomains(ValidColorFunction());

TEST(ColorFunctionTest, DefaultConstructedColorFunctionIsIdentity) {
  EXPECT_EQ(ColorFunction{},
            ColorFunction{ColorFunction::OpacityMultiplier{1}});
}

}  // namespace
}  // namespace ink
