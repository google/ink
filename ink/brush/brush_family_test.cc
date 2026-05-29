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

#include "ink/brush/brush_family.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/brush/type_matchers.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Pointwise;
using ::testing::SizeIs;

static_assert(std::numeric_limits<float>::has_quiet_NaN);
constexpr float kNan = std::numeric_limits<float>::quiet_NaN();
constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr absl::StatusCode kInvalidArgument =
    absl::StatusCode::kInvalidArgument;
constexpr absl::string_view kTestTextureId = "test-paint";

BrushTip CreatePressureTestTip() {
  return {
      .scale = {2, 1},
      .corner_rounding = 0.75,
      .rotation = kQuarterTurn,
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kNormalizedPressure,
              .source_out_of_range_behavior =
                  BrushBehavior::OutOfRange::kRepeat,
              .source_value_range = {0, 1},
          },
          BrushBehavior::ToolTypeFilterNode{
              .enabled_tool_types = {.touch = true, .stylus = true},
          },
          BrushBehavior::ResponseNode{
              .response_curve = {EasingFunction::Predefined::kEaseInOut},
          },
          BrushBehavior::DampingNode{
              .damping_source = BrushBehavior::ProgressDomain::kTimeInSeconds,
              .damping_gap = 0.1,
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kWidthMultiplier,
              .target_modifier_range = {0, 2},
          },
      }}},
  };
}

BrushPaint CreateTestPaint() {
  return {.texture_layers = {BrushPaint::StampingTexture{
              .client_texture_id = std::string(kTestTextureId),
              .blend_mode = BrushPaint::BlendMode::kDstIn}}};
}

BrushCoat CreateTestCoat() {
  return {
      .tip = CreatePressureTestTip(),
      .paint_preferences = {CreateTestPaint()},
  };
}

TEST(BrushFamilyTest, Equality) {
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create({});
  ASSERT_THAT(family1, IsOk());
  absl::StatusOr<BrushFamily> family2 = BrushFamily::Create({});
  ASSERT_THAT(family2, IsOk());
  EXPECT_EQ(*family1, *family2);

  family1 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.5f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(1)},
      {.client_brush_family_id = "family1"});
  ASSERT_THAT(family1, IsOk());
  EXPECT_NE(*family1, *family2);
  family2 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.5f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(1)},
      {.client_brush_family_id = "family1"});
  ASSERT_THAT(family2, IsOk());
  EXPECT_EQ(*family1, *family2);

  // Different coats should make families unequal.
  family2 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.6f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(1)},
      {.client_brush_family_id = "family1"});
  ASSERT_THAT(family2, IsOk());
  EXPECT_NE(*family1, *family2);

  // Different input model should make families unequal.
  family2 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.5f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(2)},
      {.client_brush_family_id = "family1"});
  ASSERT_THAT(family2, IsOk());
  EXPECT_NE(*family1, *family2);

  // Different metadata should make families unequal.
  family2 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.5f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(1)},
      {.client_brush_family_id = "family2"});
  ASSERT_THAT(family2, IsOk());
  EXPECT_NE(*family1, *family2);
}

TEST(BrushFamilyTest, AbslHash) {
  absl::StatusOr<BrushFamily> family_default = BrushFamily::Create({});
  ASSERT_THAT(family_default, IsOk());
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.1f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(1)},
      {.client_brush_family_id = "family1"});
  ASSERT_THAT(family1, IsOk());
  absl::StatusOr<BrushFamily> family2 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.2f}}},
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Millis(2)},
      {.client_brush_family_id = "family2"});
  ASSERT_THAT(family2, IsOk());
  absl::StatusOr<BrushFamily> family3 = BrushFamily::Create(
      {BrushCoat{.tip = {.pinch = 0.1f}}, BrushCoat{.tip = {.pinch = 0.2f}}},
      BrushFamily::PassthroughModel{}, {.developer_comment = "comment"});
  ASSERT_THAT(family3, IsOk());

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(
      {*family_default, *family1, *family2, *family3}));
}

TEST(BrushFamilyTest, StringifyInputModel) {
  EXPECT_EQ(
      absl::StrCat(BrushFamily::InputModel{BrushFamily::PassthroughModel{}}),
      "PassthroughModel");
  EXPECT_EQ(
      absl::StrCat(BrushFamily::InputModel{BrushFamily::SlidingWindowModel{
          .window_size = Duration32::Millis(125),
          .upsampling_period = Duration32::Infinite()}}),
      "SlidingWindowModel(window_size=125ms, upsampling_period=inf)");
}

TEST(BrushFamilyTest, StringifyWithNoId) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.scale = {3, 3},
               .corner_rounding = 0,
               .particle_gap_distance_scale = 0.1,
               .particle_gap_duration = Duration32::Seconds(2)},
      CreateTestPaint(), BrushFamily::PassthroughModel{});
  ASSERT_THAT(family, IsOk());
  EXPECT_EQ(
      absl::StrCat(*family),
      "BrushFamily(coats=[BrushCoat{tip=BrushTip{scale=<3, 3>, "
      "corner_rounding=0, particle_gap_distance_scale=0.1, "
      "particle_gap_duration=2s}, "
      "paint_preferences={BrushPaint{texture_layers={StampingTexture{"
      "client_texture_id=test-paint, animation_frames=1, animation_rows=1, "
      "animation_columns=1, animation_duration=1s, blend_mode=kDstIn}}, "
      "self_overlap=kAny}}}], input_model=PassthroughModel)");
}

TEST(BrushFamilyTest, StringifyWithId) {
  absl::StatusOr<BrushFamily> family =
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          CreateTestPaint(), BrushFamily::PassthroughModel{},
                          {.client_brush_family_id = "big-square"});
  ASSERT_THAT(family, IsOk());
  EXPECT_EQ(
      absl::StrCat(*family),
      "BrushFamily(coats=[BrushCoat{"
      "tip=BrushTip{scale=<3, 3>, corner_rounding=0}, "
      "paint_preferences={BrushPaint{texture_layers={StampingTexture{client_"
      "texture_id=test-paint, animation_frames=1, animation_rows=1, "
      "animation_columns=1, animation_duration=1s, blend_mode=kDstIn}}, "
      "self_overlap=kAny}}}], input_model=PassthroughModel, "
      "client_brush_family_id='big-square')");
}

TEST(BrushFamilyTest, CreateWithoutId) {
  BrushCoat coat = CreateTestCoat();

  absl::StatusOr<BrushFamily> family = BrushFamily::Create({coat});

  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetCoats(), ElementsAre(BrushCoatEq(coat)));
  EXPECT_THAT(family->GetMetadata().client_brush_family_id, "");
}

TEST(BrushFamilyTest, CreateWithId) {
  BrushCoat coat = CreateTestCoat();
  absl::StatusOr<BrushFamily> family =
      BrushFamily::Create({coat}, BrushFamily::DefaultInputModel(),
                          {.client_brush_family_id = "test-family"});

  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetCoats(), ElementsAre(BrushCoatEq(coat)));
  EXPECT_EQ(family->GetMetadata().client_brush_family_id, "test-family");
}

TEST(BrushFamilyTest, CreateWithNoCoats) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create({});
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetCoats(), IsEmpty());
}

TEST(BrushFamilyTest, CreateWithMultipleCoats) {
  absl::StatusOr<BrushFamily> family =
      BrushFamily::Create({CreateTestCoat(), CreateTestCoat()});
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetCoats(), SizeIs(2));
}

TEST(BrushFamilyTest, CreateWithTooManyCoats) {
  std::vector<BrushCoat> coats(BrushFamily::MaxBrushCoats() + 1,
                               CreateTestCoat());
  EXPECT_THAT(BrushFamily::Create(coats),
              StatusIs(kInvalidArgument, HasSubstr("at most")));
}

TEST(BrushFamilyTest, CreateWithInvalidInputModel) {
  std::vector<BrushCoat> coats = {CreateTestCoat()};
  BrushFamily::InputModel input_model = {
      BrushFamily::SlidingWindowModel{.window_size = Duration32::Zero()}};
  EXPECT_THAT(BrushFamily::Create(coats, input_model),
              StatusIs(kInvalidArgument, HasSubstr("window_size")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipScale) {
  EXPECT_THAT(BrushFamily::Create({.scale = {kInfinity, 1}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));
  EXPECT_THAT(BrushFamily::Create({.scale = {1, kInfinity}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));

  EXPECT_THAT(BrushFamily::Create({.scale = {kNan, 1}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));
  EXPECT_THAT(BrushFamily::Create({.scale = {1, kNan}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));

  EXPECT_THAT(BrushFamily::Create({.scale = {-1, 1}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));
  EXPECT_THAT(BrushFamily::Create({.scale = {1, -1}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));

  EXPECT_THAT(BrushFamily::Create({.scale = {0, 0}}, {}),
              StatusIs(kInvalidArgument, HasSubstr("scale")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipCornerRounding) {
  EXPECT_THAT(BrushFamily::Create({.corner_rounding = -kInfinity}, {}),
              StatusIs(kInvalidArgument, HasSubstr("corner_rounding")));
  EXPECT_THAT(BrushFamily::Create({.corner_rounding = kInfinity}, {}),
              StatusIs(kInvalidArgument, HasSubstr("corner_rounding")));
  EXPECT_THAT(BrushFamily::Create({.corner_rounding = kNan}, {}),
              StatusIs(kInvalidArgument, HasSubstr("corner_rounding")));
  EXPECT_THAT(BrushFamily::Create({.corner_rounding = -1}, {}),
              StatusIs(kInvalidArgument, HasSubstr("corner_rounding")));
  EXPECT_THAT(BrushFamily::Create({.corner_rounding = 2}, {}),
              StatusIs(kInvalidArgument, HasSubstr("corner_rounding")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipPinch) {
  EXPECT_THAT(BrushFamily::Create({.pinch = -kInfinity}, {}),
              StatusIs(kInvalidArgument, HasSubstr("pinch")));
  EXPECT_THAT(BrushFamily::Create({.pinch = kInfinity}, {}),
              StatusIs(kInvalidArgument, HasSubstr("pinch")));
  EXPECT_THAT(BrushFamily::Create({.pinch = kNan}, {}),
              StatusIs(kInvalidArgument, HasSubstr("pinch")));
  EXPECT_THAT(BrushFamily::Create({.pinch = -1}, {}),
              StatusIs(kInvalidArgument, HasSubstr("pinch")));
  EXPECT_THAT(BrushFamily::Create({.pinch = 2}, {}),
              StatusIs(kInvalidArgument, HasSubstr("pinch")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipRotation) {
  EXPECT_THAT(BrushFamily::Create({.rotation = Angle::Radians(kInfinity)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("rotation")));
  EXPECT_THAT(BrushFamily::Create({.rotation = -Angle::Radians(kInfinity)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("rotation")));
  EXPECT_THAT(BrushFamily::Create({.rotation = -Angle::Radians(kNan)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("rotation")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipSlant) {
  EXPECT_THAT(BrushFamily::Create({.slant = Angle::Radians(kInfinity)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("slant")));
  EXPECT_THAT(BrushFamily::Create({.slant = -Angle::Radians(kInfinity)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("slant")));
  EXPECT_THAT(BrushFamily::Create({.slant = -Angle::Radians(kNan)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("slant")));
  EXPECT_THAT(BrushFamily::Create({.slant = -kHalfTurn}, {}),
              StatusIs(kInvalidArgument, HasSubstr("slant")));
  EXPECT_THAT(BrushFamily::Create({.slant = kHalfTurn}, {}),
              StatusIs(kInvalidArgument, HasSubstr("slant")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipParticleGapDistanceScale) {
  EXPECT_THAT(
      BrushFamily::Create({.particle_gap_distance_scale = kInfinity}, {}),
      StatusIs(kInvalidArgument, HasSubstr("particle_gap_distance_scale")));
  EXPECT_THAT(BrushFamily::Create({.particle_gap_distance_scale = kNan}, {}),
              StatusIs(kInvalidArgument, HasSubstr("particle_gap_distance")));
  EXPECT_THAT(BrushFamily::Create({.particle_gap_distance_scale = -1.f}, {}),
              StatusIs(kInvalidArgument, HasSubstr("particle_gap_distance")));
}

TEST(BrushFamilyTest, CreateWithInvalidTipParticleGapDuration) {
  EXPECT_THAT(BrushFamily::Create(
                  {.particle_gap_duration = Duration32::Infinite()}, {}),
              StatusIs(kInvalidArgument, HasSubstr("particle_gap_duration")));
  EXPECT_THAT(BrushFamily::Create(
                  {.particle_gap_duration = -Duration32::Seconds(1)}, {}),
              StatusIs(kInvalidArgument, HasSubstr("particle_gap_duration")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorSource) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = static_cast<BrushBehavior::Source>(-1),
              .source_value_range = {0, 1},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kSizeMultiplier,
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorTarget) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kNormalizedPressure,
              .source_value_range = {0, 1},
          },
          BrushBehavior::TargetNode{
              .target = static_cast<BrushBehavior::Target>(-1),
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorOutOfRange) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kNormalizedPressure,
              .source_out_of_range_behavior =
                  static_cast<BrushBehavior::OutOfRange>(-1),
              .source_value_range = {0, 1},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kSizeMultiplier,
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  EXPECT_THAT(
      BrushFamily::Create(brush_tip, BrushPaint{}),
      StatusIs(kInvalidArgument, HasSubstr("source_out_of_range_behavior")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorSourceValueRange) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  BrushBehavior::SourceNode* source_node =
      &std::get<BrushBehavior::SourceNode>(brush_tip.behaviors[0].nodes[0]);

  source_node->source_value_range = {kInfinity, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));

  source_node->source_value_range = {0, kInfinity};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));

  source_node->source_value_range = {kNan, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));

  source_node->source_value_range = {0, kNan};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));

  source_node->source_value_range = {0, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));

  source_node->source_value_range = {5, 5};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("source_value_range")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorSourceAndOutOfRangeBehavior) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kTimeSinceInputInSeconds,
              .source_out_of_range_behavior =
                  BrushBehavior::OutOfRange::kRepeat,
              .source_value_range = {0, 1},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, 1},
          },
      }}},
  };
  EXPECT_THAT(
      BrushFamily::Create(brush_tip, BrushPaint{}),
      StatusIs(kInvalidArgument,
               HasSubstr("`kTimeSince*` sources can only be used with a "
                         "`source_out_of_range_behavior` of `kClamp`")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorTargetModifierRange) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
          },
      }}},
  };
  BrushBehavior::TargetNode* target_node =
      &std::get<BrushBehavior::TargetNode>(brush_tip.behaviors[0].nodes[1]);

  target_node->target_modifier_range = {kInfinity, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));

  target_node->target_modifier_range = {0, kInfinity};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));

  target_node->target_modifier_range = {kNan, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));

  target_node->target_modifier_range = {0, kNan};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));

  target_node->target_modifier_range = {0, 0};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));

  target_node->target_modifier_range = {5, 5};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("target_modifier_range")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorPredefinedResponseCurve) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::ResponseNode{
              .response_curve = {static_cast<EasingFunction::Predefined>(-1)},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, .2},
          },
      }}},
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("Predefined")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorCubicBezierResponseCurve) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::ResponseNode{},
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, .2},
          },
      }}},
  };
  BrushBehavior::ResponseNode* response_node =
      &std::get<BrushBehavior::ResponseNode>(brush_tip.behaviors[0].nodes[1]);

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = -1, .y1 = 0, .x2 = 1, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = 0, .x2 = 2, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = kInfinity, .y1 = 0, .x2 = 1, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = kInfinity, .x2 = 1, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = 0, .x2 = kInfinity, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = 0, .x2 = 1, .y2 = kInfinity};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = kNan, .y1 = 0, .x2 = 1, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = kNan, .x2 = 1, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = 0, .x2 = kNan, .y2 = 1};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));

  response_node->response_curve.parameters =
      EasingFunction::CubicBezier{.x1 = 0, .y1 = 0, .x2 = 1, .y2 = kNan};
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("CubicBezier")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorLinearResponseCurve) {
  auto create_family_with_linear = [](const std::vector<Point>& points) {
    return BrushFamily::Create(
        BrushTip{
            .behaviors = {BrushBehavior{{
                BrushBehavior::SourceNode{
                    .source = BrushBehavior::Source::kOrientationInRadians,
                    .source_value_range = {0, 3},
                },
                BrushBehavior::ResponseNode{
                    .response_curve = {EasingFunction::Linear{.points =
                                                                  points}},
                },
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kPinchOffset,
                    .target_modifier_range = {0, .2},
                },
            }}},
        },
        BrushPaint{});
  };
  // Non-finite Y-position:
  EXPECT_THAT(create_family_with_linear({{0, kNan}}),
              StatusIs(kInvalidArgument, HasSubstr("y-position")));
  EXPECT_THAT(create_family_with_linear({{0, kInfinity}}),
              StatusIs(kInvalidArgument, HasSubstr("y-position")));
  EXPECT_THAT(create_family_with_linear({{0, -kInfinity}}),
              StatusIs(kInvalidArgument, HasSubstr("y-position")));
  // Non-finite X-position:
  EXPECT_THAT(create_family_with_linear({{kNan, 0}}),
              StatusIs(kInvalidArgument, HasSubstr("x-position")));
  EXPECT_THAT(create_family_with_linear({{kInfinity, 0}}),
              StatusIs(kInvalidArgument, HasSubstr("x-position")));
  EXPECT_THAT(create_family_with_linear({{-kInfinity, 0}}),
              StatusIs(kInvalidArgument, HasSubstr("x-position")));
  // X-position out of range:
  EXPECT_THAT(create_family_with_linear({{-0.1, 0}}),
              StatusIs(kInvalidArgument, HasSubstr("x-position")));
  EXPECT_THAT(create_family_with_linear({{1.1, 0}}),
              StatusIs(kInvalidArgument, HasSubstr("x-position")));
  // X-positions that aren't monotonicly non-decreasing:
  EXPECT_THAT(create_family_with_linear({{0.75, 0}, {0.25, 1}}),
              StatusIs(kInvalidArgument, HasSubstr("monotonic")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorStepsResponseCurve) {
  BrushTip brush_tip = {
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::ResponseNode{},
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, .2},
          },
      }}},
  };
  BrushBehavior::ResponseNode* response_node =
      &std::get<BrushBehavior::ResponseNode>(brush_tip.behaviors[0].nodes[1]);

  response_node->response_curve.parameters = EasingFunction::Steps{
      .step_count = 0,
      .step_position = EasingFunction::StepPosition::kJumpEnd,
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("Steps")));

  response_node->response_curve.parameters = EasingFunction::Steps{
      .step_count = 1,
      .step_position = static_cast<EasingFunction::StepPosition>(-1),
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("Steps")));

  response_node->response_curve.parameters = EasingFunction::Steps{
      .step_count = 1,
      .step_position = EasingFunction::StepPosition::kJumpNone,
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("Steps")));
}

TEST(BrushFamilyTest, CreateWithInvalidBehaviorDampingGap) {
  BrushTip brush_tip = BrushTip{
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::DampingNode{
              .damping_source = BrushBehavior::ProgressDomain::kTimeInSeconds,
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, .2},
          },
      }}},
  };
  BrushBehavior::DampingNode* damping_node =
      &std::get<BrushBehavior::DampingNode>(brush_tip.behaviors[0].nodes[1]);

  damping_node->damping_gap = -0.1;
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("damping_gap")));

  damping_node->damping_gap = kInfinity;
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("damping_gap")));
}

TEST(BrushFamilyTest, CreateWithInvalidEnabledToolTypes) {
  BrushTip brush_tip = BrushTip{
      .behaviors = {BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kOrientationInRadians,
              .source_value_range = {0, 3},
          },
          BrushBehavior::ToolTypeFilterNode{
              .enabled_tool_types = {.unknown = false,
                                     .mouse = false,
                                     .touch = false,
                                     .stylus = false},
          },
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kPinchOffset,
              .target_modifier_range = {0, .2},
          },
      }}},
  };
  EXPECT_THAT(BrushFamily::Create(brush_tip, BrushPaint{}),
              StatusIs(kInvalidArgument, HasSubstr("enabled_tool_types")));
}

TEST(BrushFamilyTest, DefaultConstruction) {
  BrushFamily family;
  EXPECT_THAT(family.GetCoats(),
              ElementsAre(BrushCoatEq(BrushCoat{.tip = BrushTip{}})));
  EXPECT_THAT(family.GetInputModel(),
              BrushFamilyInputModelEq(BrushFamily::DefaultInputModel()));
  EXPECT_THAT(family.GetMetadata().client_brush_family_id, IsEmpty());
  EXPECT_EQ(family.GetTextureAnimationLoopDuration(), absl::ZeroDuration());
}

TEST(BrushFamilyTest, AnimatedTextureLoopDuration) {
  auto make_animated_coat = [](absl::Duration animation_duration) {
    return BrushCoat{CreatePressureTestTip(),
                     {BrushPaint{{BrushPaint::StampingTexture{
                         .client_texture_id = std::string(kTestTextureId),
                         .animation_frames = 2,
                         .animation_rows = 2,
                         .animation_duration = animation_duration,
                     }}}}};
  };

  // These three prime numbers multiply to 428,868,313 (which is more than
  // 2^24).
  EXPECT_THAT(
      BrushFamily::Create({make_animated_coat(absl::Milliseconds(751)),
                           make_animated_coat(absl::Milliseconds(1531)),
                           make_animated_coat(absl::Milliseconds(373))}),
      StatusIs(kInvalidArgument,
               HasSubstr("The LCM of all texture animation durations")));

  // These three numbers also multiply to more than 2^24, but their LCM is only
  // 3000.
  absl::StatusOr<BrushFamily> family =
      BrushFamily::Create({make_animated_coat(absl::Milliseconds(1000)),
                           make_animated_coat(absl::Milliseconds(600)),
                           make_animated_coat(absl::Milliseconds(1500))});
  ASSERT_THAT(family, IsOk());
  EXPECT_EQ(family->GetTextureAnimationLoopDuration(),
            absl::Milliseconds(3000));
}

TEST(BrushFamilyTest, NonAnimatedTexturesDoNotCountTowardLoopDuration) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create({
      BrushCoat{CreatePressureTestTip(),
                // Tiling textures aren't animated.
                {BrushPaint{{BrushPaint::TilingTexture{
                    .client_texture_id = std::string(kTestTextureId),
                    .size = {1, 1},
                }}}}},
      BrushCoat{CreatePressureTestTip(),
                {BrushPaint{{BrushPaint::StampingTexture{
                    .client_texture_id = std::string(kTestTextureId),
                    .animation_frames = 10,
                    .animation_rows = 10,
                    // A duration of zero means that although this texture can
                    // be affected by animation progress offset behaviors, it
                    // isn't animated when the stroke is dry.
                    .animation_duration = absl::ZeroDuration(),
                }}}}},
      BrushCoat{CreatePressureTestTip(),
                {BrushPaint{{BrushPaint::StampingTexture{
                    .client_texture_id = std::string(kTestTextureId),
                    // If the number of animation frames is 1, then by
                    // definition the texture isn't animated, regardless of what
                    // `animation_duration` says.
                    .animation_frames = 1,
                    .animation_rows = 10,
                    .animation_duration = absl::Milliseconds(7),
                }}}}},
      BrushCoat{CreatePressureTestTip(),
                {BrushPaint{{BrushPaint::StampingTexture{
                    .client_texture_id = std::string(kTestTextureId),
                    .animation_frames = 10,
                    .animation_rows = 10,
                    .animation_duration = absl::Milliseconds(13),
                }}}}},
  });
  ASSERT_THAT(family, IsOk());
  // The non-animated textures don't count, so the LCM is 13 milliseconds.
  EXPECT_EQ(family->GetTextureAnimationLoopDuration(), absl::Milliseconds(13));
}

TEST(BrushFamilyTest, CopyAndMove) {
  {
    absl::StatusOr<BrushFamily> family = BrushFamily::Create(
        CreatePressureTestTip(), CreateTestPaint(),
        BrushFamily::DefaultInputModel(),
        {.client_brush_family_id = "/brush-family:test-family"});
    ASSERT_THAT(family, IsOk());

    BrushFamily copied_family = *family;
    EXPECT_THAT(copied_family.GetCoats(),
                Pointwise(BrushCoatEq(), family->GetCoats()));
    EXPECT_EQ(copied_family.GetMetadata().client_brush_family_id,
              family->GetMetadata().client_brush_family_id);
  }
  {
    absl::StatusOr<BrushFamily> family = BrushFamily::Create(
        CreatePressureTestTip(), CreateTestPaint(),
        BrushFamily::DefaultInputModel(),
        {.client_brush_family_id = "/brush-family:test-family"});
    ASSERT_THAT(family, IsOk());

    BrushFamily copied_family;
    copied_family = *family;
    EXPECT_THAT(copied_family.GetCoats(),
                Pointwise(BrushCoatEq(), family->GetCoats()));
    EXPECT_EQ(copied_family.GetMetadata().client_brush_family_id,
              family->GetMetadata().client_brush_family_id);
  }
  {
    absl::StatusOr<BrushFamily> family = BrushFamily::Create(
        CreatePressureTestTip(), CreateTestPaint(),
        BrushFamily::DefaultInputModel(),
        {.client_brush_family_id = "/brush-family:test-family"});
    ASSERT_THAT(family, IsOk());

    BrushFamily copied_family = *family;
    BrushFamily moved_family = *std::move(family);
    EXPECT_THAT(moved_family.GetCoats(),
                Pointwise(BrushCoatEq(), copied_family.GetCoats()));
    EXPECT_EQ(moved_family.GetMetadata().client_brush_family_id,
              copied_family.GetMetadata().client_brush_family_id);
  }
  {
    absl::StatusOr<BrushFamily> family = BrushFamily::Create(
        CreatePressureTestTip(), CreateTestPaint(),
        BrushFamily::DefaultInputModel(),
        {.client_brush_family_id = "/brush-family:test-family"});
    ASSERT_THAT(family, IsOk());

    BrushFamily copied_family = *family;
    BrushFamily moved_family;
    moved_family = *std::move(family);
    EXPECT_THAT(moved_family.GetCoats(),
                Pointwise(BrushCoatEq(), copied_family.GetCoats()));
    EXPECT_EQ(moved_family.GetMetadata().client_brush_family_id,
              copied_family.GetMetadata().client_brush_family_id);
  }
}

TEST(BrushFamilyTest, CreateWithInvalidBrushPaint) {
  // `TextureLayer::origin` has invalid enum value
  EXPECT_THAT(BrushFamily::Create(
                  BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                  {.texture_layers = {BrushPaint::TilingTexture{
                       .client_texture_id = std::string(kTestTextureId),
                       .origin = static_cast<BrushPaint::TextureOrigin>(-1),
                       .size = {1, 4}}}}),
              StatusIs(kInvalidArgument,
                       HasSubstr("BrushPaint::TilingTexture::origin")));
  // TextureLayer::size_unit has invalid enum value
  EXPECT_THAT(
      BrushFamily::Create(
          BrushTip{.scale = {3, 3}, .corner_rounding = 0},
          {.texture_layers = {BrushPaint::TilingTexture{
               .client_texture_id = std::string(kTestTextureId),
               .size_unit = static_cast<BrushPaint::TextureSizeUnit>(-1),
               .size = {1, 4}}}}),
      StatusIs(kInvalidArgument,
               HasSubstr("BrushPaint::TilingTexture::size_unit")));
  // `TextureLayer::size` has negative component.
  EXPECT_THAT(
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          {.texture_layers = {BrushPaint::TilingTexture{
                               .client_texture_id = std::string(kTestTextureId),
                               .size = {-1, 4}}}}),
      StatusIs(kInvalidArgument, HasSubstr("BrushPaint::TilingTexture::size")));
  // `TextureLayer::size` has zero-size component.
  EXPECT_THAT(
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          {.texture_layers = {BrushPaint::TilingTexture{
                               .client_texture_id = std::string(kTestTextureId),
                               .size = {0, 4}}}}),
      StatusIs(kInvalidArgument, HasSubstr("BrushPaint::TilingTexture::size")));
  // `TextureLayer::size` is non-finite.
  EXPECT_THAT(
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          {.texture_layers = {BrushPaint::TilingTexture{
                               .client_texture_id = std::string(kTestTextureId),
                               .size = {3, kInfinity}}}}),
      StatusIs(kInvalidArgument, HasSubstr("BrushPaint::TilingTexture::size")));
  // `TextureLayer::offset` is infinite.
  EXPECT_THAT(
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          {.texture_layers = {BrushPaint::TilingTexture{
                               .client_texture_id = std::string(kTestTextureId),
                               .size = {1, 3},
                               .offset = {kInfinity, 0.4}}}}),
      StatusIs(kInvalidArgument,
               HasSubstr("BrushPaint::TilingTexture::offset")));
  // `TextureLayer::offset` is NaN.
  EXPECT_THAT(
      BrushFamily::Create(BrushTip{.scale = {3, 3}, .corner_rounding = 0},
                          {.texture_layers = {BrushPaint::TilingTexture{
                               .client_texture_id = std::string(kTestTextureId),
                               .size = {1, 3},
                               .offset = {1, kNan}}}}),
      StatusIs(kInvalidArgument,
               HasSubstr("BrushPaint::TilingTexture::offset")));
}

void CanCreateAnyValidBrushFamily(absl::Span<const BrushCoat> coats,
                                  const BrushFamily::InputModel& input_model) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(coats, input_model);
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetCoats(), Pointwise(BrushCoatEq(), coats));
  EXPECT_THAT(family->GetInputModel(), BrushFamilyInputModelEq(input_model));
}
FUZZ_TEST(BrushFamilyTest, CanCreateAnyValidBrushFamily)
    .WithDomains(fuzztest::VectorOf(ValidBrushCoat())
                     .WithMaxSize(BrushFamily::MaxBrushCoats()),
                 ValidBrushFamilyInputModel());

}  // namespace
}  // namespace ink
