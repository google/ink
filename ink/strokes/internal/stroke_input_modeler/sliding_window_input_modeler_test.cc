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

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
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
    auto batch = StrokeInputBatch::Create({input});
    ABSL_CHECK_OK(batch);
    batches.push_back(*batch);
  }
  return batches;
}

TEST(SlidingWindowInputModelerTest, EraseInitialPredictionWithNoRealInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  SlidingWindowInputModeler modeler(Duration32::Millis(20));
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
  SlidingWindowInputModeler modeler(Duration32::Millis(20));
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
  EXPECT_THAT(modeler.GetModeledInputs(), SizeIs(100));
  EXPECT_THAT(modeler.GetModeledInputs(),
              Each(Field("velocity", &ModeledStrokeInput::velocity,
                         VecNear({1000, 0}, 0.001))));
  EXPECT_THAT(modeler.GetModeledInputs(),
              Each(Field("acceleration", &ModeledStrokeInput::acceleration,
                         VecNear({0, 0}, 0.5))));
}

TEST(SlidingWindowInputModelerTest, Orientation) {
  SlidingWindowInputModeler modeler(Duration32::Millis(10));
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
  EXPECT_THAT(modeler.GetModeledInputs(), SizeIs(100));
  for (const ModeledStrokeInput& modeled_input : modeler.GetModeledInputs()) {
    ASSERT_NE(modeled_input.orientation, StrokeInput::kNoOrientation);
    EXPECT_THAT(
        modeled_input.orientation.NormalizedAboutZero(),
        AllOf(Ge(Angle::Degrees(-10.0001)), Le(Angle::Degrees(10.0001))));
  }
}

}  // namespace
}  // namespace ink::strokes_internal
