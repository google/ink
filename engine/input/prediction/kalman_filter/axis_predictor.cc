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

#include "ink/engine/input/prediction/kalman_filter/axis_predictor.h"

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"

namespace {
constexpr int kPositionIndex = 0;
constexpr int kVelocityIndex = 1;
constexpr int kAccelerationIndex = 2;
constexpr int kJerkIndex = 3;

constexpr double kDt = 1.0;

constexpr double kSigmaProcess = 0.01;
constexpr double kSigmaMeasurement = 1.0;
}  // namespace

namespace ink {

AxisPredictor::AxisPredictor() {
  // State translation matrix is basic physics.
  // new_pos = pre_pos + v * dt + 1/2 * a * dt^2 + 1/6 * J * dt^3.
  // new_v = v + a * dt + 1/2 * J * dt^2.
  // new_a = a * dt + J * dt.
  // new_j = J.
  // Note that glm matrices are in column-major order
  // clang-format off
  glm::dmat4 state_transition(
      1.0, 0.0, 0.0, 0.0,
      kDt, 1.0, 0.0, 0.0,
      0.5 * kDt * kDt, kDt, 1.0, 0.0,
      1.0 / 6 * kDt * kDt * kDt, 0.5 * kDt * kDt, kDt, 1.0);
  // clang-format on

  // We model the system noise as noisy force on the pen.
  // The following matrix describes the impact of that noise on each state.
  glm::dvec4 process_noise(1.0 / 6 * kDt * kDt * kDt, 0.5 * kDt * kDt, kDt,
                           1.0);
  glm::dmat4 process_noise_covariance =
      glm::outerProduct(process_noise, process_noise) * kSigmaProcess;

  // Sensor only detects location. Thus measurement only impact the position.
  glm::dvec4 measurement_vector(1.0, 0.0, 0.0, 0.0);

  kalman_filter_ = absl::make_unique<KalmanFilter>(
      state_transition, process_noise_covariance, measurement_vector,
      kSigmaMeasurement);
}

bool AxisPredictor::Stable() {
  return kalman_filter_ && kalman_filter_->Stable();
}

void AxisPredictor::Reset() {
  if (kalman_filter_) kalman_filter_->Reset();
}

void AxisPredictor::Update(double observation) {
  if (kalman_filter_) kalman_filter_->Update(observation);
}

double AxisPredictor::GetPosition() const {
  if (kalman_filter_)
    return kalman_filter_->GetStateEstimation()[kPositionIndex];
  else
    return 0.0;
}

double AxisPredictor::GetVelocity() const {
  if (kalman_filter_)
    return kalman_filter_->GetStateEstimation()[kVelocityIndex];
  else
    return 0.0;
}

double AxisPredictor::GetAcceleration() const {
  return kalman_filter_->GetStateEstimation()[kAccelerationIndex];
}

double AxisPredictor::GetJerk() const {
  return kalman_filter_->GetStateEstimation()[kJerkIndex];
}

}  // namespace ink
