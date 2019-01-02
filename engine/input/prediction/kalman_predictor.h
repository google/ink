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

#ifndef INK_ENGINE_INPUT_PREDICTION_KALMAN_PREDICTOR_H_
#define INK_ENGINE_INPUT_PREDICTION_KALMAN_PREDICTOR_H_

#include <deque>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/prediction/input_predictor.h"
#include "ink/engine/input/prediction/kalman_filter/axis_predictor.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace input {

// This predictor uses kalman filters to predict the current status of the
// motion. Then it predict the future points using <current_position,
// predicted_velocity, predicted_acceleration, predicted_jerk>. Each
// kalman_filter will only be used to predict one dimension (x, y).
class KalmanPredictor : public InputPredictor {
 public:
  bool PredictionExpectsModeling() const override { return false; };

  // Public for testing:
  std::vector<glm::vec2> PredictedPoints(int num_predictions) const;
  // Connecting points from the given modeled input to the current first
  // prediction point.
  std::vector<glm::vec2> ConnectingPoints(glm::vec2 last_modeled_point,
                                          glm::vec2 model_velocity) const;
  // Number of points to predict in the given state.
  int PointsToPredict() const;

 protected:
  void UpdateImpl(const input::InputData& cur_input,
                  bool sent_to_modeler) override;
  void ResetImpl(const Camera& cam, DurationS predict_interval,
                 DurationS min_sample_dt) override;

  bool HasPrediction() const override;
  std::vector<InputData> GeneratePredictedPoints(
      glm::vec2 last_modeled_point, glm::vec2 model_velocity) const override;

 private:
  // The following functions get the predicted values from kalman filters.
  glm::vec2 PredictPosition() const;
  glm::vec2 PredictVelocity() const;
  glm::vec2 PredictAcceleration() const;
  glm::vec2 PredictJerk() const;

  // Enqueue the new input data to sample points and maintain avg report time.
  void Enqueue(const input::InputData& cur_input);

  // Avg report rate in milliseconds.
  double avg_report_delta_time_ms_;

  // Predictor for each axis.
  AxisPredictor x_predictor_;
  AxisPredictor y_predictor_;

  // A deque contains recent sample inputs.
  std::deque<input::InputData> sample_points_;

  // True if the predictor has valid predictions.
  bool is_valid_;

  // Line start camera, used for translate coords.
  Camera cam_;

  // How far into the future to predict.
  DurationS predict_interval_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_PREDICTION_KALMAN_PREDICTOR_H_
