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

#include "ink/strokes/internal/stroke_input_modeler.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
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
using ::testing::FloatNear;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;

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

struct InputModelTestCase {
  std::string test_name;
  BrushFamily::InputModel input_model;
};

// Suite of tests that should pass for any conforming StrokeInputModel
// implementation.
class StrokeInputModelerTest
    : public ::testing::TestWithParam<InputModelTestCase> {};

TEST_P(StrokeInputModelerTest, InitialState) {
  StrokeInputModeler modeler;

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kUnknown);
  EXPECT_EQ(modeler.GetState().stroke_unit_length, std::nullopt);
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), IsEmpty());
  EXPECT_EQ(modeler.GetState().stable_input_count, 0);
  EXPECT_EQ(modeler.GetState().real_input_count, 0);
}

TEST_P(StrokeInputModelerTest, StartOnDefaultConstructed) {
  StrokeInputModeler modeler;
  modeler.StartStroke(GetParam().input_model, /* brush_epsilon = */ 0.01);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kUnknown);
  EXPECT_EQ(modeler.GetState().stroke_unit_length, std::nullopt);
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), IsEmpty());
  EXPECT_EQ(modeler.GetState().stable_input_count, 0);
  EXPECT_EQ(modeler.GetState().real_input_count, 0);
}

TEST_P(StrokeInputModelerTest, FirstExtendWithEmptyInputs) {
  StrokeInputModeler modeler;
  modeler.StartStroke(GetParam().input_model, /* brush_epsilon = */ 0.01);

  // This kind of function call is likely to never occur, but we check that the
  // `current_elapsed_time` parameter is not ignored in this case for
  // consistency of the API.
  modeler.ExtendStroke({}, {}, Duration32::Millis(10));

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kUnknown);
  EXPECT_EQ(modeler.GetState().stroke_unit_length, std::nullopt);
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, Duration32::Millis(10));
  EXPECT_THAT(modeler.GetModeledInputs(), IsEmpty());
  EXPECT_EQ(modeler.GetState().stable_input_count, 0);
  EXPECT_EQ(modeler.GetState().real_input_count, 0);
}

TEST_P(StrokeInputModelerTest, ExtendWithEmptyPredictedInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  float brush_epsilon = 0.001;
  modeler.StartStroke(GetParam().input_model, brush_epsilon);

  StrokeInputBatch synthetic_real_inputs = input_batches[0];
  ASSERT_THAT(synthetic_real_inputs.Append(input_batches[1]), IsOk());

  Duration32 current_elapsed_time = synthetic_real_inputs.Get(1).elapsed_time;
  modeler.ExtendStroke(synthetic_real_inputs, {}, current_elapsed_time);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kStylus);
  EXPECT_THAT(modeler.GetState().stroke_unit_length,
              Optional(PhysicalDistanceEq(PhysicalDistance::Centimeters(1))));
  EXPECT_THAT(modeler.GetState().complete_elapsed_time.ToSeconds(),
              FloatNear(current_elapsed_time.ToSeconds(), 0.05));

  EXPECT_GE(modeler.GetState().real_input_count,
            modeler.GetState().stable_input_count);
  EXPECT_EQ(modeler.GetModeledInputs().size(),
            modeler.GetState().real_input_count);

  EXPECT_GT(modeler.GetModeledInputs().back().traveled_distance, 0);
  EXPECT_GT(modeler.GetModeledInputs().back().elapsed_time, Duration32::Zero());
}

TEST_P(StrokeInputModelerTest, ExtendWithEmptyRealInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  float brush_epsilon = 0.01;
  modeler.StartStroke(GetParam().input_model, brush_epsilon);

  StrokeInputBatch synthetic_predicted_inputs = input_batches[0];
  ASSERT_THAT(synthetic_predicted_inputs.Append(input_batches[1]), IsOk());
  ASSERT_THAT(synthetic_predicted_inputs.Append(input_batches[2]), IsOk());

  Duration32 current_elapsed_time = Duration32::Zero();
  modeler.ExtendStroke({}, synthetic_predicted_inputs, current_elapsed_time);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kStylus);
  EXPECT_THAT(modeler.GetState().stroke_unit_length,
              Optional(PhysicalDistanceEq(PhysicalDistance::Centimeters(1))));

  Duration32 predicted_elapsed_time =
      synthetic_predicted_inputs.Last().elapsed_time;
  EXPECT_THAT(modeler.GetState().complete_elapsed_time.ToSeconds(),
              FloatNear(predicted_elapsed_time.ToSeconds(), 0.05));

  EXPECT_THAT(modeler.GetModeledInputs(), Not(IsEmpty()));
  EXPECT_EQ(modeler.GetState().stable_input_count, 0);
  EXPECT_EQ(modeler.GetState().real_input_count, 0);

  EXPECT_GT(modeler.GetModeledInputs().back().traveled_distance, 0);
  EXPECT_GT(modeler.GetModeledInputs().back().elapsed_time, Duration32::Zero());
}

TEST_P(StrokeInputModelerTest, ExtendWithBothEmptyInputsClearsPrediction) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  float brush_epsilon = 0.08;
  modeler.StartStroke(GetParam().input_model, brush_epsilon);

  Duration32 current_elapsed_time = input_batches[0].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[0], {}, current_elapsed_time);

  current_elapsed_time = input_batches[1].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[1], input_batches[4],
                       current_elapsed_time);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kStylus);
  EXPECT_THAT(modeler.GetState().stroke_unit_length,
              Optional(PhysicalDistanceEq(PhysicalDistance::Centimeters(1))));
  Duration32 predicted_elapsed_time = input_batches[4].Last().elapsed_time;
  EXPECT_THAT(modeler.GetState().complete_elapsed_time.ToSeconds(),
              FloatNear(predicted_elapsed_time.ToSeconds(), 0.05));

  EXPECT_GE(modeler.GetState().real_input_count,
            modeler.GetState().stable_input_count);
  EXPECT_GT(modeler.GetModeledInputs().size(),
            modeler.GetState().real_input_count);

  EXPECT_GT(modeler.GetModeledInputs().back().traveled_distance, 0);
  EXPECT_GT(modeler.GetModeledInputs().back().elapsed_time, Duration32::Zero());

  size_t last_stable_modeled_count = modeler.GetState().stable_input_count;

  current_elapsed_time += Duration32::Seconds(0.2);
  modeler.ExtendStroke({}, {}, current_elapsed_time);
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, current_elapsed_time);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kStylus);
  EXPECT_THAT(modeler.GetState().stroke_unit_length,
              Optional(PhysicalDistanceEq(PhysicalDistance::Centimeters(1))));

  EXPECT_EQ(modeler.GetState().stable_input_count, last_stable_modeled_count);
  EXPECT_GE(modeler.GetState().real_input_count,
            modeler.GetState().stable_input_count);
  EXPECT_EQ(modeler.GetState().real_input_count,
            modeler.GetModeledInputs().size());
}

TEST_P(StrokeInputModelerTest, ExtendKeepsRealInputAndReplacesPrediction) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  float brush_epsilon = 0.004;
  modeler.StartStroke(GetParam().input_model, brush_epsilon);

  Duration32 current_elapsed_time = input_batches[0].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[0], {}, current_elapsed_time);

  current_elapsed_time = input_batches[1].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[1], input_batches[4],
                       current_elapsed_time);

  EXPECT_GE(modeler.GetState().real_input_count,
            modeler.GetState().stable_input_count);
  EXPECT_GT(modeler.GetModeledInputs().size(),
            modeler.GetState().real_input_count);

  EXPECT_GT(modeler.GetModeledInputs().back().traveled_distance, 0);
  EXPECT_GT(modeler.GetModeledInputs().back().elapsed_time, Duration32::Zero());

  size_t last_real_modeled_count = modeler.GetState().real_input_count;
  float last_real_distance =
      modeler.GetModeledInputs()[last_real_modeled_count - 1].traveled_distance;
  Duration32 last_real_elapsed_time =
      modeler.GetModeledInputs()[last_real_modeled_count - 1].elapsed_time;
  float last_total_distance =
      modeler.GetModeledInputs().back().traveled_distance;
  Duration32 last_total_elapsed_time =
      modeler.GetModeledInputs().back().elapsed_time;

  current_elapsed_time = input_batches[2].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[2], input_batches[3],
                       current_elapsed_time);

  EXPECT_GT(modeler.GetState().real_input_count, last_real_modeled_count);
  EXPECT_GT(modeler.GetModeledInputs().size(),
            modeler.GetState().real_input_count);

  // The real traveled_distance and elapsed time of the stroke should increase,
  // but the totals should decrease as the new prediction is prior to the one
  // used for the previous extension:

  size_t real_count = modeler.GetState().real_input_count;
  EXPECT_GT(modeler.GetModeledInputs()[real_count - 1].traveled_distance,
            last_real_distance);
  EXPECT_GT(modeler.GetModeledInputs()[real_count - 1].elapsed_time,
            last_real_elapsed_time);

  EXPECT_LT(modeler.GetModeledInputs().back().traveled_distance,
            last_total_distance);
  EXPECT_LT(modeler.GetModeledInputs().back().elapsed_time,
            last_total_elapsed_time);
}

TEST_P(StrokeInputModelerTest, StartClearsAfterExtending) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  modeler.StartStroke(GetParam().input_model, /* brush_epsilon = */ 0.01);

  Duration32 current_elapsed_time = input_batches[0].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[0], {}, current_elapsed_time);

  current_elapsed_time = input_batches[1].Last().elapsed_time;
  modeler.ExtendStroke(input_batches[1], input_batches[2],
                       current_elapsed_time);

  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kStylus);
  EXPECT_THAT(modeler.GetState().stroke_unit_length,
              Optional(PhysicalDistanceEq(PhysicalDistance::Centimeters(1))));
  EXPECT_GT(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), Not(IsEmpty()));
  EXPECT_GT(modeler.GetState().real_input_count, 0);

  modeler.StartStroke(GetParam().input_model, /* brush_epsilon = */ 0.01);
  EXPECT_EQ(modeler.GetState().tool_type, StrokeInput::ToolType::kUnknown);
  EXPECT_EQ(modeler.GetState().stroke_unit_length, std::nullopt);
  EXPECT_EQ(modeler.GetState().complete_elapsed_time, Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(), IsEmpty());

  EXPECT_EQ(modeler.GetState().stable_input_count, 0);
  EXPECT_EQ(modeler.GetState().real_input_count, 0);
}

TEST_P(StrokeInputModelerTest, CumulativeDistanceTraveled) {
  StrokeInputModeler modeler;
  modeler.StartStroke(GetParam().input_model, /* brush_epsilon = */ 0.01);

  // Extend the stroke with a bunch of inputs (some real, some predicted) that
  // move at a constant velocity of 1000 stroke units per second.
  auto append_input = [](StrokeInputBatch& inputs, int i) {
    EXPECT_THAT(inputs.Append(StrokeInput{
                    .position = {static_cast<float>(i), 0},
                    .elapsed_time = Duration32::Millis(i),
                }),
                IsOk());
  };
  StrokeInputBatch real_inputs;
  for (int i = 0; i < 100; ++i) {
    append_input(real_inputs, i);
  }
  StrokeInputBatch predicted_inputs;
  for (int i = 100; i < 200; ++i) {
    append_input(predicted_inputs, i);
  }
  modeler.ExtendStroke(real_inputs, predicted_inputs, Duration32::Millis(200));

  // After these 200ms of inputs, the total modeled distance traveled should be
  // on the order of *around* 200 stroke units. Exactly how close the distance
  // is will depend on the modeler implementation, but it shouldn't be *too* far
  // off.
  EXPECT_THAT(modeler.GetState().complete_traveled_distance,
              FloatNear(200, 25));
  // Only the first 100ms of inputs were real, so the total real distance should
  // be *around* 100 stroke units (again, we'll leave a generous margin to allow
  // for different modeling strategies).
  EXPECT_THAT(modeler.GetState().total_real_distance, FloatNear(100, 25));
  // Intermediate elapsed times/distances should also be reasonable.  Different
  // modeling implementations may have different upsampling strategies, but
  // given the regularity of these test inputs, it is reasonable to assume that
  // the modeled inputs should be reasonably evenly spaced in time and space,
  // and therefore that 25% of the way through the modeled inputs, we should
  // have traveled *around* 25% of the total distance.
  size_t index_at_25_percent_progress = modeler.GetModeledInputs().size() / 4;
  const ModeledStrokeInput& input_at_25_percent_progress =
      modeler.GetModeledInputs()[index_at_25_percent_progress];
  EXPECT_THAT(input_at_25_percent_progress.traveled_distance,
              FloatNear(50, 25));
}

TEST_P(StrokeInputModelerTest, EraseInitialPredictionWithNoRealInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  modeler.StartStroke(GetParam().input_model,
                      /* brush_epsilon = */ 0.01);

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

TEST_P(StrokeInputModelerTest, ExtendWithoutStart) {
  StrokeInputModeler modeler;
  EXPECT_DEATH_IF_SUPPORTED(modeler.ExtendStroke({}, {}, Duration32::Zero()),
                            "`StartStroke\\(\\)` has not been called\\.");
}

TEST_P(StrokeInputModelerTest, StartWithZeroEpsilon) {
  StrokeInputModeler modeler;
  EXPECT_DEATH_IF_SUPPORTED(modeler.StartStroke(GetParam().input_model,
                                                /* brush_epsilon = */ 0),
                            "brush_epsilon");
}

INSTANTIATE_TEST_SUITE_P(
    TestInputModels, StrokeInputModelerTest,
    // LINT.IfChange(input_model_types)
    ::testing::ValuesIn<InputModelTestCase>({
        {"SpringModel", {BrushFamily::SpringModel{}}},
        {"NaiveModel", {BrushFamily::ExperimentalNaiveModel{}}},
        {"SlidingWindowModel_default", {BrushFamily::SlidingWindowModel{}}},
        {"SlidingWindowModel_250ms_100ms",
         {BrushFamily::SlidingWindowModel{
             .window_size = Duration32::Millis(250),
             .upsampling_period = Duration32::Millis(100)}}},
        {"SlidingWindowModel_1500ms_inf",
         {BrushFamily::SlidingWindowModel{
             .window_size = Duration32::Millis(1500),
             .upsampling_period = Duration32::Infinite()}}},
    }),
    // LINT.ThenChange(../../brush/brush_family.h:input_model_types)
    [](const ::testing::TestParamInfo<InputModelTestCase>& info) {
      return info.param.test_name;
    });

}  // namespace
}  // namespace ink::strokes_internal
