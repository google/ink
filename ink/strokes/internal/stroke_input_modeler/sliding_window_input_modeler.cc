// Copyright 2025 Google LLC
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

#include "ink/strokes/internal/stroke_input_modeler/sliding_window_input_modeler.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/distance.h"
#include "ink/geometry/internal/lerp.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {
namespace {

/// When upsampling, don't subdivide the gap between raw inputs into more than
/// this many segments. The value chosen here is mostly arbitrary, but should be
/// (1) high enough that we shouldn't get anywhere near this limit in typical
/// cases (e.g. upsampling 30 Hz input to 180 Hz requires only 6 divisions), and
/// (2) low enough to prevent memory usage from completely exploding in unusual
/// cases (e.g. long gaps between two inputs due to, say, the system clock
/// updating in the middle of a stroke).
constexpr int kMaxUpsampleDivisions = 100;

// Holds time-weighted sums of `StrokeInput` fields, for computing average
// values over a window of time.
struct StrokeInputIntegrals {
  Vec position_dt;
  float pressure_dt = 0;
  Angle tilt_dt;
  // For orientation, we ultimately want a circular mean [1] of the inputs being
  // averaged together. So rather than summing up orientation angles, we sum up
  // unit vectors in those directions. At the end, we'll divide by time and take
  // the direction of the resulting vector as our average orientation direction.
  //
  // [1] See https://en.wikipedia.org/wiki/Circular_mean
  Vec orientation_dt;
};

// Integrates each of position, pressure, tilt, and orientation over the elapsed
// time between the two inputs, assuming that each of those quantities vary
// linearly between the two inputs, and add the totals to `integrals`.
void Integrate(StrokeInputIntegrals& integrals, const StrokeInput& input1,
               const StrokeInput& input2) {
  ABSL_DCHECK_LE(input1.elapsed_time, input2.elapsed_time);
  float dt = (input2.elapsed_time - input1.elapsed_time).ToSeconds();
  // For each of position/pressure/tilt/orientation, we are computing the
  // integral with respect to time of the value as it changes from `input1` to
  // `input2`. In the absence of better information, we just assume this change
  // is linear. Therefore, we are effectively computing the area of a trapezoid
  // with width `dt` and side heights `input1.foo` and `input2.foo`. That area
  // is equal to the width (that is, `dt`) times the average of the two side
  // heights. See https://en.wikipedia.org/wiki/Trapezoidal_rule.
  integrals.position_dt +=
      dt * 0.5 * (input1.position.Offset() + input2.position.Offset());
  if (input1.HasPressure()) {
    ABSL_DCHECK(input2.HasPressure());
    integrals.pressure_dt += dt * 0.5 * (input1.pressure + input2.pressure);
  }
  if (input1.HasTilt()) {
    ABSL_DCHECK(input2.HasTilt());
    integrals.tilt_dt += dt * 0.5 * (input1.tilt + input2.tilt);
  }
  if (input1.HasOrientation()) {
    ABSL_DCHECK(input2.HasOrientation());
    integrals.orientation_dt += dt * 0.5 *
                                (Vec::UnitVecWithDirection(input1.orientation) +
                                 Vec::UnitVecWithDirection(input2.orientation));
  }
}

// Given two consecutive stroke inputs and a timestamp that falls between them,
// produces an interpolated stroke input.
StrokeInput InterpolateStrokeInput(const StrokeInput& input1,
                                   const StrokeInput& input2,
                                   Duration32 elapsed_time) {
  ABSL_DCHECK_LE(input1.elapsed_time, elapsed_time);
  ABSL_DCHECK_LE(elapsed_time, input2.elapsed_time);
  float lerp_ratio = geometry_internal::InverseLerp(
      input1.elapsed_time.ToSeconds(), input2.elapsed_time.ToSeconds(),
      elapsed_time.ToSeconds());
  StrokeInput interpolated = {
      .tool_type = input1.tool_type,
      .position =
          geometry_internal::Lerp(input1.position, input2.position, lerp_ratio),
      .elapsed_time = elapsed_time,
      .stroke_unit_length = input1.stroke_unit_length,
  };
  if (input1.HasPressure() && input2.HasPressure()) {
    interpolated.pressure =
        geometry_internal::Lerp(input1.pressure, input2.pressure, lerp_ratio);
  }
  if (input1.HasTilt() && input2.HasTilt()) {
    interpolated.tilt =
        geometry_internal::Lerp(input1.tilt, input2.tilt, lerp_ratio);
  }
  if (input1.HasOrientation() && input2.HasOrientation()) {
    interpolated.orientation = geometry_internal::NormalizedAngleLerp(
        input1.orientation, input2.orientation, lerp_ratio);
  }
  return interpolated;
}

// Given two consecutive modeled stroke inputs and a timestamp that falls
// between them, produces an interpolated field value.
template <typename Value>
Value InterpolateModeledValue(Value ModeledStrokeInput::* value_field,
                              const ModeledStrokeInput& input1,
                              const ModeledStrokeInput& input2,
                              Duration32 elapsed_time) {
  ABSL_DCHECK_LE(input1.elapsed_time, elapsed_time);
  ABSL_DCHECK_LE(elapsed_time, input2.elapsed_time);
  float lerp_ratio = geometry_internal::InverseLerp(
      input1.elapsed_time.ToSeconds(), input2.elapsed_time.ToSeconds(),
      elapsed_time.ToSeconds());
  return geometry_internal::Lerp(input1.*value_field, input2.*value_field,
                                 lerp_ratio);
}

// Compute `derivative_field` for each unstable input in `modeled_inputs` by
// computing the average rate of change of the `value_field` over the
// sliding window size.
template <typename Value>
void ComputeDerivativeForUnstableInputs(
    Value ModeledStrokeInput::* value_field,
    Vec ModeledStrokeInput::* derivative_field,
    std::vector<ModeledStrokeInput>& modeled_inputs, int stable_input_count,
    Duration32 half_window_size) {
  int num_modeled_inputs = modeled_inputs.size();
  // As we iterate through `modeled_inputs`, keep track of the indices of the
  // inputs at or just before/after the edges of the sliding window.  We will
  // march these start/end indices forward as we iterate (in order to keep this
  // loop O(n)).
  int start_index = 0;
  int end_index = stable_input_count;
  for (int index = stable_input_count; index < num_modeled_inputs; ++index) {
    ModeledStrokeInput& input = modeled_inputs[index];
    // Timestamps for the start and end of the sliding window for this input
    // (clamped to the start and end of all modeled inputs so far):
    Duration32 start_time = std::max(input.elapsed_time - half_window_size,
                                     modeled_inputs.front().elapsed_time);
    Duration32 end_time = std::min(input.elapsed_time + half_window_size,
                                   modeled_inputs.back().elapsed_time);
    ABSL_DCHECK_LE(start_time, end_time);
    // If the sliding window around this input has zero duration (e.g. because
    // this is the only input so far), then just treat the derivative as zero
    // for now (until we get more inputs later).
    float dt = (end_time - start_time).ToSeconds();
    if (dt == 0) {
      input.*derivative_field = {0, 0};
      continue;
    }
    // March `start_index` forward until it is at or just before `start_time`.
    while (start_index + 1 < num_modeled_inputs &&
           modeled_inputs[start_index + 1].elapsed_time <= start_time) {
      ++start_index;
    }
    // March `end_index` forward until it is at or just after `end_time`.
    while (end_index + 1 < num_modeled_inputs &&
           modeled_inputs[end_index].elapsed_time <= end_time) {
      ++end_index;
    }
    // Compute (interpolating as needed) the value of `value_field` at the start
    // and end of the sliding window around this input.
    Value start_value = start_index + 1 < num_modeled_inputs
                            ? InterpolateModeledValue(
                                  value_field, modeled_inputs[start_index],
                                  modeled_inputs[start_index + 1], start_time)
                            : modeled_inputs[start_index].*value_field;
    Value end_value = end_index > 0
                          ? InterpolateModeledValue(
                                value_field, modeled_inputs[end_index - 1],
                                modeled_inputs[end_index], end_time)
                          : modeled_inputs[end_index].*value_field;
    // Compute the average rate of change during the sliding window.
    input.*derivative_field = (end_value - start_value) / dt;
  }
}

float DistanceTraveled(const std::vector<ModeledStrokeInput>& modeled_inputs,
                       Point position) {
  if (modeled_inputs.empty()) {
    return 0;
  }
  const ModeledStrokeInput& last_input = modeled_inputs.back();
  return last_input.traveled_distance + Distance(last_input.position, position);
}

}  // namespace

SlidingWindowInputModeler::SlidingWindowInputModeler(
    Duration32 window_size, Duration32 upsampling_period,
    float position_epsilon)
    : half_window_size_(window_size * 0.5),
      upsampling_period_(upsampling_period),
      position_epsilon_(position_epsilon) {
  ABSL_DCHECK_GE(window_size, Duration32::Zero());
  ABSL_DCHECK_GT(upsampling_period_, Duration32::Zero());
}

void SlidingWindowInputModeler::ExtendStroke(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) {
  if (real_inputs.IsEmpty() && predicted_inputs.IsEmpty()) return;
  EraseUnstableModeledInputs(state, modeled_inputs);
  AppendRawInputsToSlidingWindow(state, real_inputs);
  int sliding_window_real_input_count = sliding_window_.Size();
  AppendRawInputsToSlidingWindow(state, predicted_inputs);
  ModelUnstableInputs(state, modeled_inputs, sliding_window_real_input_count);
  UpdateRealAndCompleteDistanceAndTime(state, modeled_inputs);
  MarkStableModeledInputs(state, modeled_inputs);
  TrimSlidingWindow(state, modeled_inputs, sliding_window_real_input_count);
}

void SlidingWindowInputModeler::EraseUnstableModeledInputs(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs) {
  modeled_inputs.resize(state.stable_input_count);
  state.real_input_count = state.stable_input_count;
}

void SlidingWindowInputModeler::AppendRawInputsToSlidingWindow(
    InputModelerState& state, const StrokeInputBatch& raw_inputs) {
  absl::Status status = sliding_window_.Append(raw_inputs);
  ABSL_DCHECK(status.ok());
}

void SlidingWindowInputModeler::ModelUnstableInputPosition(
    std::vector<ModeledStrokeInput>& modeled_inputs, Duration32 elapsed_time,
    int& start_index, int& end_index) {
  int sliding_window_input_count = sliding_window_.Size();
  ABSL_DCHECK_GT(sliding_window_input_count, 0);

  Duration32 half_window_size =
      std::min(half_window_size_,
               std::min(elapsed_time - sliding_window_.First().elapsed_time,
                        sliding_window_.Last().elapsed_time - elapsed_time));
  Duration32 start_time = elapsed_time - half_window_size;
  Duration32 end_time = elapsed_time + half_window_size;

  // March `start_index` forward until it is at or just before `start_time`.
  while (start_index + 1 < sliding_window_input_count &&
         sliding_window_.Get(start_index + 1).elapsed_time <= start_time) {
    ++start_index;
  }
  // March `end_index` forward until it is at or just after `end_time`.
  while (end_index + 1 < sliding_window_input_count &&
         sliding_window_.Get(end_index).elapsed_time <= end_time) {
    ++end_index;
  }

  float dt = (end_time - start_time).ToSeconds();
  if (dt <= 0) {
    StrokeInput input = sliding_window_.Get(start_index);
    if (IsWithinEpsilonOfLastInput(modeled_inputs, input.position)) {
      return;
    }
    modeled_inputs.push_back(ModeledStrokeInput{
        .position = input.position,
        .traveled_distance = DistanceTraveled(modeled_inputs, input.position),
        .elapsed_time = input.elapsed_time,
        .pressure = input.pressure,
        .tilt = input.tilt,
        .orientation = input.orientation,
    });
    return;
  }

  // Otherwise, if dt > 0, then `start_index` and `end_index` must be distinct,
  // and therefore there is at least one raw-input-to-raw-input interval to
  // integrate over.
  ABSL_DCHECK_LT(start_index, end_index);

  StrokeInputIntegrals integrals;
  for (int i = start_index; i < end_index; ++i) {
    StrokeInput input1 = sliding_window_.Get(i);
    StrokeInput input2 = sliding_window_.Get(i + 1);
    if (input1.elapsed_time < start_time) {
      input1 = InterpolateStrokeInput(input1, input2, start_time);
    }
    if (input2.elapsed_time > end_time) {
      input2 = InterpolateStrokeInput(input1, input2, end_time);
    }
    Integrate(integrals, input1, input2);
  }

  Point position = Point::FromOffset(integrals.position_dt / dt);
  if (IsWithinEpsilonOfLastInput(modeled_inputs, position)) {
    return;
  }
  ModeledStrokeInput modeled_input = {
      .position = position,
      .traveled_distance = DistanceTraveled(modeled_inputs, position),
      .elapsed_time = elapsed_time,
  };
  if (sliding_window_.HasPressure()) {
    modeled_input.pressure = integrals.pressure_dt / dt;
  }
  if (sliding_window_.HasTilt()) {
    modeled_input.tilt = integrals.tilt_dt / dt;
  }
  if (sliding_window_.HasOrientation()) {
    modeled_input.orientation =
        (integrals.orientation_dt / dt).Direction().Normalized();
  }
  modeled_inputs.push_back(modeled_input);
}

void SlidingWindowInputModeler::ModelUnstableInputPositions(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    Duration32 real_input_cutoff) {
  int sliding_window_input_count = sliding_window_.Size();
  // As we iterate through timestamps for new modeled inputs, keep track of the
  // indices of the raw inputs at or just before/after the edges of the sliding
  // window.  We will march these start/end indices forward as we iterate (in
  // order to keep this loop O(n)).
  int start_index = 0;
  int end_index = 0;
  Duration32 prev_modeled_input_time = !modeled_inputs.empty()
                                           ? modeled_inputs.back().elapsed_time
                                           : -Duration32::Infinite();
  for (int i = 0; i < sliding_window_input_count; ++i) {
    Duration32 raw_input_time = sliding_window_.Get(i).elapsed_time;
    if (raw_input_time <= prev_modeled_input_time) continue;

    // If upsampling is necessary, generate intermediate modeled inputs between
    // the last one and the one that corresponds to this raw input.
    if (prev_modeled_input_time.IsFinite()) {
      Duration32 dt = raw_input_time - prev_modeled_input_time;
      int num_divisions =
          static_cast<int>(std::min(std::ceil(dt / upsampling_period_),
                                    static_cast<float>(kMaxUpsampleDivisions)));
      if (num_divisions > 1) {
        Duration32 period = dt / num_divisions;
        for (int i = 1; i < num_divisions; ++i) {
          Duration32 elapsed_time = prev_modeled_input_time + period * i;
          ModelUnstableInputPosition(modeled_inputs, elapsed_time, start_index,
                                     end_index);
        }
      }
    }

    // Always generate a modeled input at each raw input. (We don't have to
    // choose to do this--we could instead just generate exactly one modeled
    // input every `upsampling_period_` like clockwork--but it's a convenient
    // way to stay more faithful to the raw input, and ensure that we capture
    // the endpoints of the stroke correctly.)
    Duration32 elapsed_time = raw_input_time;
    ModelUnstableInputPosition(modeled_inputs, elapsed_time, start_index,
                               end_index);
    if (elapsed_time <= real_input_cutoff) {
      state.real_input_count = modeled_inputs.size();
    }
    prev_modeled_input_time = elapsed_time;
  }
}

void SlidingWindowInputModeler::ModelUnstableInputs(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    int sliding_window_real_input_count) {
  ABSL_DCHECK_LE(sliding_window_real_input_count, sliding_window_.Size());
  // Get the timestamp of the last real raw input so far, if any. Any modeled
  // inputs generated up to and including this timestamp should be considered
  // "real".
  Duration32 real_input_cutoff =
      sliding_window_real_input_count == 0
          ? -Duration32::Infinite()
          : sliding_window_.Get(sliding_window_real_input_count - 1)
                .elapsed_time;
  // Append new modeled inputs, without velocity or acceleration for now.
  ModelUnstableInputPositions(state, modeled_inputs, real_input_cutoff);
  // Now that we've modeled positions, we can use them to compute the velocity
  // for each unstable input.
  ComputeDerivativeForUnstableInputs(
      &ModeledStrokeInput::position, &ModeledStrokeInput::velocity,
      modeled_inputs, state.stable_input_count, half_window_size_);
  // Now that we've modeled velocities, we can use them to compute the
  // acceleration for each unstable input.
  ComputeDerivativeForUnstableInputs(
      &ModeledStrokeInput::velocity, &ModeledStrokeInput::acceleration,
      modeled_inputs, state.stable_input_count, half_window_size_);
}

void SlidingWindowInputModeler::UpdateRealAndCompleteDistanceAndTime(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs) {
  if (state.real_input_count > 0) {
    const ModeledStrokeInput& last_real_input =
        modeled_inputs[state.real_input_count - 1];
    state.total_real_elapsed_time = last_real_input.elapsed_time;
    state.total_real_distance = last_real_input.traveled_distance;
  }
  if (!modeled_inputs.empty()) {
    const ModeledStrokeInput& last_input = modeled_inputs.back();
    state.complete_traveled_distance = last_input.traveled_distance;
    state.complete_elapsed_time = last_input.elapsed_time;
  }
}

void SlidingWindowInputModeler::MarkStableModeledInputs(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs) {
  ABSL_DCHECK_LE(state.stable_input_count, state.real_input_count);
  ABSL_DCHECK_LE(state.real_input_count, modeled_inputs.size());
  while (state.stable_input_count < state.real_input_count &&
         modeled_inputs[state.stable_input_count].elapsed_time +
                 half_window_size_ <
             state.total_real_elapsed_time) {
    ++state.stable_input_count;
  }
}

void SlidingWindowInputModeler::TrimSlidingWindow(
    InputModelerState& state, std::vector<ModeledStrokeInput>& modeled_inputs,
    int sliding_window_real_input_count) {
  ABSL_DCHECK_LE(sliding_window_real_input_count, sliding_window_.Size());

  // Erase all predicted stroke inputs from the end of the sliding window.
  sliding_window_.Erase(sliding_window_real_input_count);

  // If there are no real modeled inputs yet, there's nothing to trim from the
  // front of the window.
  if (state.real_input_count == 0) return;

  // The last real modeled input will never be stable (since more raw inputs
  // could appear right after it). Therefore, if there are any real modeled
  // inputs, then there is at least one unstable modeled input.
  ABSL_DCHECK_LT(state.stable_input_count, state.real_input_count);
  const ModeledStrokeInput& first_unstable_input =
      modeled_inputs[state.stable_input_count];

  // We can trim a real raw input from the front of the sliding window if the
  // next real input after it is already at or before the start of the first
  // unstable input's window.
  Duration32 cutoff = first_unstable_input.elapsed_time - half_window_size_;
  int next_input_index = 1;
  while (next_input_index < sliding_window_real_input_count &&
         sliding_window_.Get(next_input_index).elapsed_time <= cutoff) {
    ++next_input_index;
  }
  int num_sliding_inputs_to_trim = next_input_index - 1;
  sliding_window_.Erase(0, num_sliding_inputs_to_trim);
}

bool SlidingWindowInputModeler::IsWithinEpsilonOfLastInput(
    const std::vector<ModeledStrokeInput>& modeled_inputs,
    Point position) const {
  if (modeled_inputs.empty()) {
    return false;
  }
  const ModeledStrokeInput& last_input = modeled_inputs.back();
  return Distance(last_input.position, position) <= position_epsilon_;
}

}  // namespace ink::strokes_internal
