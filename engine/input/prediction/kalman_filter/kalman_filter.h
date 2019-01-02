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

#ifndef INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_
#define INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_

#include "third_party/glm/glm/glm.hpp"

namespace ink {

// Kalman filter.
// Generates a state estimation based upon observations which can then be used
// to compute predicted values.
class KalmanFilter {
 public:
  KalmanFilter(const glm::dmat4& state_transition,
               const glm::dmat4& process_noise_covariance,
               const glm::dvec4& measurement_vector,
               double measurement_noise_variance);

  // Get the estimation of current state.
  const glm::dvec4& GetStateEstimation() const;

  // Will return true only if the kalman filter has seen enough data and is
  // considered as stable.
  bool Stable();

  // Update the observation of the system.
  void Update(double observation);

  void Reset();

 private:
  void Predict();

  // Estimate of the latent state
  // Symbol: X
  // Dimension: state_vector_dim_
  glm::dvec4 state_estimation_{0, 0, 0, 0};

  // The covariance of the difference between prior predicted latent
  // state and posterior estimated latent state (the so-called "innovation".
  // Symbol: P
  glm::dmat4 error_covariance_matrix_{1};

  // For position, state transition matrix is derived from basic physics:
  // new_x = x + v * dt + 1/2 * a * dt^2 + 1/6 * jerk * dt^3
  // new_v = v + a * dt + 1/2 * jerk * dt^2
  // ...
  // Matrix that transmit current state to next state
  // Symbol: F
  glm::dmat4 state_transition_matrix_{1};

  // Process_noise_covariance_matrix_ is a time-varying parameter that will be
  // estimated as part of the kalman filter process.
  // Symbol: Q
  glm::dmat4 process_noise_covariance_matrix_{1};

  // Vector to transform estimate to measurement.
  // Symbol: H
  const glm::dvec4 measurement_vector_{0, 0, 0, 0};

  // measurement_noise_ is a time-varying parameter that will be estimated as
  // part of the kalman filter process.
  // Symbol: R
  double measurement_noise_variance_;

  // Tracks number of update iteration happened at this kalman filter. At the
  // 1st iteration, the state estimate will be updated to the measured value.
  // After a few (defined in .cc file) iterations, the KalmanFilter is
  // considered stable.
  int iter_num_;
};

}  // namespace ink
#endif  // INK_ENGINE_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_
