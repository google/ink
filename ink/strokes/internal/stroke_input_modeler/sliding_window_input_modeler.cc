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
#include <cstddef>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/distance.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {
namespace {

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

void SlidingWindowInputModeler::StartStroke(float brush_epsilon) {
  modeled_inputs_.clear();
  state_ = State{};
  sliding_window_.Clear();
  stable_sliding_window_input_count_ = 0;
}

void SlidingWindowInputModeler::ExtendStroke(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs, Duration32 current_elapsed_time) {
  modeled_inputs_.resize(state_.stable_input_count);
  AppendInputsToSlidingWindow(real_inputs);
  state_.real_input_count = state_.stable_input_count + sliding_window_.Size() -
                            stable_sliding_window_input_count_;
  AppendInputsToSlidingWindow(predicted_inputs);
  ModelUnstableInputs();
  UpdateRealAndCompleteDistanceAndTime(current_elapsed_time);
  MarkInputsStable();
  TrimSlidingWindow();
}

void SlidingWindowInputModeler::AppendInputsToSlidingWindow(
    const StrokeInputBatch& inputs) {
  if (inputs.IsEmpty()) return;
  state_.tool_type = inputs.GetToolType();
  state_.stroke_unit_length = inputs.GetStrokeUnitLength();
  absl::Status status = sliding_window_.Append(inputs);
  ABSL_DCHECK(status.ok());
}

void SlidingWindowInputModeler::ModelUnstableInputPosition(int input_index) {
  int sliding_window_input_count = sliding_window_.Size();
  const StrokeInput& input = sliding_window_.Get(input_index);
  Duration32 half_window_size = std::min(
      std::min(half_window_size_,
               input.elapsed_time - sliding_window_.Get(0).elapsed_time),
      sliding_window_.Get(sliding_window_input_count - 1).elapsed_time -
          input.elapsed_time);
  Duration32 start_time = input.elapsed_time - half_window_size;
  Duration32 end_time = input.elapsed_time + half_window_size;
  float dt = (end_time - start_time).ToSeconds();
  if (dt <= 0) {
    modeled_inputs_.push_back(ModeledStrokeInput{
        .position = input.position,
        .traveled_distance = DistanceTraveled(modeled_inputs_, input.position),
        .elapsed_time = input.elapsed_time,
        .pressure = input.pressure,
        .tilt = input.tilt,
        .orientation = input.orientation,
    });
    return;
  }

  StrokeInputIntegrals integrals;
  for (int i = input_index; i > 0; --i) {
    StrokeInput input1 = sliding_window_.Get(i - 1);
    StrokeInput input2 = sliding_window_.Get(i);
    ABSL_DCHECK_GE(input2.elapsed_time, start_time);
    if (input1.elapsed_time < start_time) {
      input1 = InterpolateStrokeInput(input1, input2, start_time);
    }
    Integrate(integrals, input1, input2);
    if (input1.elapsed_time <= start_time) break;
  }
  for (int i = input_index; i + 1 < sliding_window_input_count; ++i) {
    StrokeInput input1 = sliding_window_.Get(i);
    StrokeInput input2 = sliding_window_.Get(i + 1);
    ABSL_DCHECK_LE(input1.elapsed_time, end_time);
    if (input2.elapsed_time > end_time) {
      input2 = InterpolateStrokeInput(input1, input2, end_time);
    }
    Integrate(integrals, input1, input2);
    if (input2.elapsed_time >= end_time) break;
  }

  ModeledStrokeInput modeled_input = {
      .position = Point::FromOffset(integrals.position_dt / dt),
      .elapsed_time = input.elapsed_time,
  };
  modeled_input.traveled_distance =
      DistanceTraveled(modeled_inputs_, modeled_input.position);
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
  modeled_inputs_.push_back(modeled_input);
}

void SlidingWindowInputModeler::ModelUnstableInputs() {
  int sliding_window_input_count = sliding_window_.Size();
  // Append new modeled inputs.  Note that we intentionally don't call
  // `modeled_inputs_.reserve()` here, since the next call to `ExtendStroke()`
  // will append yet more modeled inputs, and we don't want to disrupt
  // `std::vector`s long-term amortization.
  for (int i = stable_sliding_window_input_count_;
       i < sliding_window_input_count; ++i) {
    ModelUnstableInputPosition(i);
  }
  // Now that we've modeled positions, we can use them to compute the velocity
  // for each unstable input.
  ComputeDerivativeForUnstableInputs(
      &ModeledStrokeInput::position, &ModeledStrokeInput::velocity,
      modeled_inputs_, state_.stable_input_count, half_window_size_);
  // Now that we've modeled velocities, we can use them to compute the
  // acceleration for each unstable input.
  ComputeDerivativeForUnstableInputs(
      &ModeledStrokeInput::velocity, &ModeledStrokeInput::acceleration,
      modeled_inputs_, state_.stable_input_count, half_window_size_);
}

void SlidingWindowInputModeler::UpdateRealAndCompleteDistanceAndTime(
    Duration32 current_elapsed_time) {
  if (state_.real_input_count > 0) {
    const ModeledStrokeInput& last_real_input =
        modeled_inputs_[state_.real_input_count - 1];
    state_.total_real_elapsed_time = last_real_input.elapsed_time;
    state_.total_real_distance = last_real_input.traveled_distance;
  }
  if (modeled_inputs_.empty()) {
    // Although it would basically never happen in practice, it's theoretically
    // possible for the API caller to extend a stroke with predicted inputs only
    // (no real inputs at all), and then later erase that prediction. In this
    // case, `modeled_inputs_` would be non-empty and `complete_elapsed_time`
    // and `complete_traveled_distance` would be non-zero, and then later
    // `modeled_inputs_` would become empty again, so we need to zero
    // `complete_traveled_distance` and `complete_elapsed_time` in that case.
    state_.complete_traveled_distance = 0;
    state_.complete_elapsed_time = Duration32::Zero();
  } else {
    const ModeledStrokeInput& last_input = modeled_inputs_.back();
    state_.complete_traveled_distance = last_input.traveled_distance;
    state_.complete_elapsed_time = last_input.elapsed_time;
  }
  state_.complete_elapsed_time =
      std::max(state_.complete_elapsed_time, current_elapsed_time);
}

void SlidingWindowInputModeler::MarkInputsStable() {
  while (stable_sliding_window_input_count_ < sliding_window_.Size() &&
         sliding_window_.Get(stable_sliding_window_input_count_).elapsed_time +
                 half_window_size_ <=
             state_.total_real_elapsed_time) {
    ++state_.stable_input_count;
    ++stable_sliding_window_input_count_;
  }
}

void SlidingWindowInputModeler::TrimSlidingWindow() {
  // Erase all predicted stroke inputs from the end of the sliding window.
  sliding_window_.Erase(stable_sliding_window_input_count_ +
                        state_.real_input_count - state_.stable_input_count);

  // If none of the stroke inputs in the sliding window are stable yet, there's
  // nothing to trim from the front of the window.
  if (stable_sliding_window_input_count_ == 0) return;

  // The last raw input in `sliding_window_` will never be stable (since more
  // raw inputs could appear right after it).
  ABSL_DCHECK_LT(stable_sliding_window_input_count_, sliding_window_.Size());
  StrokeInput first_unstable_input =
      sliding_window_.Get(stable_sliding_window_input_count_);

  // We can trim a stable input from the front of the sliding window if the
  // next stable input after it is already at or before the start of the first
  // unstable input's window.
  Duration32 cutoff = first_unstable_input.elapsed_time - half_window_size_;
  int next_input_index = 1;
  while (next_input_index < stable_sliding_window_input_count_ &&
         sliding_window_.Get(next_input_index).elapsed_time <= cutoff) {
    ++next_input_index;
  }
  int num_sliding_inputs_to_trim = next_input_index - 1;
  sliding_window_.Erase(0, num_sliding_inputs_to_trim);
  stable_sliding_window_input_count_ -= num_sliding_inputs_to_trim;
}

}  // namespace ink::strokes_internal
