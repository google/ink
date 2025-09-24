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

#include <memory>
#include <variant>

#include "absl/base/nullability.h"
#include "ink/brush/brush_family.h"
#include "ink/strokes/internal/stroke_input_modeler/naive_input_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler/sliding_window_input_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler/spring_based_input_modeler.h"

namespace ink::strokes_internal {
namespace {

absl_nonnull std::unique_ptr<StrokeInputModeler> CreateInputModeler(
    const BrushFamily::SpringModel& spring_model) {
  return std::make_unique<SpringBasedInputModeler>(
      SpringBasedInputModeler::Version::kSpringModel);
}

absl_nonnull std::unique_ptr<StrokeInputModeler> CreateInputModeler(
    const BrushFamily::ExperimentalRawPositionModel& raw_position_model) {
  return std::make_unique<SpringBasedInputModeler>(
      SpringBasedInputModeler::Version::kExperimentalRawPositionModel);
}

absl_nonnull std::unique_ptr<StrokeInputModeler> CreateInputModeler(
    const BrushFamily::ExperimentalNaiveModel& naive_model) {
  return std::make_unique<NaiveInputModeler>();
}

absl_nonnull std::unique_ptr<StrokeInputModeler> CreateInputModeler(
    const BrushFamily::ExperimentalSlidingWindowModel& sliding_window_model) {
  return std::make_unique<SlidingWindowInputModeler>(
      sliding_window_model.window_size);
}

}  // namespace

absl_nonnull std::unique_ptr<StrokeInputModeler> StrokeInputModeler::Create(
    const BrushFamily::InputModel& input_model) {
  return std::visit([](auto& model) { return CreateInputModeler(model); },
                    input_model);
}

}  // namespace ink::strokes_internal
