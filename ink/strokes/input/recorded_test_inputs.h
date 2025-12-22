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

#ifndef INK_STROKES_INPUT_RECORDED_TEST_INPUTS_H_
#define INK_STROKES_INPUT_RECORDED_TEST_INPUTS_H_

#include <optional>
#include <utility>
#include <vector>

#include "ink/geometry/rect.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink {

constexpr std::array<absl::string_view, 2> kTestDataFiles = {
    "spring_shape.binarypb", "straight_line.binarypb"};

// Returns incremental inputs loaded from the given `IncrementalStrokeInputs`
// binary proto test file, rescaled to fit to `bounds` if provided. Each
// incremental input consists of a a pair of real and predicted
// `StrokeInputBatch`.
absl::StatusOr<std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>>
LoadIncrementalStrokeInputs(absl::string_view filename,
                            std::optional<Rect> bounds = std::nullopt);

// Returns a complete `StrokeInputBatch` loaded from the given
// `IncrementalStrokeInputs` binary proto test file, obtained by coalescing the
// incremental real inputs and rescaled to fit to`bounds` if provided.
absl::StatusOr<StrokeInputBatch> LoadCompleteStrokeInputs(
    absl::string_view filename, std::optional<Rect> bounds = std::nullopt);
}  // namespace ink

#endif  // INK_STROKES_INPUT_RECORDED_TEST_INPUTS_H_
