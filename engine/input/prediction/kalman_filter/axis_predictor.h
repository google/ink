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

#ifndef INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_
#define INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_

#include <memory>

#include "ink/engine/input/prediction/kalman_filter/kalman_filter.h"

namespace ink {

// Class to predict on axis.
//
// This predictor use one instance of kalman filter to predict one dimension of
// stylus movement.
class AxisPredictor {
 public:
  AxisPredictor();

  // Return true if the underlying kalman filter is stable.
  bool Stable();

  // Reset the underlying kalman filter.
  void Reset();

  // Update the predictor with a new observation.
  void Update(double observation);

  // Get the predicted values from the underlying kalman filter.
  double GetPosition() const;
  double GetVelocity() const;
  double GetAcceleration() const;
  double GetJerk() const;

 private:
  std::unique_ptr<KalmanFilter> kalman_filter_;
};

}  // namespace ink
#endif  // INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_
