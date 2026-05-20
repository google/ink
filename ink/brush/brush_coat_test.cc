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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/geometry/mesh_format.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

constexpr absl::string_view kTestTextureId = "test-paint";

TEST(BrushCoatTest, Equality) {
  BrushCoat coat1, coat2;
  EXPECT_EQ(coat1, coat2);

  coat1.tip.pinch = 0.5;
  EXPECT_NE(coat1, coat2);
  coat2.tip.pinch = 0.5;
  EXPECT_EQ(coat1, coat2);

  coat1.paint_preferences.push_back(BrushPaint{});
  EXPECT_NE(coat1, coat2);
  coat2.paint_preferences.push_back(BrushPaint{});
  EXPECT_EQ(coat1, coat2);

  coat1.paint_preferences[0] =
      BrushPaint{.self_overlap = BrushPaint::SelfOverlap::kDiscard};
  EXPECT_NE(coat1, coat2);
  coat2.paint_preferences[0] =
      BrushPaint{.self_overlap = BrushPaint::SelfOverlap::kDiscard};
  EXPECT_EQ(coat1, coat2);
}

TEST(BrushCoatTest, AbslHash) {
  BrushPaint paint_a = {.self_overlap = BrushPaint::SelfOverlap::kDiscard};
  BrushPaint paint_b = {};
  BrushTip tip_a = {.pinch = 0.1f};
  BrushTip tip_b = {.pinch = 0.2f};

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(
      {BrushCoat{}, BrushCoat{.tip = tip_a}, BrushCoat{.tip = tip_b},
       BrushCoat{.paint_preferences = {paint_a}},
       BrushCoat{.paint_preferences = {paint_b}},
       BrushCoat{.paint_preferences = {paint_a, paint_b}},
       BrushCoat{.tip = tip_a, .paint_preferences = {paint_a}},
       BrushCoat{.tip = tip_b, .paint_preferences = {paint_b}}}));
}

TEST(BrushCoatTest, Stringify) {
  EXPECT_EQ(absl::StrCat(BrushCoat{.tip = BrushTip{}}),
            "BrushCoat{tip=BrushTip{scale=<1, 1>, corner_rounding=1}, "
            "paint_preferences={BrushPaint{self_overlap=kAny}}}");
}

TEST(BrushCoatTest, CoatWithDefaultTipAndPaintIsValid) {
  EXPECT_THAT(brush_internal::ValidateBrushCoat(BrushCoat{
                  .tip = BrushTip{}, .paint_preferences = {BrushPaint{}}}),
              IsOk());
}

TEST(BrushCoatTest, CoatWithInvalidTipIsInvalid) {
  EXPECT_THAT(
      brush_internal::ValidateBrushCoat(BrushCoat{
          .tip = BrushTip{.pinch = -1}, .paint_preferences = {BrushPaint{}}}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("pinch")));
}

void CanValidateValidBrushCoat(const BrushCoat& coat) {
  EXPECT_THAT(brush_internal::ValidateBrushCoat(coat), IsOk());
}
FUZZ_TEST(EasingFunctionTest, CanValidateValidBrushCoat)
    .WithDomains(ValidBrushCoat());

TEST(BrushCoatTest, AddAttributeIdsRequiredByDefaultCoat) {
  absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
  brush_internal::AddAttributeIdsRequiredByCoat(BrushCoat(),
                                                required_attributes);
  EXPECT_THAT(required_attributes,
              UnorderedElementsAre(MeshFormat::AttributeId::kPosition,
                                   MeshFormat::AttributeId::kSideDerivative,
                                   MeshFormat::AttributeId::kSideLabel,
                                   MeshFormat::AttributeId::kForwardDerivative,
                                   MeshFormat::AttributeId::kForwardLabel,
                                   MeshFormat::AttributeId::kOpacityShift));
}

TEST(BrushCoatTest, AddAttributeIdsRequiredByCoatWithoutColorShift) {
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
  BrushCoat coat = {.tip = tip};
  absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
  brush_internal::AddAttributeIdsRequiredByCoat(coat, required_attributes);
  EXPECT_THAT(required_attributes,
              Not(Contains(MeshFormat::AttributeId::kColorShiftHsl)));
}

TEST(BrushCoatTest, AddAttributeIdsRequiredByCoatWithColorShift) {
  constexpr BrushBehavior::Target kColorShiftTargets[] = {
      BrushBehavior::Target::kHueOffsetInRadians,
      BrushBehavior::Target::kSaturationMultiplier,
      BrushBehavior::Target::kLuminosityOffset,
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
    BrushCoat coat = {.tip = tip};
    absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
    brush_internal::AddAttributeIdsRequiredByCoat(coat, required_attributes);
    EXPECT_THAT(required_attributes,
                Contains(MeshFormat::AttributeId::kColorShiftHsl));
  }
}

TEST(BrushCoatTest, AddAttributeIdsRequiredByCoatWithoutStampingTextures) {
  BrushPaint paint = {
      .texture_layers = {BrushPaint::TilingTexture{
          .client_texture_id = std::string(kTestTextureId),
      }},
  };
  BrushCoat coat = {.tip = BrushTip(), .paint_preferences = {paint}};
  absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
  brush_internal::AddAttributeIdsRequiredByCoat(coat, required_attributes);
  EXPECT_THAT(required_attributes,
              Not(Contains(MeshFormat::AttributeId::kSurfaceUv)));
}

TEST(BrushCoatTest, AddAttributeIdsRequiredByCoatWithStampingTextures) {
  BrushPaint paint = {
      .texture_layers = {BrushPaint::StampingTexture{
          .client_texture_id = std::string(kTestTextureId),
      }},
  };
  BrushCoat coat = {.tip = BrushTip(), .paint_preferences = {paint}};
  absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
  brush_internal::AddAttributeIdsRequiredByCoat(coat, required_attributes);
  EXPECT_THAT(required_attributes,
              Contains(MeshFormat::AttributeId::kSurfaceUv));
}

}  // namespace
}  // namespace ink
