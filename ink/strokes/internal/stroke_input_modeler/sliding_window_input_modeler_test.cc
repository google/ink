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

#include "ink/strokes/internal/stroke_input_modeler/sliding_window_input_modeler.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush_family.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"
#include "ink/types/type_matchers.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::FloatNear;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Not;
using ::testing::Optional;
using ::testing::SizeIs;

// Returns a vector of single-input `StrokeInputBatch` that can be used for a
// single synthetic stroke.
std::vector<StrokeInputBatch> MakeStylusInputBatchSequence() {
  StrokeInput::ToolType tool_type = StrokeInput::ToolType::kStylus;
  PhysicalDistance stroke_unit_length = PhysicalDistance::Centimeters(1);
  std::vector<StrokeInput> inputs = {{.tool_type = tool_type,
                                      .position = {10, 20},
                                      .elapsed_time = Duration32::Zero(),
                                      .stroke_unit_length = stroke_unit_length,
                                      .pressure = 0.4,
                                      .tilt = Angle::Radians(1),
                                      .orientation = Angle::Radians(2)},
                                     {.tool_type = tool_type,
                                      .position = {10, 23},
                                      .elapsed_time = Duration32::Seconds(1),
                                      .stroke_unit_length = stroke_unit_length,
                                      .pressure = 0.3,
                                      .tilt = Angle::Radians(0.9),
                                      .orientation = Angle::Radians(0.9)},
                                     {.tool_type = tool_type,
                                      .position = {10, 17},
                                      .elapsed_time = Duration32::Seconds(2),
                                      .stroke_unit_length = stroke_unit_length,
                                      .pressure = 0.5,
                                      .tilt = Angle::Radians(0.8),
                                      .orientation = Angle::Radians(1.1)},
                                     {.tool_type = tool_type,
                                      .position = {5, 5},
                                      .elapsed_time = Duration32::Seconds(3),
                                      .stroke_unit_length = stroke_unit_length,
                                      .pressure = 0.8,
                                      .tilt = Angle::Radians(1.5),
                                      .orientation = Angle::Radians(1.3)},
                                     {.tool_type = tool_type,
                                      .position = {4, 3},
                                      .elapsed_time = Duration32::Seconds(5),
                                      .stroke_unit_length = stroke_unit_length,
                                      .pressure = 1.0,
                                      .tilt = Angle::Radians(1.3),
                                      .orientation = Angle::Radians(1.5)}};

  std::vector<StrokeInputBatch> batches;
  for (const StrokeInput& input : inputs) {
    absl::StatusOr<StrokeInputBatch> batch = StrokeInputBatch::Create({input});
    ABSL_CHECK_OK(batch);
    batches.push_back(*batch);
  }
  return batches;
}

TEST(SlidingWindowInputModelerTest, EraseInitialPredictionWithNoRealInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  SlidingWindowInputModeler modeler(
      /* window_size= */ Duration32::Millis(20),
      /* upsampling_period= */ Duration32::Millis(5));
  modeler.StartStroke(/* brush_epsilon = */ 0.01);

  // Start off with some predicted inputs, but no real inputs (this doesn't
  // generally occur in practice, but is a legal usage of the API). There should
  // be some modeled inputs, with nonzero elapsed time and distance traveled.
  StrokeInputBatch synthetic_predicted_inputs = input_batches[0];
  ASSERT_THAT(synthetic_predicted_inputs.Append(input_batches[1]), IsOk());
  modeler.ExtendStroke({}, synthetic_predicted_inputs, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), Not(IsEmpty()));
  EXPECT_GT(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_GT(modeler.GetState().complete_traveled_distance, 0);

  // Now erase the prediction, still with no real inputs. Elapsed time and
  // distance traveled should go back to zero.
  modeler.ExtendStroke({}, {}, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), IsEmpty());
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_EQ(modeler.GetState().complete_traveled_distance, 0);
}

TEST(SlidingWindowInputModelerTest, ConstantVelocityRawInputs) {
  SlidingWindowInputModeler modeler(
      /* window_size= */ Duration32::Millis(20),
      /* upsampling_period= */ Duration32::Millis(5));
  modeler.StartStroke(/* brush_epsilon = */ 0.01);

  // Extend the stroke with a bunch of inputs that move at a constant velocity
  // of 1000 stroke units per second.
  StrokeInputBatch inputs;
  for (int i = 0; i < 100; ++i) {
    StrokeInput input = StrokeInput{
        .position = {static_cast<float>(i), 0},
        .elapsed_time = Duration32::Millis(i),
    };
    ASSERT_THAT(inputs.Append(input), IsOk());
  }
  modeler.ExtendStroke(inputs, {}, Duration32::Millis(100));

  // All modeled inputs (even the first and last one) should have a modeled
  // velocity of about 1000 stroke units per second, and a modeled acceleration
  // of (roughly) zero.
  EXPECT_THAT(modeler.GetModeledInputs(), SizeIs(Ge(100)));
  EXPECT_THAT(modeler.GetModeledInputs(),
              Each(Field("velocity", &ModeledStrokeInput::velocity,
                         VecNear({1000, 0}, 0.001))));
  EXPECT_THAT(modeler.GetModeledInputs(),
              Each(Field("acceleration", &ModeledStrokeInput::acceleration,
                         VecNear({0, 0}, 0.5))));
}

TEST(SlidingWindowInputModelerTest, Orientation) {
  SlidingWindowInputModeler modeler(
      /* window_size= */ Duration32::Millis(10),
      /* upsampling_period= */ Duration32::Millis(5));
  modeler.StartStroke(/* brush_epsilon = */ 0.01);

  // Extend the stroke with a bunch of inputs with an orientation of 10°, then
  // a bunch of inputs with an orientation of 350°.
  StrokeInputBatch inputs;
  for (int i = 0; i < 100; ++i) {
    StrokeInput input = StrokeInput{
        .position = {static_cast<float>(i), 0},
        .elapsed_time = Duration32::Millis(i),
        .orientation = i < 50 ? Angle::Degrees(10) : Angle::Degrees(350),
    };
    ASSERT_THAT(inputs.Append(input), IsOk());
  }
  modeler.ExtendStroke(inputs, {}, Duration32::Millis(100));

  // All modeled inputs should have an orientation (roughly) between ±10° when
  // normalized about zero; they shouldn't be naively averaged between 10° and
  // 350° to get ~180°.
  EXPECT_THAT(modeler.GetModeledInputs(), SizeIs(Ge(100)));
  for (const ModeledStrokeInput& modeled_input : modeler.GetModeledInputs()) {
    ASSERT_NE(modeled_input.orientation, StrokeInput::kNoOrientation);
    EXPECT_THAT(
        modeled_input.orientation.NormalizedAboutZero(),
        AllOf(Ge(Angle::Degrees(-10.0001)), Le(Angle::Degrees(10.0001))));
  }
}

TEST(SlidingWindowInputModelerTest, Upsampling) {
  SlidingWindowInputModeler modeler(
      /* window_size= */ Duration32::Millis(10),
      /* upsampling_period= */ Duration32::Millis(1));
  modeler.StartStroke(/* brush_epsilon = */ 0.01);

  // Extend the stroke with three raw inputs, spaced 10 ms apart.
  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      {.position = {0, 0}, .elapsed_time = Duration32::Millis(0)},
      {.position = {100, 0}, .elapsed_time = Duration32::Millis(10)},
      {.position = {100, 100}, .elapsed_time = Duration32::Millis(20)},
  });
  ASSERT_THAT(inputs, IsOk());
  modeler.ExtendStroke(*inputs, {}, Duration32::Millis(20));

  // Since the upsampling period is 1 ms, we should end up with 21 modeled
  // inputs.
  absl::Span<const ModeledStrokeInput> modeled = modeler.GetModeledInputs();
  ASSERT_THAT(modeled, SizeIs(21));
  // The modeled positions should move in a smooth curve near the corner of the
  // L shaped formed by the raw inputs.
  EXPECT_THAT(modeled[0].position, PointNear({0.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[1].position, PointNear({10.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[2].position, PointNear({20.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[3].position, PointNear({30.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[4].position, PointNear({40.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[5].position, PointNear({50.0, 0.0}, 0.1));
  EXPECT_THAT(modeled[6].position, PointNear({59.5, 0.5}, 0.1));
  EXPECT_THAT(modeled[7].position, PointNear({68.0, 2.0}, 0.1));
  EXPECT_THAT(modeled[8].position, PointNear({75.5, 4.5}, 0.1));
  EXPECT_THAT(modeled[9].position, PointNear({82.0, 8.0}, 0.1));
  EXPECT_THAT(modeled[10].position, PointNear({87.5, 12.5}, 0.1));
  EXPECT_THAT(modeled[11].position, PointNear({92.0, 18.0}, 0.1));
  EXPECT_THAT(modeled[12].position, PointNear({95.5, 24.5}, 0.1));
  EXPECT_THAT(modeled[13].position, PointNear({98.0, 32.0}, 0.1));
  EXPECT_THAT(modeled[14].position, PointNear({99.5, 40.5}, 0.1));
  EXPECT_THAT(modeled[15].position, PointNear({100.0, 50.0}, 0.1));
  EXPECT_THAT(modeled[16].position, PointNear({100.0, 60.0}, 0.1));
  EXPECT_THAT(modeled[17].position, PointNear({100.0, 70.0}, 0.1));
  EXPECT_THAT(modeled[18].position, PointNear({100.0, 80.0}, 0.1));
  EXPECT_THAT(modeled[19].position, PointNear({100.0, 90.0}, 0.1));
  EXPECT_THAT(modeled[20].position, PointNear({100.0, 100.0}, 0.1));
}

}  // namespace
}  // namespace ink::strokes_internal
