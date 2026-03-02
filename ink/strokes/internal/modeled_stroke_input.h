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

#ifndef INK_STROKES_INTERNAL_MODELED_STROKE_INPUT_H_
#define INK_STROKES_INTERNAL_MODELED_STROKE_INPUT_H_

#include <cstddef>
#include <memory>
#include <optional>

#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink::strokes_internal {

// The result of modeling `StrokeInput` by the `StrokeInputModeler`.
//
// The `velocity` and `acceleration` are the modeled instantaneous
// velocity/acceleration of the input, and `traveled_distance` is the modeled
// total distance traveled since the start of the stroke. The other members are
// the modeled analogues of member variables of `StrokeInput` with the same
// names.
struct ModeledStrokeInput {
  Point position;
  Vec velocity;
  Vec acceleration;
  float traveled_distance = 0;
  Duration32 elapsed_time;
  float pressure = StrokeInput::kNoPressure;
  Angle tilt = StrokeInput::kNoTilt;
  Angle orientation = StrokeInput::kNoOrientation;
};

// Computes the linear interpolation between `a` and `b` when `t` is in the
// range [0, 1], and the linear extrapolation otherwise.
ModeledStrokeInput Lerp(const ModeledStrokeInput& a,
                        const ModeledStrokeInput& b, float t);

// Measures the input distance/time from one point on the stroke input to
// another.
struct InputMetrics {
  // The distance traveled by the input pointer, in stroke space units.
  float traveled_distance = 0;
  // The input time elapsed.
  Duration32 elapsed_time;
};

// The current state of modeling all `StrokeInput`s so far for a stroke by an
// input modeler.
struct InputModelerState {
  // The current tool type of the stroke.
  //
  // When the current stroke has no inputs, the return value is `kUnknown`.
  StrokeInput::ToolType tool_type = StrokeInput::ToolType::kUnknown;
  // The physical distance that the pointer must travel in order to produce an
  // input motion of one stroke unit. For stylus/touch, this is the real-world
  // distance that the stylus/fingertip must move in physical space; for mouse,
  // this is the visual distance that the mouse pointer must travel along the
  // surface of the display.
  //
  // A value of `std::nullopt` indicates that the relationship between stroke
  // space and physical space is unknown (possibly because the current stroke
  // has no inputs yet) or ill-defined.
  std::optional<PhysicalDistance> stroke_unit_length;
  // The total modeled distance/time from the start of the stroke until the last
  // "real" (i.e. non-predicted) modeled input so far.
  InputMetrics real_input_metrics;
  // The total modeled distance/time from the start of the stroke until the last
  // modeled input so far (including unstable/predicted modeled inputs).
  InputMetrics full_input_metrics;
  // The modeled time elapsed from the start of the stroke until either "now" or
  // the last modeled input, whichever comes later.
  //
  // This value may be different from the `current_elapsed_time` value passed to
  // `ExtendStroke()` due to modeling and prediction. This value will always be
  // greater than or equal to `full_input_metrics.elapsed_time`.
  Duration32 complete_elapsed_time = Duration32::Zero();
  // The number of "stable" elements at the start of `GetModeledInputs()`.
  //
  // These will not be removed or modified by subsequent calls to
  // `ExtendStroke()`, which means the values of this member variable over the
  // course of a single stroke will be non-decreasing.
  size_t stable_input_count = 0;
  // The number of elements at the start of `GetModeledInputs()` that were a
  // result of modeling only the "real" (i.e. non-predicted) inputs.
  //
  // This number will always be greater than or equal to the value of
  // `stable_input_count`. As with the stable count, the values of this member
  // variable will be non-decreasing over the course of a single stroke.
  size_t real_input_count = 0;
  // True if inputs are finished for this stroke (in which case, no more real or
  // predicted inputs will be added).
  bool inputs_are_finished = false;
};

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_MODELED_STROKE_INPUT_H_
