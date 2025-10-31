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
#include "ink/geometry/angle.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink::stroke_input_internal {
namespace {

using ::testing::HasSubstr;

TEST(ValidateConsistentAttributesTest, ValidInputsWithNoOptionalProperties) {
  EXPECT_EQ(absl::OkStatus(),
            ValidateConsistentAttributes(
                {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                {.position = {2, 3}, .elapsed_time = Duration32::Millis(10)}));
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithAllOptionalProperties) {
  EXPECT_EQ(absl::OkStatus(), ValidateConsistentAttributes(
                                  {.position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5),
                                   .pressure = 0.5,
                                   .tilt = kFullTurn / 8,
                                   .orientation = kHalfTurn},
                                  {.position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10),
                                   .pressure = 0.6,
                                   .tilt = kQuarterTurn,
                                   .orientation = kFullTurn}));
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyPressure) {
  EXPECT_EQ(absl::OkStatus(), ValidateConsistentAttributes(
                                  {.position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5),
                                   .pressure = 0.5},
                                  {.position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10),
                                   .pressure = 0.6}));
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyTilt) {
  EXPECT_EQ(absl::OkStatus(), ValidateConsistentAttributes(
                                  {.position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5),
                                   .tilt = kFullTurn / 8},
                                  {.position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10),
                                   .tilt = kQuarterTurn}));
}

TEST(ValidateConsistentAttributesTest, ValidInputsWithOnlyOrientation) {
  EXPECT_EQ(absl::OkStatus(), ValidateConsistentAttributes(
                                  {.position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5),
                                   .orientation = kHalfTurn},
                                  {.position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10),
                                   .orientation = kFullTurn}));
}

TEST(ValidateAdvancingXytTest, ValidInputsDuplicatePosition) {
  EXPECT_EQ(absl::OkStatus(),
            ValidateAdvancingXYT(
                {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                {.position = {1, 2}, .elapsed_time = Duration32::Millis(10)}));
}

TEST(ValidateAdvancingXytTest, ValidInputsDuplicateElapsedTime) {
  EXPECT_EQ(absl::OkStatus(),
            ValidateAdvancingXYT(
                {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
                {.position = {2, 3}, .elapsed_time = Duration32::Millis(5)}));
}

TEST(ValidateConsistentAttributesTest, MismatchedToolTypes) {
  absl::Status mismatched_tool_type =
      ValidateConsistentAttributes({.tool_type = StrokeInput::ToolType::kMouse,
                                    .position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5)},
                                   {.tool_type = StrokeInput::ToolType::kStylus,
                                    .position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10)});
  EXPECT_EQ(mismatched_tool_type.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_tool_type.message(), HasSubstr("tool_type"));
}

TEST(ValidateAdvancingXytTest, DuplicatePositionAndElapsedTime) {
  absl::Status duplicate_input = ValidateAdvancingXYT(
      {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
      {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)});
  EXPECT_EQ(duplicate_input.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(duplicate_input.message(), HasSubstr("duplicate"));
}

TEST(ValidateAdvancingXytTest, DecreasingElapsedTime) {
  absl::Status decreasing_time = ValidateAdvancingXYT(
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
      {.position = {2, 3}, .elapsed_time = Duration32::Seconds(0.99)});
  EXPECT_EQ(decreasing_time.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(decreasing_time.message(), HasSubstr("non-decreasing"));
}

TEST(ValidateConsistentAttributesTest, MismatchedStrokeUnitLength) {
  absl::Status mismatched_tool_type = ValidateConsistentAttributes(
      {.position = {1, 2},
       .elapsed_time = Duration32::Millis(5),
       .stroke_unit_length = PhysicalDistance::Centimeters(1)},
      {.position = {2, 3},
       .elapsed_time = Duration32::Millis(10),
       .stroke_unit_length = PhysicalDistance::Centimeters(2)});
  EXPECT_EQ(mismatched_tool_type.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_tool_type.message(), HasSubstr("stroke_unit_length"));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalPressure) {
  absl::Status mismatched_has_pressure =
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .pressure = StrokeInput::kNoPressure},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .pressure = 0.5});
  EXPECT_EQ(mismatched_has_pressure.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_has_pressure.message(), HasSubstr("pressure"));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalTilt) {
  absl::Status mismatched_has_tilt =
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .tilt = StrokeInput::kNoTilt},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .tilt = kQuarterTurn});
  EXPECT_EQ(mismatched_has_tilt.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_has_tilt.message(), HasSubstr("tilt"));
}

TEST(ValidateConsistentAttributesTest, MismatchedOptionalOrientation) {
  absl::Status mismatched_has_orientation =
      ValidateConsistentAttributes({.position = {1, 2},
                                    .elapsed_time = Duration32::Millis(5),
                                    .orientation = StrokeInput::kNoOrientation},
                                   {.position = {2, 3},
                                    .elapsed_time = Duration32::Millis(10),
                                    .orientation = kHalfTurn});
  EXPECT_EQ(mismatched_has_orientation.code(),
            absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_has_orientation.message(), HasSubstr("orientation"));
}

TEST(ValidateConsecutiveInputs, MismatchedAttributes) {
  // Mismatched attributes
  {
    absl::Status mismatched_has_orientation =
        ValidateConsecutiveInputs({.position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5),
                                   .orientation = StrokeInput::kNoOrientation},
                                  {.position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10),
                                   .orientation = kHalfTurn});
    EXPECT_EQ(mismatched_has_orientation.code(),
              absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(mismatched_has_orientation.message(), HasSubstr("orientation"));
  }

  // Mismatched tool-types
  {
    absl::Status mismatched_tool_type =
        ValidateConsecutiveInputs({.tool_type = StrokeInput::ToolType::kMouse,
                                   .position = {1, 2},
                                   .elapsed_time = Duration32::Millis(5)},
                                  {.tool_type = StrokeInput::ToolType::kStylus,
                                   .position = {2, 3},
                                   .elapsed_time = Duration32::Millis(10)});
    EXPECT_EQ(mismatched_tool_type.code(), absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(mismatched_tool_type.message(), HasSubstr("tool_type"));
  }
}

TEST(ValidateConsecutiveInputs, InvalidPositionsAndTimes) {
  // Decreasing time
  {
    absl::Status decreasing_time = ValidateConsecutiveInputs(
        {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
        {.position = {2, 3}, .elapsed_time = Duration32::Seconds(0.99)});
    EXPECT_EQ(decreasing_time.code(), absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(decreasing_time.message(), HasSubstr("non-decreasing"));
  }

  // Duplicate position and time
  {
    absl::Status duplicate_input = ValidateConsecutiveInputs(
        {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)},
        {.position = {1, 2}, .elapsed_time = Duration32::Millis(5)});
    EXPECT_EQ(duplicate_input.code(), absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(duplicate_input.message(), HasSubstr("duplicate"));
  }
}

}  // namespace
}  // namespace ink::stroke_input_internal
