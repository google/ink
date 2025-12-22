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

#include "ink/strokes/input/recorded_test_inputs.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;

Envelope GetEnvelope(const StrokeInputBatch& inputs) {
  Envelope envelope;
  for (const StrokeInput& input : inputs) {
    envelope.Add(input.position);
  }
  return envelope;
}

Envelope GetEnvelope(
    const std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& input) {
  Envelope envelope;
  for (const auto& [real, predicted] : input) {
    envelope.Add(GetEnvelope(real));
    envelope.Add(GetEnvelope(predicted));
  }
  return envelope;
}

TEST(RecordedTestInputsTest, IncrementalInputsHasPrediction) {
  for (const auto& filename : kTestDataFiles) {
    auto incremental_inputs = LoadIncrementalStrokeInputs(filename);
    EXPECT_THAT(incremental_inputs, IsOk());

    bool has_predicted_inputs = false;
    for (const auto& [real, predicted] : *incremental_inputs) {
      if (predicted.Size() > 0) {
        has_predicted_inputs = true;
        break;
      }
    }
    EXPECT_TRUE(has_predicted_inputs);
  }
}

TEST(RecordedTestInputsTest, IncrementalInputsRespectsBounds) {
  for (const auto& filename : kTestDataFiles) {
    Rect bounds = Rect::FromTwoPoints({0, 1}, {2, 3});
    auto incremental_inputs = LoadIncrementalStrokeInputs(filename, bounds);
    EXPECT_THAT(incremental_inputs, IsOk());
    auto envelope = GetEnvelope(*incremental_inputs);
    EXPECT_THAT(envelope, EnvelopeNear(bounds, 0.001f));
  }
}

TEST(RecordedTestInputsTest, CompleteInputsWorks) {
  for (const auto& filename : kTestDataFiles) {
    auto incremental_inputs = LoadIncrementalStrokeInputs(filename);
    EXPECT_THAT(incremental_inputs, IsOk());
    auto complete_inputs = LoadCompleteStrokeInputs(filename);
    EXPECT_THAT(complete_inputs, IsOk());

    size_t points = 0;
    for (const auto& [real, predicted] : *incremental_inputs) {
      points += real.Size();
    }

    EXPECT_EQ(complete_inputs->Size(), points);
  }
}

TEST(RecordedTestInputsTest, CompleteInputsRespectsBounds) {
  for (const auto& filename : kTestDataFiles) {
    Rect bounds = Rect::FromTwoPoints({0, 1}, {2, 3});
    auto complete_inputs = LoadCompleteStrokeInputs(filename, bounds);
    EXPECT_THAT(complete_inputs, IsOk());
    auto envelope = GetEnvelope(*complete_inputs);
    EXPECT_THAT(envelope, EnvelopeNear(bounds, 0.001f));
  }
}

}  // namespace
}  // namespace ink
