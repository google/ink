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

#ifndef INK_STROKES_INPUT_SYNTHETIC_TEST_INPUTS_H_
#define INK_STROKES_INPUT_SYNTHETIC_TEST_INPUTS_H_

#include "ink/geometry/rect.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink {

struct SyntheticInputOptions {
  Duration32 full_stroke_duration = Duration32::Seconds(1);
  // TODO: b/314950788 - Remove `input_count` once upsampling is implemented.
  int input_count = 180;
  StrokeInput::ToolType tool_type = StrokeInput::ToolType::kUnknown;
  PhysicalDistance stroke_unit_length = StrokeInput::kNoStrokeUnitLength;
  bool include_pressure = false;
  bool include_tilt = false;
  bool include_orientation = false;
  bool include_barrel_twist = false;
};

// Returns a single input batch for a complete Lissajous curve stroke. See
// https://en.wikipedia.org/wiki/Lissajous_curve.
StrokeInputBatch MakeCompleteLissajousCurveInputs(
    const Rect& bounds, const SyntheticInputOptions& options = {});

}  // namespace ink

#endif  // INK_STROKES_INPUT_SYNTHETIC_TEST_INPUTS_H_
