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

#include "ink/strokes/input/synthetic_test_inputs.h"

#include <cmath>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/rect.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink {

StrokeInputBatch MakeCompleteLissajousCurveInputs(
    const Rect& bounds, const SyntheticInputOptions& options) {
  auto wave_function = [](float min, float max, float progress,
                          float frequency) {
    return 0.5f * (min + max) +
           0.5f * (max - min) * Cos(frequency * kHalfTurn * progress);
  };

  constexpr float kXFrequency = 7;
  constexpr float kYFrequency = 9;

  std::vector<StrokeInput> inputs;
  for (int i = 0; i < options.input_count; ++i) {
    float progress = i / (options.input_count - 1.f);
    float x =
        wave_function(bounds.XMin(), bounds.XMax(), progress, kXFrequency);
    float y =
        wave_function(bounds.YMin(), bounds.YMax(), progress, kYFrequency);
    StrokeInput input = {
        .tool_type = options.tool_type,
        .position = {x, y},
        .elapsed_time = progress * options.full_stroke_duration,
        .stroke_unit_length = options.stroke_unit_length,
    };

    // Make pressure roughly proportional to speed.
    if (options.include_pressure) {
      input.pressure = std::clamp(Vec{Sin(kXFrequency * kHalfTurn * progress),
                                      Sin(kYFrequency * kHalfTurn * progress)}
                                          .Magnitude() /
                                      std::sqrt(2.0f),
                                  0.0f, 1.0f);
    }

    // Define a fixed 3D location for the hand holding the stylus.
    float hand_x = std::lerp(bounds.XMin(), bounds.XMax(), 0.75f);
    float hand_y = std::lerp(bounds.YMin(), bounds.YMax(), 0.75f);
    float hand_z = 0.5f * (bounds.XMax() - bounds.XMin());

    // Calculate orientation and tilt, assuming that the stylus points from the
    // hand position to the input position.
    Vec delta_xy = {hand_x - x, hand_y - y};
    if (options.include_orientation) {
      input.orientation = delta_xy.Direction().Normalized();
    }
    Vec delta_zd = {hand_z, delta_xy.Magnitude()};
    if (options.include_tilt) {
      input.tilt = delta_zd.Direction();
    }

    // For barrel twist, imagine that the stylus barrel has a flat side, and
    // that the twist angle is considered zero whenever the flat side faces
    // directly away from the drawing surface (i.e. in the positive-z
    // direction).  Calculate the barrel twist angle that would make that flat
    // side instead be facing directly towards the negative-y direction.
    if (options.include_barrel_twist) {
      float sine_of_tilt = hand_z / delta_zd.Magnitude();
      input.barrel_twist =
          Vec{sine_of_tilt * delta_xy.y, delta_xy.x}.Direction().Normalized();
    }

    inputs.push_back(input);
  }

  auto input_batch = StrokeInputBatch::Create(inputs);
  ABSL_CHECK_OK(input_batch);
  return *std::move(input_batch);
}

}  // namespace ink
