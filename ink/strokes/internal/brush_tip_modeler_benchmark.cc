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

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "benchmark/benchmark.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/stock_brushes_test_params.h"
#include "ink/color/color.h"
#include "ink/strokes/input/recorded_test_inputs.h"
#include "ink/strokes/internal/brush_tip_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler.h"

namespace ink::strokes_internal {
namespace {

using ::benchmark::internal::Benchmark;

Brush MakeBrush(const BrushFamily& family, float brush_size,
                float brush_epsilon) {
  absl::StatusOr<Brush> brush =
      Brush::Create(family, Color::Black(), brush_size, brush_epsilon);
  ABSL_CHECK_OK(brush);
  return *std::move(brush);
}

void TestCases(Benchmark* b) {
  int num_test_files = kTestDataFiles.size();
  std::vector<std::pair<std::string, BrushFamily>> stock_brushes_test_params =
      stock_brushes::GetParams();
  for (int test_file_idx = 0; test_file_idx < num_test_files; ++test_file_idx) {
    for (int brush_size = 1; brush_size <= 32; brush_size *= 2) {
      for (size_t brush_idx = 0; brush_idx < stock_brushes_test_params.size();
           ++brush_idx) {
        b->Args({test_file_idx, brush_size, static_cast<int>(brush_idx)});
      }
    }
  }
}

void BM_BrushTipModeler(benchmark::State& state) {
  absl::string_view test_input_name = kTestDataFiles[state.range(0)];
  const float brush_size = state.range(1);
  const BrushFamily brush_family =
      stock_brushes::GetParams()[state.range(2)].second;
  const Brush brush = MakeBrush(brush_family, brush_size, kTestBrushEpsilon);

  auto inputs = LoadCompleteStrokeInputs(test_input_name);
  ABSL_CHECK_OK(inputs);

  state.SetLabel(absl::StrFormat(
      "stroke: %s, brush size: %f, brush: %s", test_input_name, brush_size,
      stock_brushes::GetParams()[state.range(2)].first));

  StrokeInputModeler input_modeler;
  input_modeler.StartStroke(brush_family.GetInputModel(), kTestBrushEpsilon);
  input_modeler.ExtendStroke(*inputs, {}, inputs->Last().elapsed_time);

  for (auto s : state) {
    std::vector<BrushTipModeler> modelers(brush.CoatCount());
    for (size_t i = 0; i < brush.CoatCount(); ++i) {
      modelers[i].StartStroke(&brush.GetCoats().at(0).tip, brush_size);
      modelers[i].UpdateStroke(input_modeler.GetState(),
                               input_modeler.GetModeledInputs());
    }
  }
}
BENCHMARK(BM_BrushTipModeler)->Apply(TestCases);

}  // namespace
}  // namespace ink::strokes_internal
