/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_INPUT_INPUT_MODEL_DATA_H_
#define INK_ENGINE_INPUT_INPUT_MODEL_DATA_H_

#include <string>

#include "ink/engine/public/types/input.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace input {

class InputModelParams {
 public:
  // Construct the correct InputModelParams by type (Mouse, Pen, Touch)
  //
  // InputType::Invalid is considered to be Touch.
  explicit InputModelParams(InputType type);

  // Oversample the model by this factor
  int NumInterpolationPoints() const { return num_interpolation_pts_; }

  // Downsample input to at most this frequency.
  // Watch out for aliasing artifacts!
  double MaxSampleHz() const { return max_sample_hz_; }

  // Predict at most this far into the future.
  DurationS PredictInterval() const { return predict_interval_; }

  // Speed multiplier to slow down the model for input past TUp
  // Lower values slow down the modeled input faster
  float SpeedModForStrokeEnd() const { return speed_mod_for_stroke_end_; }

  // The maximum number of modeled points to generate after a TUp is seen
  int MaxPointsAfterUp() const { return max_points_after_up_; }

  // Multiplicative dampener on modeled input position
  // See engine/brushes/README.md
  float Drag() const { return drag_; }

  // Higher mass decreases how much the model responds to each input event
  // See engine/brushes/README.md
  float Mass() const { return mass_; }

  // Determines how far back in time the moving average should look when
  // reducing input wobble (see PhysicsInputModeler::FilterWobble) -- the
  // timeout will be wobble_timeout_ratio / mazSampleHz.
  float WobbleTimeoutRatio() const { return wobble_timeout_ratio_; }

  // The speed bounds for interpolating between the given and moving average
  // points when reducing input wobble (see PhysicsInputModeler::FilterWobble).
  float WobbleSlowSpeedCM() const { return wobble_slow_speed_cm_; }
  float WobbleFastSpeedCM() const { return wobble_fast_speed_cm_; }

  std::string ToString() const;
  std::string toString() const { return ToString(); }

 private:
  // For testing.
  friend InputModelParams MakeInputModelParams(double, InputModelParams);
  friend InputModelParams MakeInputModelParamsWithWobble(float, float, float);

  int num_interpolation_pts_;
  double max_sample_hz_;
  DurationS predict_interval_;
  float speed_mod_for_stroke_end_;
  int max_points_after_up_;
  float drag_;
  float mass_;
  float wobble_timeout_ratio_;
  float wobble_slow_speed_cm_;
  float wobble_fast_speed_cm_;
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_MODEL_DATA_H_
