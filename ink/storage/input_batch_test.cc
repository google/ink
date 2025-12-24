// Copyright 2024-2025 Google LLC
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

#include "ink/storage/input_batch.h"

#include <limits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/storage/proto/stroke_input_batch.pb.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/type_matchers.h"
#include "ink/types/duration.h"
#include "ink/types/iterator_range.h"
#include "ink/types/numbers.h"
#include "ink/types/physical_distance.h"
#include "google/protobuf/text_format.h"

namespace ink {
namespace {

using ::ink::numbers::kPi;
using ::google::protobuf::TextFormat;
using ::testing::ElementsAre;
using ::testing::HasSubstr;

TEST(CodedStrokeInputBatchIteratorTest, DecodeStylusStrokeInputBatch) {
  proto::CodedStrokeInputBatch coded;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        tool_type: STYLUS
        stroke_unit_length_in_centimeters: 1.0
        x_stroke_space {
          scale: 2.5
          deltas: [ 1, 2, 1 ]
        }
        y_stroke_space {
          scale: 2.5
          deltas: [ 1, -1, 1 ]
        }
        elapsed_time_seconds {
          scale: 0.001
          deltas: [ 0, 30, 30 ]
        }
        pressure {
          scale: 0.01
          deltas: [ 25, 50, -25 ]
        }
        tilt {
          scale: 0.0314159
          deltas: [ 10, 10, 10 ]
        }
        orientation {
          scale: 0.0314159
          deltas: [ 25, -50, 25 ]
        }
      )pb",
      &coded));

  absl::StatusOr<iterator_range<CodedStrokeInputBatchIterator>> range =
      DecodeStrokeInputBatchProto(coded);
  ASSERT_EQ(range.status(), absl::OkStatus());
  std::vector<StrokeInput> values(range->begin(), range->end());
  EXPECT_THAT(
      values,
      ElementsAre(
          StrokeInputNear(
              {
                  .tool_type = StrokeInput::ToolType::kStylus,
                  .position = {2.5f, 2.5f},
                  .elapsed_time = Duration32::Seconds(0.000f),
                  .stroke_unit_length = PhysicalDistance::Centimeters(1.0f),
                  .pressure = 0.25f,
                  .tilt = Angle::Radians(kPi / 10),
                  .orientation = Angle::Radians(kPi / 4),
              },
              1e-5f),
          StrokeInputNear(
              {
                  .tool_type = StrokeInput::ToolType::kStylus,
                  .position = {7.5f, 0.0f},
                  .elapsed_time = Duration32::Seconds(0.030f),
                  .stroke_unit_length = PhysicalDistance::Centimeters(1.0f),
                  .pressure = 0.75f,
                  .tilt = Angle::Radians(2 * kPi / 10),
                  .orientation = Angle::Radians(-kPi / 4),
              },
              1e-5f),
          StrokeInputNear(
              {
                  .tool_type = StrokeInput::ToolType::kStylus,
                  .position = {10.0f, 2.5f},
                  .elapsed_time = Duration32::Seconds(0.060f),
                  .stroke_unit_length = PhysicalDistance::Centimeters(1.0f),
                  .pressure = 0.50f,
                  .tilt = Angle::Radians(3 * kPi / 10),
                  .orientation = Angle::Radians(0.0f),
              },
              1e-5f)));
}

TEST(CodedStrokeInputBatchIteratorTest, DecodeMouseStrokeInputBatch) {
  // Pressure/tilt/orientation are optional and may be omitted (e.g. for mouse
  // input).
  proto::CodedStrokeInputBatch coded;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        tool_type: MOUSE
        x_stroke_space {
          scale: 2.5
          deltas: [ 1, 2, 1 ]
        }
        y_stroke_space {
          scale: 2.5
          deltas: [ 1, -1, 1 ]
        }
        elapsed_time_seconds {
          scale: 0.001
          deltas: [ 0, 30, 30 ]
        }
      )pb",
      &coded));

  absl::StatusOr<iterator_range<CodedStrokeInputBatchIterator>> range =
      DecodeStrokeInputBatchProto(coded);
  ASSERT_EQ(range.status(), absl::OkStatus());
  std::vector<StrokeInput> values(range->begin(), range->end());
  ASSERT_THAT(values,
              ElementsAre(StrokeInputEq({
                              .tool_type = StrokeInput::ToolType::kMouse,
                              .position = {2.5f, 2.5f},
                              .elapsed_time = Duration32::Seconds(0.000f),
                              .stroke_unit_length = PhysicalDistance::Zero(),
                              .pressure = StrokeInput::kNoPressure,
                              .tilt = StrokeInput::kNoTilt,
                              .orientation = StrokeInput::kNoOrientation,
                          }),
                          StrokeInputEq({
                              .tool_type = StrokeInput::ToolType::kMouse,
                              .position = {7.5f, 0.0f},
                              .elapsed_time = Duration32::Seconds(0.030f),
                              .stroke_unit_length = PhysicalDistance::Zero(),
                              .pressure = StrokeInput::kNoPressure,
                              .tilt = StrokeInput::kNoTilt,
                              .orientation = StrokeInput::kNoOrientation,
                          }),
                          StrokeInputEq({
                              .tool_type = StrokeInput::ToolType::kMouse,
                              .position = {10.0f, 2.5f},
                              .elapsed_time = Duration32::Seconds(0.060f),
                              .stroke_unit_length = PhysicalDistance::Zero(),
                              .pressure = StrokeInput::kNoPressure,
                              .tilt = StrokeInput::kNoTilt,
                              .orientation = StrokeInput::kNoOrientation,
                          })));
}

TEST(CodedStrokeInputBatchIteratorTest, IteratorPostIncrement) {
  proto::CodedStrokeInputBatch coded;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        x_stroke_space { deltas: [ 1, 2, 1 ] }
        y_stroke_space { deltas: [ 1, -1, 1 ] }
        elapsed_time_seconds { deltas: [ 0, 1, 1 ] }
      )pb",
      &coded));

  absl::StatusOr<iterator_range<CodedStrokeInputBatchIterator>> range =
      DecodeStrokeInputBatchProto(coded);
  ASSERT_EQ(range.status(), absl::OkStatus());
  CodedStrokeInputBatchIterator iter = range->begin();
  const CodedStrokeInputBatchIterator iter0 = iter++;
  const CodedStrokeInputBatchIterator iter1 = iter++;
  const CodedStrokeInputBatchIterator iter2 = iter++;

  EXPECT_THAT(iter0->position, PointEq({1, 1}));
  EXPECT_THAT(iter1->position, PointEq({3, 0}));
  EXPECT_THAT(iter2->position, PointEq({4, 1}));
  EXPECT_EQ(iter, range->end());
}

TEST(CodedStrokeInputBatchIteratorTest, DecodeEmptyStrokeInputBatch) {
  proto::CodedStrokeInputBatch coded;
  absl::StatusOr<iterator_range<CodedStrokeInputBatchIterator>> batch =
      DecodeStrokeInputBatchProto(coded);
  ASSERT_EQ(batch.status(), absl::OkStatus());
  EXPECT_THAT(*batch, ElementsAre());
}

TEST(CodedStrokeInputBatchIteratorTest,
     DecodeStrokeInputBatchWithMismatchedLengths) {
  proto::CodedStrokeInputBatch coded;
  coded.mutable_pressure()->add_deltas(1);
  absl::Status mismatched_lengths = DecodeStrokeInputBatchProto(coded).status();
  EXPECT_EQ(mismatched_lengths.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(mismatched_lengths.message(),
              HasSubstr("mismatched numeric run lengths"));
}

TEST(CodedStrokeInputBatchIteratorTest,
     DecodeStrokeInputBatchWithInvalidPressure) {
  proto::CodedStrokeInputBatch coded;
  coded.mutable_x_stroke_space()->add_deltas(0);
  coded.mutable_y_stroke_space()->add_deltas(0);
  coded.mutable_elapsed_time_seconds()->add_deltas(0);
  coded.mutable_pressure()->add_deltas(0);
  coded.mutable_pressure()->set_scale(std::numeric_limits<float>::infinity());
  absl::Status non_finite_scale = DecodeStrokeInputBatchProto(coded).status();
  EXPECT_EQ(non_finite_scale.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(non_finite_scale.message(), HasSubstr("non-finite scale"));
}

}  // namespace
}  // namespace ink
