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

#include "ink/strokes/internal/stroke_input_modeler/spring_based_input_modeler.h"

#include <cstddef>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "ink/brush/brush_family.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/type_matchers.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"
#include "ink/types/type_matchers.h"

namespace ink::strokes_internal {
namespace {

using ::testing::ElementsAre;

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

MATCHER_P(PositionsAreSeparatedByAtLeast, min_distance, "") {
  for (size_t i = 1; i < arg.size(); ++i) {
    float distance = (arg[i].position - arg[i - 1].position).Magnitude();
    if (distance < min_distance) {
      *result_listener << absl::StrFormat(
          "Inputs at indices %d and %d, with positions %v and %v, are "
          "separated by %f",
          i - 1, i, arg[i - 1].position, arg[i].position, distance);
      return false;
    }
  }
  return true;
}

TEST(SpringBasedInputModelerTest, LargeBrushEpsilonIsRespected) {
  std::vector<StrokeInputBatch> input_batches = MakeStylusInputBatchSequence();

  StrokeInputModeler modeler;
  float brush_epsilon = 3;
  modeler.StartStroke(BrushFamily::SpringModel{}, brush_epsilon);

  modeler.ExtendStroke(input_batches[0], input_batches[1], Duration32::Zero());
  modeler.ExtendStroke(input_batches[1], input_batches[2], Duration32::Zero());
  modeler.ExtendStroke(input_batches[2], input_batches[3], Duration32::Zero());
  modeler.ExtendStroke(input_batches[3], input_batches[4], Duration32::Zero());
  EXPECT_THAT(modeler.GetModeledInputs(),
              PositionsAreSeparatedByAtLeast(brush_epsilon));
}

}  // namespace
}  // namespace ink::strokes_internal
