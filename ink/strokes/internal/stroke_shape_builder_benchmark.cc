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
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/stroke_shape_builder.h"

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

void BenchmarkTestCases(Benchmark* b) {
  std::vector<int> brush_sizes;
  int num_test_files = kTestDataFiles.size();
  std::vector<stock_brushes::StockBrushesTestParam> stock_brushes_test_params =
      stock_brushes::GetParams();
  // TODO: b/374775850 - Add test cases for unique brushes that test individual
  // parts of the family structure.
  for (int test_file = 0; test_file < num_test_files; ++test_file) {
    for (int brush_size = 1; brush_size <= 32; brush_size *= 2) {
      for (size_t brush = 0; brush < stock_brushes_test_params.size();
           brush++) {
        b->Args({brush_size, test_file, static_cast<int>(brush)});
      }
    }
  }
}

void BM_BuildStrokeShape(benchmark::State& state) {
  const float brush_size = state.range(0);
  absl::string_view test_inputs_name = kTestDataFiles[state.range(1)];
  const BrushFamily brush_family =
      stock_brushes::GetParams()[state.range(2)].second;

  state.SetLabel(absl::StrFormat(
      "stroke: %s, brush size: %f, brush: %s", test_inputs_name, brush_size,
      stock_brushes::GetParams()[state.range(2)].first));

  auto brush = MakeBrush(brush_family, brush_size, kTestBrushEpsilon);

  auto raw_inputs = LoadCompleteStrokeInputs(test_inputs_name);
  ABSL_CHECK_OK(raw_inputs);
  StrokeInputModeler input_modeler;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(),
                            brush.GetEpsilon());
  input_modeler.ExtendStroke(*raw_inputs, {}, raw_inputs->Last().elapsed_time);

  for (auto s : state) {
    std::vector<StrokeShapeBuilder> builders(brush.CoatCount());
    for (size_t i = 0; i < brush.CoatCount(); ++i) {
      builders[i].StartStroke(brush.GetCoats()[i], brush.GetSize(),
                              brush.GetEpsilon());
      builders[i].ExtendStroke(input_modeler);
    }
  }
}
BENCHMARK(BM_BuildStrokeShape)->Apply(BenchmarkTestCases);

// Helper function that returns a sequence of `StrokeInputModeler`s that
// represent the incremental states of the stroke input model, as it processes
// the sequence of raw inputs.
std::vector<StrokeInputModeler> GetIncrementalInputModelers(
    std::vector<std::pair<StrokeInputBatch, StrokeInputBatch>>& inputs) {
  std::vector<StrokeInputModeler> modelers(inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    modelers[i].StartStroke(BrushFamily::DefaultInputModel(),
                            kTestBrushEpsilon);
    for (size_t j = 0; j <= i; ++j) {
      const StrokeInputBatch& real = inputs[j].first;
      const StrokeInputBatch& predicted = inputs[j].second;
      modelers[i].ExtendStroke(real, predicted,
                               real.IsEmpty() ? predicted.Last().elapsed_time
                                              : real.Last().elapsed_time);
    }
  }

  return modelers;
}

void BM_BuildStrokeShapeIncrementally(benchmark::State& state) {
  const float brush_size = state.range(0);
  absl::string_view test_inputs_name = kTestDataFiles[state.range(1)];
  const BrushFamily brush_family =
      stock_brushes::GetParams()[state.range(2)].second;

  state.SetLabel(absl::StrFormat(
      "stroke: %s, brush size: %f, brush: %s", test_inputs_name, brush_size,
      stock_brushes::GetParams()[state.range(2)].first));

  auto brush = MakeBrush(brush_family, brush_size, kTestBrushEpsilon);

  auto raw_inputs = LoadIncrementalStrokeInputs(test_inputs_name);
  ABSL_CHECK_OK(raw_inputs);

  // Model all the inputs once-for-all ahead of time, so the benchmark
  // measures purely the runtime of building the shape.
  StrokeInputModeler input_modeler;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(),
                            brush.GetEpsilon());
  auto modeler_sequence = GetIncrementalInputModelers(*raw_inputs);

  for (auto s : state) {
    std::vector<StrokeShapeBuilder> builders(brush.CoatCount());
    for (size_t i = 0; i < brush.CoatCount(); ++i) {
      builders[i].StartStroke(brush.GetCoats()[i], brush.GetSize(),
                              brush.GetEpsilon());
    }

    for (const StrokeInputModeler& input_modeler : modeler_sequence) {
      for (size_t i = 0; i < brush.CoatCount(); ++i) {
        builders[i].ExtendStroke(input_modeler);
      }
    }
  }
}
BENCHMARK(BM_BuildStrokeShapeIncrementally)->Apply(BenchmarkTestCases);

}  // namespace
}  // namespace ink::strokes_internal
