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

#include "ink/brush/brush_behavior.h"

#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::testing::HasSubstr;

constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr float kNan = std::numeric_limits<float>::quiet_NaN();

TEST(BrushBehaviorTest, StringifySource) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kNormalizedPressure),
            "kNormalizedPressure");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTiltInRadians),
            "kTiltInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTiltXInRadians),
            "kTiltXInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTiltYInRadians),
            "kTiltYInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kOrientationInRadians),
            "kOrientationInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kOrientationAboutZeroInRadians),
            "kOrientationAboutZeroInRadians");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::kSpeedInMultiplesOfBrushSizePerSecond),
            "kSpeedInMultiplesOfBrushSizePerSecond");
  EXPECT_EQ(
      absl::StrCat(
          BrushBehavior::Source::kVelocityXInMultiplesOfBrushSizePerSecond),
      "kVelocityXInMultiplesOfBrushSizePerSecond");
  EXPECT_EQ(
      absl::StrCat(
          BrushBehavior::Source::kVelocityYInMultiplesOfBrushSizePerSecond),
      "kVelocityYInMultiplesOfBrushSizePerSecond");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kNormalizedDirectionX),
            "kNormalizedDirectionX");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kNormalizedDirectionY),
            "kNormalizedDirectionY");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::kDistanceTraveledInMultiplesOfBrushSize),
            "kDistanceTraveledInMultiplesOfBrushSize");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTimeOfInputInSeconds),
            "kTimeOfInputInSeconds");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTimeOfInputInMillis),
            "kTimeOfInputInMillis");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::
                             kPredictedDistanceTraveledInMultiplesOfBrushSize),
            "kPredictedDistanceTraveledInMultiplesOfBrushSize");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kPredictedTimeElapsedInSeconds),
            "kPredictedTimeElapsedInSeconds");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kPredictedTimeElapsedInMillis),
            "kPredictedTimeElapsedInMillis");
  EXPECT_EQ(
      absl::StrCat(
          BrushBehavior::Source::kDistanceRemainingInMultiplesOfBrushSize),
      "kDistanceRemainingInMultiplesOfBrushSize");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTimeSinceInputInSeconds),
            "kTimeSinceInputInSeconds");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::kTimeSinceInputInMillis),
            "kTimeSinceInputInMillis");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::
                       kAccelerationInMultiplesOfBrushSizePerSecondSquared),
      "kAccelerationInMultiplesOfBrushSizePerSecondSquared");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::
                       kAccelerationXInMultiplesOfBrushSizePerSecondSquared),
      "kAccelerationXInMultiplesOfBrushSizePerSecondSquared");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::
                       kAccelerationYInMultiplesOfBrushSizePerSecondSquared),
      "kAccelerationYInMultiplesOfBrushSizePerSecondSquared");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::
                    kAccelerationForwardInMultiplesOfBrushSizePerSecondSquared),
            "kAccelerationForwardInMultiplesOfBrushSizePerSecondSquared");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::
                    kAccelerationLateralInMultiplesOfBrushSizePerSecondSquared),
            "kAccelerationLateralInMultiplesOfBrushSizePerSecondSquared");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::kInputSpeedInCentimetersPerSecond),
      "kInputSpeedInCentimetersPerSecond");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::kInputVelocityXInCentimetersPerSecond),
            "kInputVelocityXInCentimetersPerSecond");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::Source::kInputVelocityYInCentimetersPerSecond),
            "kInputVelocityYInCentimetersPerSecond");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::kInputDistanceTraveledInCentimeters),
      "kInputDistanceTraveledInCentimeters");
  EXPECT_EQ(
      absl::StrCat(
          BrushBehavior::Source::kPredictedInputDistanceTraveledInCentimeters),
      "kPredictedInputDistanceTraveledInCentimeters");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::
                             kInputAccelerationInCentimetersPerSecondSquared),
            "kInputAccelerationInCentimetersPerSecondSquared");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::
                             kInputAccelerationXInCentimetersPerSecondSquared),
            "kInputAccelerationXInCentimetersPerSecondSquared");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Source::
                             kInputAccelerationYInCentimetersPerSecondSquared),
            "kInputAccelerationYInCentimetersPerSecondSquared");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::
                       kInputAccelerationForwardInCentimetersPerSecondSquared),
      "kInputAccelerationForwardInCentimetersPerSecondSquared");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::Source::
                       kInputAccelerationLateralInCentimetersPerSecondSquared),
      "kInputAccelerationLateralInCentimetersPerSecondSquared");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::Source>(123)),
            "Source(123)");
}

TEST(BrushBehaviorTest, StringifyTarget) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kWidthMultiplier),
            "kWidthMultiplier");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kHeightMultiplier),
            "kHeightMultiplier");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kSizeMultiplier),
            "kSizeMultiplier");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kSlantOffsetInRadians),
            "kSlantOffsetInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kPinchOffset), "kPinchOffset");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kRotationOffsetInRadians),
            "kRotationOffsetInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kCornerRoundingOffset),
            "kCornerRoundingOffset");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kHueOffsetInRadians),
            "kHueOffsetInRadians");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kSaturationMultiplier),
            "kSaturationMultiplier");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kLuminosity), "kLuminosity");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Target::kOpacityMultiplier),
            "kOpacityMultiplier");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::Target>(91)), "Target(91)");
}

TEST(BrushBehaviorTest, StringifyOutOfRange) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::OutOfRange::kClamp), "kClamp");
  EXPECT_EQ(absl::StrCat(BrushBehavior::OutOfRange::kRepeat), "kRepeat");
  EXPECT_EQ(absl::StrCat(BrushBehavior::OutOfRange::kMirror), "kMirror");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::OutOfRange>(47)),
            "OutOfRange(47)");
}

TEST(BrushBehaviorTest, StringifyEnabledToolTypes) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::kAllToolTypes), "all");
  EXPECT_EQ(absl::StrCat(BrushBehavior::EnabledToolTypes{}), "none");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::EnabledToolTypes{.touch = true, .stylus = true}),
            "touch/stylus");
  EXPECT_EQ(absl::StrCat(BrushBehavior::EnabledToolTypes{
                .unknown = true, .mouse = true, .touch = true}),
            "unknown/mouse/touch");
}

TEST(BrushBehaviorTest, StringifyOptionalInputProperty) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::OptionalInputProperty::kPressure),
            "kPressure");
  EXPECT_EQ(absl::StrCat(BrushBehavior::OptionalInputProperty::kTilt), "kTilt");
  EXPECT_EQ(absl::StrCat(BrushBehavior::OptionalInputProperty::kOrientation),
            "kOrientation");
  EXPECT_EQ(absl::StrCat(BrushBehavior::OptionalInputProperty::kTiltXAndY),
            "kTiltXAndY");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::OptionalInputProperty>(73)),
            "OptionalInputProperty(73)");
}

TEST(BrushBehaviorTest, StringifyBinaryOp) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::BinaryOp::kProduct), "kProduct");
  EXPECT_EQ(absl::StrCat(BrushBehavior::BinaryOp::kSum), "kSum");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::BinaryOp>(147)),
            "BinaryOp(147)");
}

TEST(BrushBehaviorTest, StringifyDampingSource) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::DampingSource::kTimeInSeconds),
            "kTimeInSeconds");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::DampingSource>(73)),
            "DampingSource(73)");
}

TEST(BrushBehaviorTest, StringifyInterpolation) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::Interpolation::kLerp), "kLerp");
  EXPECT_EQ(absl::StrCat(BrushBehavior::Interpolation::kInverseLerp),
            "kInverseLerp");
  EXPECT_EQ(absl::StrCat(static_cast<BrushBehavior::Interpolation>(73)),
            "Interpolation(73)");
}

TEST(BrushBehaviorTest, StringifySourceNode) {
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kNormalizedPressure,
          .source_value_range = {0.5, 0.75},
      }),
      "SourceNode{source=kNormalizedPressure, source_value_range={0.5, 0.75}}");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kRepeat,
          .source_value_range = {0, 1},
      }),
      "SourceNode{source=kInputDistanceTraveledInCentimeters, "
      "source_out_of_range_behavior=kRepeat, "
      "source_value_range={0, 1}}");
}

TEST(BrushBehaviorTest, StringifyConstantNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::ConstantNode{0.25}),
            "ConstantNode{0.25}");
}

TEST(BrushBehaviorTest, StringifyFallbackFilterNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::FallbackFilterNode{
                BrushBehavior::OptionalInputProperty::kPressure}),
            "FallbackFilterNode{kPressure}");
}

TEST(BrushBehaviorTest, StringifyToolTypeFilterNode) {
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::ToolTypeFilterNode{
          BrushBehavior::EnabledToolTypes{.touch = true, .stylus = true}}),
      "ToolTypeFilterNode{touch/stylus}");
}

TEST(BrushBehaviorTest, StringifyDampingNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::DampingNode{
                .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
                .damping_gap = 0.25f,
            }),
            "DampingNode{damping_source=kTimeInSeconds, damping_gap=0.25}");
}

TEST(BrushBehaviorTest, StringifyResponseNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::ResponseNode{
                EasingFunction::Predefined::kEaseIn}),
            "ResponseNode{kEaseIn}");
}

TEST(BrushBehaviorTest, StringifyBinaryOpNode) {
  EXPECT_EQ(
      absl::StrCat(BrushBehavior::BinaryOpNode{BrushBehavior::BinaryOp::kSum}),
      "BinaryOpNode{kSum}");
  EXPECT_EQ(absl::StrCat(
                BrushBehavior::BinaryOpNode{BrushBehavior::BinaryOp::kProduct}),
            "BinaryOpNode{kProduct}");
}

TEST(BrushBehaviorTest, StringifyInterpolationNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::InterpolationNode{
                .interpolation = BrushBehavior::Interpolation::kLerp,
            }),
            "InterpolationNode{kLerp}");
  EXPECT_EQ(absl::StrCat(BrushBehavior::InterpolationNode{
                .interpolation = BrushBehavior::Interpolation::kInverseLerp,
            }),
            "InterpolationNode{kInverseLerp}");
}

TEST(BrushBehaviorTest, StringifyTargetNode) {
  EXPECT_EQ(absl::StrCat(BrushBehavior::TargetNode{
                .target = BrushBehavior::Target::kSizeMultiplier,
                .target_modifier_range = {0.5, 1.5},
            }),
            "TargetNode{target=kSizeMultiplier, "
            "target_modifier_range={0.5, 1.5}}");
}

TEST(BrushBehaviorTest, StringifyBrushBehavior) {
  EXPECT_EQ(absl::StrCat(BrushBehavior{{}}), "BrushBehavior{nodes={}}");
  EXPECT_EQ(
      absl::StrCat(BrushBehavior{{
          BrushBehavior::ConstantNode{1.f},
          BrushBehavior::TargetNode{
              .target =
                  BrushBehavior::Target::kPositionOffsetXInMultiplesOfBrushSize,
              .target_modifier_range = {0, 0.5},
          },
      }}),
      "BrushBehavior{nodes={ConstantNode{1}, "
      "TargetNode{target=kPositionOffsetXInMultiplesOfBrushSize, "
      "target_modifier_range={0, 0.5}}}}");
}

TEST(BrushBehaviorTest, EnabledToolTypesHasAllTypes) {
  EXPECT_TRUE(BrushBehavior::kAllToolTypes.HasAllTypes());
  EXPECT_FALSE(BrushBehavior::EnabledToolTypes{}.HasAllTypes());
  EXPECT_FALSE(BrushBehavior::EnabledToolTypes{.touch = true}.HasAllTypes());
  EXPECT_FALSE((BrushBehavior::EnabledToolTypes{.unknown = true, .mouse = true})
                   .HasAllTypes());
}

TEST(BrushBehaviorTest, EnabledToolTypesHasAnyTypes) {
  EXPECT_TRUE(BrushBehavior::kAllToolTypes.HasAnyTypes());
  EXPECT_FALSE(BrushBehavior::EnabledToolTypes{}.HasAnyTypes());
  EXPECT_TRUE(BrushBehavior::EnabledToolTypes{.unknown = true}.HasAnyTypes());
  EXPECT_TRUE(BrushBehavior::EnabledToolTypes{.mouse = true}.HasAnyTypes());
  EXPECT_TRUE(BrushBehavior::EnabledToolTypes{.touch = true}.HasAnyTypes());
  EXPECT_TRUE(BrushBehavior::EnabledToolTypes{.stylus = true}.HasAnyTypes());
}

TEST(BrushBehaviorTest, EnabledToolTypesEqualAndNotEqual) {
  BrushBehavior::EnabledToolTypes enabled_tool_types{
      .unknown = false, .mouse = true, .touch = false, .stylus = true};

  EXPECT_EQ(
      enabled_tool_types,
      BrushBehavior::EnabledToolTypes(
          {.unknown = false, .mouse = true, .touch = false, .stylus = true}));
  EXPECT_NE(
      enabled_tool_types,
      BrushBehavior::EnabledToolTypes(
          {.unknown = true, .mouse = true, .touch = false, .stylus = true}));
  EXPECT_NE(
      enabled_tool_types,
      BrushBehavior::EnabledToolTypes(
          {.unknown = false, .mouse = false, .touch = false, .stylus = true}));
  EXPECT_NE(
      enabled_tool_types,
      BrushBehavior::EnabledToolTypes(
          {.unknown = false, .mouse = true, .touch = true, .stylus = true}));
  EXPECT_NE(
      enabled_tool_types,
      BrushBehavior::EnabledToolTypes(
          {.unknown = false, .mouse = true, .touch = false, .stylus = false}));
}

TEST(BrushBehaviorTest, SourceNodeEqualAndNotEqual) {
  BrushBehavior::SourceNode node = {
      .source = BrushBehavior::Source::kNormalizedPressure,
      .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
      .source_value_range = {0.25, 0.5},
  };
  EXPECT_EQ(
      (BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kNormalizedPressure,
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
          .source_value_range = {0.25, 0.5},
      }),
      node);
  EXPECT_NE(
      (BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kTiltInRadians,  // different
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
          .source_value_range = {0.25, 0.5},
      }),
      node);
  EXPECT_NE((BrushBehavior::SourceNode{
                .source = BrushBehavior::Source::kNormalizedPressure,
                .source_out_of_range_behavior =
                    BrushBehavior::OutOfRange::kRepeat,  // different
                .source_value_range = {0.25, 0.5},
            }),
            node);
  EXPECT_NE(
      (BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kNormalizedPressure,
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
          .source_value_range = {0.25, 0.75},  // different
      }),
      node);
}

TEST(BrushBehaviorTest, ConstantNodeEqualAndNotEqual) {
  BrushBehavior::ConstantNode node = {.value = 42};
  EXPECT_EQ((BrushBehavior::ConstantNode{.value = 42}), node);
  EXPECT_NE((BrushBehavior::ConstantNode{.value = 37}), node);
}

TEST(BrushBehaviorTest, FallbackFilterNodeEqualAndNotEqual) {
  BrushBehavior::FallbackFilterNode node = {
      .is_fallback_for = BrushBehavior::OptionalInputProperty::kPressure};
  EXPECT_EQ(
      (BrushBehavior::FallbackFilterNode{
          .is_fallback_for = BrushBehavior::OptionalInputProperty::kPressure}),
      node);
  EXPECT_NE(
      (BrushBehavior::FallbackFilterNode{
          .is_fallback_for = BrushBehavior::OptionalInputProperty::kTilt}),
      node);
}

TEST(BrushBehaviorTest, ToolTypeFilterNodeEqualAndNotEqual) {
  BrushBehavior::ToolTypeFilterNode node = {
      .enabled_tool_types = {.stylus = true}};
  EXPECT_EQ((BrushBehavior::ToolTypeFilterNode{
                .enabled_tool_types = {.stylus = true}}),
            node);
  EXPECT_NE((BrushBehavior::ToolTypeFilterNode{
                .enabled_tool_types = {.touch = true}}),
            node);
}

TEST(BrushBehaviorTest, DampingNodeEqualAndNotEqual) {
  BrushBehavior::DampingNode node = {
      .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
      .damping_gap = 0.5,
  };
  EXPECT_EQ((BrushBehavior::DampingNode{
                .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
                .damping_gap = 0.5,
            }),
            node);
  EXPECT_NE((BrushBehavior::DampingNode{
                .damping_source = static_cast<BrushBehavior::DampingSource>(
                    123),  // different
                .damping_gap = 0.5,
            }),
            node);
  EXPECT_NE((BrushBehavior::DampingNode{
                .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
                .damping_gap = 0.75,  // different
            }),
            node);
}

TEST(BrushBehaviorTest, ResponseNodeEqualAndNotEqual) {
  BrushBehavior::ResponseNode node = {
      .response_curve = {EasingFunction::Predefined::kEaseIn}};
  EXPECT_EQ((BrushBehavior::ResponseNode{
                .response_curve = {EasingFunction::Predefined::kEaseIn}}),
            node);
  EXPECT_NE((BrushBehavior::ResponseNode{
                .response_curve = {EasingFunction::Predefined::kEaseOut}}),
            node);
}

TEST(BrushBehaviorTest, BinaryOpNodeEqualAndNotEqual) {
  BrushBehavior::BinaryOpNode node = {.operation =
                                          BrushBehavior::BinaryOp::kSum};
  EXPECT_EQ(
      (BrushBehavior::BinaryOpNode{.operation = BrushBehavior::BinaryOp::kSum}),
      node);
  EXPECT_NE((BrushBehavior::BinaryOpNode{
                .operation = BrushBehavior::BinaryOp::kProduct}),
            node);
}

TEST(BrushBehaviorTest, TargetNodeEqualAndNotEqual) {
  BrushBehavior::TargetNode node = {
      .target = BrushBehavior::Target::kSizeMultiplier,
      .target_modifier_range = {0.5, 1.5},
  };
  EXPECT_EQ((BrushBehavior::TargetNode{
                .target = BrushBehavior::Target::kSizeMultiplier,
                .target_modifier_range = {0.5, 1.5},
            }),
            node);
  EXPECT_NE((BrushBehavior::TargetNode{
                .target = BrushBehavior::Target::kPinchOffset,  // different
                .target_modifier_range = {0.5, 1.5},
            }),
            node);
  EXPECT_NE((BrushBehavior::TargetNode{
                .target = BrushBehavior::Target::kSizeMultiplier,
                .target_modifier_range = {0.5, 2.0},  // different
            }),
            node);
}

TEST(BrushBehaviorTest, BrushBehaviorEqualAndNotEqual) {
  EXPECT_EQ(BrushBehavior{}, BrushBehavior{});
  EXPECT_NE(BrushBehavior{},
            (BrushBehavior{{
                BrushBehavior::ConstantNode{.value = 1},
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kPinchOffset,
                    .target_modifier_range = {0, 1},
                },
            }}));
  EXPECT_EQ((BrushBehavior{{
                BrushBehavior::ConstantNode{.value = 1},
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kPinchOffset,
                    .target_modifier_range = {0, 1},
                },
            }}),
            (BrushBehavior{{
                BrushBehavior::ConstantNode{.value = 1},
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kPinchOffset,
                    .target_modifier_range = {0, 1},
                },
            }}));
  EXPECT_NE((BrushBehavior{{
                BrushBehavior::ConstantNode{.value = 1},
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kPinchOffset,
                    .target_modifier_range = {0, 1},
                },
            }}),
            (BrushBehavior{{
                BrushBehavior::ConstantNode{.value = 1},
                BrushBehavior::TargetNode{
                    .target = BrushBehavior::Target::kCornerRoundingOffset,
                    .target_modifier_range = {0, 1},
                },
            }}));
}

TEST(BrushBehaviorTest, ValidateSourceNode) {
  EXPECT_EQ(
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
          .source = BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
          .source_value_range = {0, 2},
      }),
      absl::OkStatus());

  absl::Status status =
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
          .source = static_cast<BrushBehavior::Source>(123),
          .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
          .source_value_range = {0, 2},
      });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
      .source = BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
      .source_out_of_range_behavior =
          static_cast<BrushBehavior::OutOfRange>(123),
      .source_value_range = {0, 2},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
      .source = BrushBehavior::Source::kTimeSinceInputInSeconds,
      .source_out_of_range_behavior = BrushBehavior::OutOfRange::kRepeat,
      .source_value_range = {0, 2},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("kTimeSinceInput*` must only be used with "
                        "`source_out_of_range_behavior` of `kClamp"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
      .source = BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
      .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
      .source_value_range = {0, kInfinity},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(
      status.message(),
      HasSubstr("source_value_range` must hold 2 finite and distinct values"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::SourceNode{
      .source = BrushBehavior::Source::kInputDistanceTraveledInCentimeters,
      .source_out_of_range_behavior = BrushBehavior::OutOfRange::kMirror,
      .source_value_range = {2, 2},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(
      status.message(),
      HasSubstr("source_value_range` must hold 2 finite and distinct values"));
}

TEST(BrushBehaviorTest, ValidateConstantNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(
                BrushBehavior::ConstantNode{-1}),
            absl::OkStatus());
  EXPECT_EQ(
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::ConstantNode{7}),
      absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::ConstantNode{kInfinity});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("must be finite"));

  status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::ConstantNode{kNan});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("must be finite"));
}

TEST(BrushBehaviorTest, ValidateFallbackFilterNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(
                BrushBehavior::FallbackFilterNode{
                    BrushBehavior::OptionalInputProperty::kPressure}),
            absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::FallbackFilterNode{
          static_cast<BrushBehavior::OptionalInputProperty>(123)});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));
}

TEST(BrushBehaviorTest, ValidateToolTypeFilterNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(
                BrushBehavior::ToolTypeFilterNode{
                    BrushBehavior::EnabledToolTypes{.mouse = true}}),
            absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::ToolTypeFilterNode{BrushBehavior::EnabledToolTypes{}});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("must contain at least one true value"));
}

TEST(BrushBehaviorTest, ValidateDampingNode) {
  EXPECT_EQ(
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::DampingNode{
          .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
          .damping_gap = 0.25,
      }),
      absl::OkStatus());

  absl::Status status =
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::DampingNode{
          .damping_source = static_cast<BrushBehavior::DampingSource>(123),
          .damping_gap = 0.25,
      });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::DampingNode{
      .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
      .damping_gap = -1,
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("damping_gap` must be finite and non-negative"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::DampingNode{
      .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
      .damping_gap = kInfinity,
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("damping_gap` must be finite and non-negative"));
}

TEST(BrushBehaviorTest, ValidateResponseNode) {
  EXPECT_EQ(
      brush_internal::ValidateBrushBehaviorNode(
          BrushBehavior::ResponseNode{EasingFunction::Predefined::kEaseIn}),
      absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::ResponseNode{EasingFunction::Steps{
          .step_count = -1,
          .step_position = EasingFunction::StepPosition::kJumpEnd,
      }});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
}

TEST(BrushBehaviorTest, ValidateBinaryOpNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(
                BrushBehavior::BinaryOpNode{BrushBehavior::BinaryOp::kSum}),
            absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::BinaryOpNode{static_cast<BrushBehavior::BinaryOp>(123)});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));
}

TEST(BrushBehaviorTest, ValidateInterpolationNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(
                BrushBehavior::InterpolationNode{
                    .interpolation = BrushBehavior::Interpolation::kLerp,
                }),
            absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehaviorNode(
      BrushBehavior::InterpolationNode{
          .interpolation = static_cast<BrushBehavior::Interpolation>(123),
      });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));
}

TEST(BrushBehaviorTest, ValidateTargetNode) {
  EXPECT_EQ(brush_internal::ValidateBrushBehaviorNode(BrushBehavior::TargetNode{
                .target = BrushBehavior::Target::kSizeMultiplier,
                .target_modifier_range = {0, 2},
            }),
            absl::OkStatus());

  absl::Status status =
      brush_internal::ValidateBrushBehaviorNode(BrushBehavior::TargetNode{
          .target = static_cast<BrushBehavior::Target>(123),
          .target_modifier_range = {0, 2},
      });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("non-enumerator value 123"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::TargetNode{
      .target = BrushBehavior::Target::kSizeMultiplier,
      .target_modifier_range = {0, kInfinity},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(
      status.message(),
      HasSubstr(
          "target_modifier_range` must hold 2 finite and distinct values"));

  status = brush_internal::ValidateBrushBehaviorNode(BrushBehavior::TargetNode{
      .target = BrushBehavior::Target::kSizeMultiplier,
      .target_modifier_range = {2, 2},
  });
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(
      status.message(),
      HasSubstr(
          "target_modifier_range` must hold 2 finite and distinct values"));
}

TEST(BrushBehaviorTest, ValidateBrushBehavior) {
  EXPECT_EQ(brush_internal::ValidateBrushBehavior(BrushBehavior{}),
            absl::OkStatus());
  EXPECT_EQ(
      brush_internal::ValidateBrushBehavior(BrushBehavior{{
          BrushBehavior::SourceNode{
              .source = BrushBehavior::Source::kNormalizedPressure,
              .source_value_range = {0.5, 0.75},
          },
          BrushBehavior::ToolTypeFilterNode{{.stylus = true}},
          BrushBehavior::FallbackFilterNode{
              BrushBehavior::OptionalInputProperty::kTilt},
          BrushBehavior::DampingNode{
              .damping_source = BrushBehavior::DampingSource::kTimeInSeconds,
              .damping_gap = 0.25,
          },
          BrushBehavior::ConstantNode{0.75},
          BrushBehavior::BinaryOpNode{BrushBehavior::BinaryOp::kProduct},
          BrushBehavior::TargetNode{
              .target = BrushBehavior::Target::kSizeMultiplier,
              .target_modifier_range = {1, 2},
          },
      }}),
      absl::OkStatus());

  absl::Status status = brush_internal::ValidateBrushBehavior(BrushBehavior{{
      BrushBehavior::ResponseNode{EasingFunction::Predefined::kEaseOut},
  }});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Insufficient inputs"));

  status = brush_internal::ValidateBrushBehavior(BrushBehavior{{
      BrushBehavior::ConstantNode{0},
      BrushBehavior::ConstantNode{1},
  }});
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("there were 2 values remaining"));
}

void CanValidateValidBrushBehavior(const BrushBehavior& behavior) {
  EXPECT_EQ(absl::OkStatus(), brush_internal::ValidateBrushBehavior(behavior));
}
FUZZ_TEST(EasingFunctionTest, CanValidateValidBrushBehavior)
    .WithDomains(ValidBrushBehavior());

void CanValidateValidBrushBehaviorNode(const BrushBehavior::Node& node) {
  EXPECT_EQ(absl::OkStatus(), brush_internal::ValidateBrushBehaviorNode(node));
}
FUZZ_TEST(EasingFunctionTest, CanValidateValidBrushBehaviorNode)
    .WithDomains(ValidBrushBehaviorNode());

}  // namespace
}  // namespace ink
