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

#include "ink/strokes/internal/stroke_input_modeler/naive_input_modeler.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status_matchers.h"
#include "ink/brush/brush_family.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/fuzz_domains.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"
#include "ink/types/type_matchers.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::AllOf;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::FloatEq;
using ::testing::FloatNear;
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

TEST(NaiveInputModelerTest, ModeledInputsMatchRawInputs) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();
  StrokeInputBatch synthetic_real_inputs;
  for (const StrokeInputBatch& input_batch : MakeStylusInputBatchSequence()) {
    ASSERT_THAT(synthetic_real_inputs.Append(input_batch), IsOk());
  }

  NaiveInputModeler modeler;
  modeler.StartStroke(/*brush_epsilon=*/0.001);
  modeler.ExtendStroke(synthetic_real_inputs, {}, Duration32::Seconds(5));

  ASSERT_THAT(modeler.GetModeledInputs(), SizeIs(synthetic_real_inputs.Size()));
  for (int i = 0; i < synthetic_real_inputs.Size(); ++i) {
    StrokeInput raw_input = synthetic_real_inputs.Get(i);
    const ModeledStrokeInput& modeled_input = modeler.GetModeledInputs()[i];
    EXPECT_THAT(modeled_input.position, PointEq(raw_input.position));
    EXPECT_THAT(modeled_input.elapsed_time,
                Duration32Eq(raw_input.elapsed_time));
    EXPECT_THAT(modeled_input.pressure, FloatEq(raw_input.pressure));
    EXPECT_THAT(modeled_input.tilt, AngleEq(raw_input.tilt));
    EXPECT_THAT(modeled_input.orientation, AngleEq(raw_input.orientation));
  }
}

void CanModelAnyStrokeInputBatch(const StrokeInputBatch& inputs) {
  NaiveInputModeler modeler;
  modeler.StartStroke(/*brush_epsilon=*/1);
  modeler.ExtendStroke(inputs, {}, Duration32::Zero());
  // The `NaiveInputModeler` always produces exactly one modeled input for each
  // raw input.
  EXPECT_EQ(modeler.GetModeledInputs().size(), inputs.Size());
  // Since there were no predicted inputs, all modeled inputs should be real.
  EXPECT_EQ(modeler.GetState().real_input_count,
            modeler.GetModeledInputs().size());
  // For the `NaiveInputModeler`, all real modeled inputs are always stable.
  EXPECT_EQ(modeler.GetState().stable_input_count,
            modeler.GetState().real_input_count);
}
FUZZ_TEST(NaiveInputModelerTest, CanModelAnyStrokeInputBatch)
    .WithDomains(ArbitraryStrokeInputBatch());

}  // namespace
}  // namespace ink::strokes_internal
