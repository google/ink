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

#include <array>
#include <utility>

#include "benchmark/benchmark.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_family.h"
#include "ink/strokes/input/recorded_test_inputs.h"
#include "ink/strokes/internal/stroke_input_modeler.h"

namespace ink::strokes_internal {
namespace {

using ::benchmark::internal::Benchmark;

// LINT.IfChange(input_model_types)
const std::array<absl::string_view, 3> input_model_names = {
    "SpringModel", "NaiveModel", "SlidingWindowModel"};
const std::array<BrushFamily::InputModel, 3> input_models = {
    BrushFamily::SpringModel{}, BrushFamily::ExperimentalNaiveModel{},
    BrushFamily::SlidingWindowModel{}};
// LINT.ThenChange(../../brush/brush_family.h:input_model_types)

void TestCases(Benchmark* b) {
  int num_test_inputs = kTestDataFiles.size();
  int num_input_models = input_models.size();
  for (int i = 0; i < num_test_inputs; ++i) {
    for (int j = 0; j < num_input_models; ++j) {
      b->Args({i, j});
    }
  }
}

void BM_IncrementalStrokeInputModeler(benchmark::State& state) {
  absl::string_view test_input_name = kTestDataFiles[state.range(0)];
  absl::string_view input_model_name = input_model_names[state.range(1)];
  const BrushFamily::InputModel& input_model = input_models[state.range(1)];

  auto inputs = LoadIncrementalStrokeInputs(test_input_name);
  ABSL_CHECK_OK(inputs);

  state.SetLabel(absl::StrFormat("stroke: %s, model: %s", test_input_name,
                                 input_model_name));

  for (auto s : state) {
    StrokeInputModeler input_modeler;
    input_modeler.StartStroke(input_model, kTestBrushEpsilon);
    for (const auto& [real, predicted] : *inputs) {
      input_modeler.ExtendStroke(real, predicted,
                                 real.IsEmpty() ? predicted.First().elapsed_time
                                                : real.Last().elapsed_time);
      benchmark::DoNotOptimize(input_modeler);
    }
  }
}
BENCHMARK(BM_IncrementalStrokeInputModeler)->Apply(TestCases);

void BM_CompleteStrokeInputModeler(benchmark::State& state) {
  absl::string_view test_input_name = kTestDataFiles[state.range(0)];
  absl::string_view input_model_name = input_model_names[state.range(1)];
  const BrushFamily::InputModel& input_model = input_models[state.range(1)];

  auto inputs = LoadCompleteStrokeInputs(test_input_name);
  ABSL_CHECK_OK(inputs);

  state.SetLabel(absl::StrFormat("stroke: %s, model: %s", test_input_name,
                                 input_model_name));

  for (auto s : state) {
    StrokeInputModeler input_modeler;
    input_modeler.StartStroke(input_model, kTestBrushEpsilon);
    input_modeler.ExtendStroke(*inputs, {}, inputs->Last().elapsed_time);
    benchmark::DoNotOptimize(input_modeler);
  }
}
BENCHMARK(BM_CompleteStrokeInputModeler)->Apply(TestCases);

}  // namespace
}  // namespace ink::strokes_internal
