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

#include "ink/strokes/input/internal/stroke_input_validation_helpers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "ink/geometry/angle.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink::stroke_input_internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

TEST(ValidateConsistentAttributesTest, ValidInputsWithNoOptionalProperties) {
  EXPECT_THAT(ValidateConsistentAttributes(
                  {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                  {.position = {2, 3}, .elapsed_time = Duration32::Millis(10)}),
              IsOk());
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithAllOptionalProperties) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .pressure = 0.5,
                                    .tilt = kFullTurn / 8,
                                    .orientation = kHalfTurn},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .pressure = 0.6,
                                    .tilt = kQuarterTurn,
                                    .orientation = kFullTurn}),
      IsOk());
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyPressure) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .pressure = 0.5},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .pressure = 0.6}),
      IsOk());
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyTilt) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .tilt = kFullTurn / 8},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .tilt = kQuarterTurn}),
      IsOk());
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyOrientation) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .orientation = kHalfTurn},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .orientation = kFullTurn}),
      IsOk());
}

TEST(ValidateAdvancingXytTest, ValidInputsDuplicatePosition) {
  EXPECT_THAT(ValidateAdvancingXYT(
                  {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                  {.position = {1, 2}, .elapsed_time = Duration32::Millis(10)}),
              IsOk());
}

TEST(ValidateAdvancingXytTest, ValidInputsDuplicateElapsedTime) {
  EXPECT_THAT(ValidateAdvancingXYT(
                  {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                  {.position = {2, 3}, .elapsed_time = Duration32::Millis(5)}),
              IsOk());
}

TEST(ValidateConsistentAttributesTest, MismatchedToolTypes) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.tool_type = StrokeInput::ToolType::kMouse,
                                    .position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5)},
                                   {.tool_type = StrokeInput::ToolType::kStylus,
                                    .position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10)}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("tool_type")));
}

TEST(ValidateAdvancingXytTest, DuplicatePositionAndElapsedTime) {
  EXPECT_THAT(
      ValidateAdvancingXYT(
          {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
          {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("duplicate")));
}

TEST(ValidateAdvancingXytTest, DecreasingElapsedTime) {
  EXPECT_THAT(
      ValidateAdvancingXYT(
          {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
          {.position = {2, 3}, .elapsed_time = Duration32::Seconds(0.99)}),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("non-decreasing")));
}

TEST(ValidateConsistentAttributesTest, MismatchedStrokeUnitLength) {
  EXPECT_THAT(ValidateConsistentAttributes(
                  {.position = {1, 2},
                   .elapsed_time = Duration32::Millis(5),
                   .stroke_unit_length = PhysicalDistance::Centimeters(1)},
                  {.position = {2, 3},
                   .elapsed_time = Duration32::Millis(10),
                   .stroke_unit_length = PhysicalDistance::Centimeters(2)}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("stroke_unit_length")));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalPressure) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .pressure = StrokeInput::kNoPressure},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .pressure = 0.5}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("pressure")));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalTilt) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .tilt = StrokeInput::kNoTilt},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .tilt = kQuarterTurn}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("tilt")));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalOrientation) {
  EXPECT_THAT(
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .orientation = StrokeInput::kNoOrientation},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .orientation = kHalfTurn}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("orientation")));
}

TEST(ValidateConsecutiveInputs, MismatchedAttributes) {
  // Mismatched attributes
  EXPECT_THAT(
      ValidateConsecutiveInputs({.position = {1, 2},
                                 .elapsed_time = Duration32::Millis(5),
                                 .orientation = StrokeInput::kNoOrientation},
                                {.position = {2, 3},
                                 .elapsed_time = Duration32::Millis(10),
                                 .orientation = kHalfTurn}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("orientation")));

  // Mismatched tool-types
  EXPECT_THAT(
      ValidateConsecutiveInputs({.tool_type = StrokeInput::ToolType::kMouse,
                                 .position = {1, 2},
                                 .elapsed_time = Duration32::Millis(5)},
                                {.tool_type = StrokeInput::ToolType::kStylus,
                                 .position = {2, 3},
                                 .elapsed_time = Duration32::Millis(10)}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("tool_type")));
}

TEST(ValidateConsecutiveInputs, InvalidPositionsAndTimes) {
  // Decreasing time
  EXPECT_THAT(
      ValidateConsecutiveInputs(
          {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
          {.position = {2, 3}, .elapsed_time = Duration32::Seconds(0.99)}),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("non-decreasing")));

  // Duplicate position and time
  EXPECT_THAT(
      ValidateConsecutiveInputs(
          {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
          {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("duplicate")));
}

}  // namespace
}  // namespace ink::stroke_input_internal
