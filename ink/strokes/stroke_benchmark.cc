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

#include <utility>
#include <vector>

#include "benchmark/benchmark.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/color/color.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/recorded_test_inputs.h"
#include "ink/strokes/stroke.h"

namespace ink {
namespace {

using ::benchmark::internal::Benchmark;
constexpr float kBrushEpsilon = 0.01;

Brush MakeCircleBrush(float brush_size, float brush_epsilon) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.scale = {1, 1}, .corner_rounding = 1}, BrushPaint{});
  ABSL_CHECK_OK(family);
  absl::StatusOr<Brush> brush = Brush::Create(
      *std::move(family), Color::Black(), brush_size, brush_epsilon);
  ABSL_CHECK_OK(brush);
  return *std::move(brush);
}

// TODO(b/374775850): Include test cases for stock brushes.
void BenchmarkTestCases(Benchmark* b) {
  std::vector<int> brush_sizes;
  int num_test_files = kTestDataFiles.size();
  for (int test_file = 0; test_file < num_test_files; ++test_file) {
    for (int brush_size = 1; brush_size <= 32; brush_size *= 2) {
      b->Args({brush_size, test_file});
    }
  }
}

void BM_Stroke(benchmark::State& state) {
  const float brush_size = state.range(0);
  auto brush = MakeCircleBrush(brush_size, kBrushEpsilon);

  absl::string_view test_inputs_name = kTestDataFiles[state.range(1)];
  auto inputs = LoadCompleteStrokeInputs(test_inputs_name);
  ABSL_CHECK_OK(inputs);

  state.SetLabel(absl::StrFormat("stroke: %s, brush size: %f", test_inputs_name,
                                 brush_size));

  for (auto s : state) {
    Stroke stroke(brush, *inputs);
    benchmark::DoNotOptimize(stroke);
  }
}
BENCHMARK(BM_Stroke)->Apply(BenchmarkTestCases);

void BM_InProgressStroke(benchmark::State& state) {
  const float brush_size = state.range(0);
  auto brush = MakeCircleBrush(brush_size, kBrushEpsilon);

  absl::string_view test_inputs_name = kTestDataFiles[state.range(1)];
  auto inputs = LoadIncrementalStrokeInputs(test_inputs_name);
  ABSL_CHECK_OK(inputs);

  state.SetLabel(absl::StrFormat("stroke: %s, brush size: %f", test_inputs_name,
                                 brush_size));

  for (auto s : state) {
    InProgressStroke stroke;
    stroke.Start(brush);
    for (const auto& [real, predicted] : *inputs) {
      ABSL_CHECK_OK(stroke.EnqueueInputs(real, predicted));
      if (!real.IsEmpty()) {
        ABSL_CHECK_OK(stroke.UpdateShape(real.Last().elapsed_time));
      }
      benchmark::DoNotOptimize(stroke);
    }
  }
}
BENCHMARK(BM_InProgressStroke)->Apply(BenchmarkTestCases);

}  // namespace
}  // namespace ink
