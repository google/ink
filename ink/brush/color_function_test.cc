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
  EXPECT_EQ(absl::OkStatus(),
            brush_internal::ValidateColorFunction(color_function));
}
FUZZ_TEST(ColorFunctionTest, CanValidateValidColorFunction)
    .WithDomains(ValidColorFunction());

TEST(ColorFunctionTest, DefaultConstructedColorFunctionIsIdentity) {
  EXPECT_EQ(ColorFunction{},
            ColorFunction{ColorFunction::OpacityMultiplier{1}});
}

}  // namespace
}  // namespace ink
