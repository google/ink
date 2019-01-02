// Copyright 2018 Google LLC
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

#include "ink/engine/input/prediction/kalman_filter/kalman_filter.h"

namespace {
constexpr int kStableIterNum = 4;
}  // namespace

namespace ink {

KalmanFilter::KalmanFilter(const glm::dmat4& state_transition,
                           const glm::dmat4& process_noise_covariance,
                           const glm::dvec4& measurement_vector,
                           double measurement_noise_variance)
    : state_transition_matrix_(state_transition),
      process_noise_covariance_matrix_(process_noise_covariance),
      measurement_vector_(measurement_vector),
      measurement_noise_variance_(measurement_noise_variance),
      iter_num_(0) {}

const glm::dvec4& KalmanFilter::GetStateEstimation() const {
  return state_estimation_;
}

bool KalmanFilter::Stable() { return iter_num_ >= kStableIterNum; }

void KalmanFilter::Predict() {
  // X = F * X
  state_estimation_ = state_transition_matrix_ * state_estimation_;
  // P = F * P * F' + Q
  error_covariance_matrix_ = state_transition_matrix_ *
                                 error_covariance_matrix_ *
                                 glm::transpose(state_transition_matrix_) +
                             process_noise_covariance_matrix_;
}

void KalmanFilter::Update(double observation) {
  if (iter_num_++ == 0) {
    // We only update the state estimation in the first iteration.
    state_estimation_[0] = observation;
    return;
  }
  Predict();
  // Y = z - H * X
  double y = observation - glm::dot(measurement_vector_, state_estimation_);
  // S = H * P * H' + R
  double S = glm::dot(measurement_vector_ * error_covariance_matrix_,
                      measurement_vector_) +
             measurement_noise_variance_;
  // K = P * H' * inv(S)
  glm::dvec4 kalman_gain = measurement_vector_ * error_covariance_matrix_ / S;

  // X = X + K * Y
  state_estimation_ = state_estimation_ + kalman_gain * y;

  // I_HK = eye(P) - K * H
  glm::dmat4 I_KH =
      glm::dmat4(1.0) - glm::outerProduct(kalman_gain, measurement_vector_);

  // P = I_KH * P * I_KH' + K * R * K'
  error_covariance_matrix_ =
      I_KH * error_covariance_matrix_ * glm::transpose(I_KH) +
      glm::outerProduct(kalman_gain, kalman_gain) * measurement_noise_variance_;
}

void KalmanFilter::Reset() {
  state_estimation_ = {0, 0, 0, 0};
  error_covariance_matrix_ = glm::dmat4(1.0);  // identity
  iter_num_ = 0;
}

}  // namespace ink
