// Copyright 2024 Google LLC
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

#include "ink/brush/brush_coat.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/geometry/mesh_format.h"
#include "ink/types/uri.h"

namespace ink {
namespace {

using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

TEST(BrushCoatTest, Stringify) {
  EXPECT_EQ(absl::StrCat(BrushCoat{}),
            "BrushCoat{tips=[], paint=BrushPaint{texture_layers={}}}");
  EXPECT_EQ(absl::StrCat(BrushCoat{.tips = {BrushTip{}}}),
            "BrushCoat{tips=[BrushTip{scale=<1, 1>, corner_rounding=1}], "
            "paint=BrushPaint{texture_layers={}}}");
  EXPECT_EQ(absl::StrCat(BrushCoat{.tips = {BrushTip{.corner_rounding = 0},
                                            BrushTip{.corner_rounding = 1}}}),
            "BrushCoat{tips=[BrushTip{scale=<1, 1>, corner_rounding=0}, "
            "BrushTip{scale=<1, 1>, corner_rounding=1}], "
            "paint=BrushPaint{texture_layers={}}}");
}

TEST(BrushCoatTest, CoatWithDefaultTipAndPaintIsValid) {
  absl::Status status = brush_internal::ValidateBrushCoat(
      BrushCoat{.tips = {BrushTip{}}, .paint = BrushPaint{}});
  EXPECT_EQ(status, absl::OkStatus());
}

TEST(BrushCoatTest, CoatWithInvalidTipIsInvalid) {
  absl::Status status = brush_internal::ValidateBrushCoat(
      BrushCoat{.tips = {BrushTip{.pinch = -1}}, .paint = BrushPaint{}});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("pinch"));
}

TEST(BrushCoatTest, CoatWithInvalidPaintIsInvalid) {
  absl::Status status = brush_internal::ValidateBrushCoat(BrushCoat{
      .tips = {BrushTip{}},
      .paint = BrushPaint{.texture_layers = {{.color_texture_uri = Uri()}}}});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("color_texture_uri"));
}

TEST(BrushCoatTest, CoatWithNoTipsIsInvalid) {
  absl::Status status = brush_internal::ValidateBrushCoat(BrushCoat{});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("must have at least one BrushTip"));
}

TEST(BrushCoatTest, CoatWithTooManyTipsIsInvalid) {
  BrushCoat coat;
  // TODO: b/285594469 - For now, two tips is all it takes to be too many. Once
  // multi-tip `BrushCoat` support is implemented, we'll still want some kind of
  // cap on the number of tips.
  for (int i = 0; i < 2; ++i) {
    coat.tips.push_back(BrushTip{});
  }
  absl::Status status = brush_internal::ValidateBrushCoat(coat);
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
}

void CanValidateValidBrushCoat(const BrushCoat& coat) {
  EXPECT_EQ(absl::OkStatus(), brush_internal::ValidateBrushCoat(coat));
}
FUZZ_TEST(EasingFunctionTest, CanValidateValidBrushCoat)
    .WithDomains(ValidBrushCoat());

TEST(BrushCoatTest, GetRequiredAttributeIdsDefaultBrushCoat) {
  EXPECT_THAT(brush_internal::GetRequiredAttributeIds(BrushCoat()),
              UnorderedElementsAre(MeshFormat::AttributeId::kPosition,
                                   MeshFormat::AttributeId::kSideDerivative,
                                   MeshFormat::AttributeId::kSideLabel,
                                   MeshFormat::AttributeId::kForwardDerivative,
                                   MeshFormat::AttributeId::kForwardLabel,
                                   MeshFormat::AttributeId::kOpacityShift));
}

TEST(BrushCoatTest, GetRequiredAttributeIdsWithoutColorShift) {
  BrushTip tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kNormalizedPressure,
              .source_value_range = {0, 1},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  BrushCoat coat = {.tips = {tip}};
  EXPECT_THAT(brush_internal::GetRequiredAttributeIds(coat),
              Not(Contains(MeshFormat::AttributeId::kColorShiftHsl)));
}

TEST(BrushCoatTest, GetRequiredAttributeIdsWithColorShift) {
  constexpr BrushBehavior::Target kColorShiftTargets[] = {
      BrushBehavior::Target::kHueOffsetInRadians,
      BrushBehavior::Target::kSaturationMultiplier,
      BrushBehavior::Target::kLuminosity,
  };
  for (BrushBehavior::Target target : kColorShiftTargets) {
    BrushTip tip = {
        .behaviors = {BrushBehavior{{
            BrushBehavior::SourceNode{
                .source = BrushBehavior::Source::kNormalizedPressure,
                .source_value_range = {0, 1},
            },
            BrushBehavior::TargetNode{
                .target = target,
                .target_modifier_range = {0, 1},
            },
        }}},
    };
    BrushCoat coat = {.tips = {tip}};
    EXPECT_THAT(brush_internal::GetRequiredAttributeIds(coat),
                Contains(MeshFormat::AttributeId::kColorShiftHsl));
  }
}

}  // namespace
}  // namespace ink
