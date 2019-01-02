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

#include "ink/engine/input/prediction/kalman_predictor.h"

#include <algorithm>
#include <cmath>

#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/bezier.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace {
// Max number of samples in sliding window for mean dt measurement
constexpr int kMaxSampleSize = 20;
// Influence of jerk during each prediction sample
constexpr float kJerkInfluence = 0.1f;
// Influence of acceleration during each prediction sample
constexpr float kAccelerationInfluence = 0.5f;
// Influence of velocity during each prediction sample
constexpr float kVelocityInfluence = 1.0f;

// Range of jerk values to expect.
// Low value will use maximum prediction, high value will use no prediction.
constexpr float kJerkLow = 0.02f;
constexpr float kJerkHigh = 0.2f;

// Range of pen speed to expect (in pixels / ms).
// Low value will not use prediction, high value will use full prediction.
constexpr float kSpeedLow = 0.0f;
constexpr float kSpeedHigh = 2.0f;

double ToMilliseconds(ink::DurationS seconds) {
  return static_cast<double>(seconds * 1000);
}
}  // namespace

namespace ink {
namespace input {

void KalmanPredictor::UpdateImpl(const input::InputData& cur_input,
                                 bool sent_to_modeler) {
  if (cur_input.DeltaTime() < 0) {
    SLOG(SLOG_INPUT, "Skipped input with negative or zero DeltaTime");
    return;
  }
  Enqueue(cur_input);
  x_predictor_.Update(cur_input.screen_pos.x);
  y_predictor_.Update(cur_input.screen_pos.y);

  is_valid_ = x_predictor_.Stable() && y_predictor_.Stable();
}

bool KalmanPredictor::HasPrediction() const {
  return is_valid_ && LastInput().type != InputType::Mouse;
}

std::vector<glm::vec2> KalmanPredictor::PredictedPoints(
    int num_predictions) const {
  glm::vec2 position = PredictPosition();
  glm::vec2 velocity = PredictVelocity();
  glm::vec2 acceleration = PredictAcceleration();
  glm::vec2 jerk = PredictJerk();

  std::vector<glm::vec2> predictions;
  for (int i = 0; i < num_predictions; i++) {
    acceleration += jerk * kJerkInfluence;
    velocity += acceleration * kAccelerationInfluence;
    position += velocity * kVelocityInfluence;
    predictions.push_back(position);
  }
  return predictions;
}

std::vector<glm::vec2> KalmanPredictor::ConnectingPoints(
    glm::vec2 last_modeled_point, glm::vec2 model_velocity) const {
  glm::vec2 velocity = PredictVelocity();
  float speed = glm::length(velocity);
  glm::vec2 position = PredictPosition();
  std::vector<glm::vec2> connecting_points;

  if (speed < std::numeric_limits<float>::epsilon() ||
      glm::length(model_velocity) < std::numeric_limits<float>::epsilon()) {
    // No speed on either end, can't construct bezier, so bail out.
    return connecting_points;
  }

  // Draw the control points 1/4 of the distance between the start and end,
  // projected along the velocity vectors.
  float control_pt_distance = glm::distance(position, last_modeled_point) / 4;

  // Unit vector for the modeled velocity
  glm::vec2 model_vec = model_velocity / glm::length(model_velocity);
  glm::vec2 modeled_control =
      last_modeled_point + model_vec * control_pt_distance;

  glm::vec2 prediction_vec = velocity / speed;
  glm::vec2 prediction_control =
      position - prediction_vec * control_pt_distance;

  int points = glm::distance(position, last_modeled_point) / speed;
  Bezier b;
  b.SetNumEvalPoints(points);
  b.MoveTo(last_modeled_point);
  b.CurveTo(modeled_control, prediction_control, position);
  auto polyline = b.Polyline();

  connecting_points.reserve(points);
  for (auto line : polyline) {
    for (auto pt : line) {
      connecting_points.push_back(pt);
    }
  }
  return connecting_points;
}

int KalmanPredictor::PointsToPredict() const {
  glm::vec2 velocity = PredictVelocity();
  glm::vec2 jerk = PredictJerk();

  // Adjust prediction distance based on confidence of the Kalman filter and
  // movement speed.
  float speed = glm::length(velocity) / avg_report_delta_time_ms_;
  float speed_factor =
      util::Clamp01(util::Normalize(kSpeedLow, kSpeedHigh, speed));
  float jerk_abs = glm::length(jerk);
  float jerk_factor =
      1.0f - util::Clamp01(util::Normalize(kJerkLow, kJerkHigh, jerk_abs));
  float confidence = speed_factor * jerk_factor;

  int predict_target_sample_num =
      static_cast<int>(ceil(ToMilliseconds(predict_interval_) /
                            avg_report_delta_time_ms_ * confidence));
  SLOG(SLOG_INPUT, "Predict target: $0, confidence: $1, actual: $2",
       static_cast<int>(
           ceil(ToMilliseconds(predict_interval_) / avg_report_delta_time_ms_)),
       confidence, predict_target_sample_num);

  return predict_target_sample_num;
}

std::vector<InputData> KalmanPredictor::GeneratePredictedPoints(
    glm::vec2 last_modeled_point, glm::vec2 model_velocity) const {
  std::vector<InputData> points;
  input::InputData last_point = sample_points_.back();

  auto add_point = [this, &points, &last_point](glm::vec2 pt) {
    InputData next_point = last_point;
    next_point.screen_pos = pt;
    next_point.world_pos = cam_.ConvertPosition(
        next_point.screen_pos, CoordType::kScreen, CoordType::kWorld);
    input::InputData::SetLastPacketInfo(&next_point, &last_point);
    points.push_back(next_point);
    last_point = next_point;
  };

  auto connecting_points = ConnectingPoints(last_modeled_point, model_velocity);
  std::for_each(connecting_points.begin(), connecting_points.end(), add_point);

  int predict_target_sample_num = PointsToPredict();

  // If we have low confidence or no predict interval, always at least return
  // the last input received.
  if (predict_target_sample_num == 0) {
    points.push_back(LastInput());
  }

  auto predictions = PredictedPoints(predict_target_sample_num);
  std::for_each(predictions.begin(), predictions.end(), add_point);

  return points;
}

void KalmanPredictor::ResetImpl(const Camera& cam, DurationS predict_interval,
                                DurationS min_sample_dt) {
  x_predictor_.Reset();
  y_predictor_.Reset();
  cam_ = cam;
  predict_interval_ = predict_interval;
}

glm::vec2 KalmanPredictor::PredictPosition() const {
  return glm::vec2(x_predictor_.GetPosition(), y_predictor_.GetPosition());
}

glm::vec2 KalmanPredictor::PredictVelocity() const {
  return glm::vec2(x_predictor_.GetVelocity(), y_predictor_.GetVelocity());
}

glm::vec2 KalmanPredictor::PredictAcceleration() const {
  return glm::vec2(x_predictor_.GetAcceleration(),
                   y_predictor_.GetAcceleration());
}

glm::vec2 KalmanPredictor::PredictJerk() const {
  return glm::vec2(x_predictor_.GetJerk(), y_predictor_.GetJerk());
}

void KalmanPredictor::Enqueue(const input::InputData& cur_input) {
  sample_points_.push_back(cur_input);
  if (sample_points_.size() > kMaxSampleSize) sample_points_.pop_front();
  // If we have only one point, we could only use the delta time as the avg.
  if (sample_points_.size() == 1) {
    avg_report_delta_time_ms_ =
        ToMilliseconds(sample_points_.front().DeltaTime());
    return;
  }

  DurationS sum_of_delta =
      sample_points_.back().time - sample_points_.front().time;
  avg_report_delta_time_ms_ =
      ToMilliseconds(sum_of_delta / (sample_points_.size() - 1));
}

}  // namespace input
}  // namespace ink
