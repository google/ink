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

#include "ink/engine/input/prediction/input_predictor.h"

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace input {

void InputPredictor::Reset(const Camera& cam, DurationS predict_interval,
                           DurationS min_sample_dt) {
  min_sample_dt_ = min_sample_dt;
  has_modeled_input_ = false;
  ResetImpl(cam, predict_interval, min_sample_dt);
}

void InputPredictor::Update(const InputData& point, bool sent_to_modeler) {
  last_input_received_ = point;
  if (sent_to_modeler) {
    last_input_sent_to_model_ = point;
  }
  has_modeled_input_ = true;
  UpdateImpl(point, sent_to_modeler);
}

std::vector<InputData> InputPredictor::PredictedPoints(
    glm::vec2 last_modeled_point, glm::vec2 model_velocity) const {
  ASSERT(has_modeled_input_);
  std::vector<InputData> predicted_points;
  if (HasPrediction()) {
    predicted_points =
        GeneratePredictedPoints(last_modeled_point, model_velocity);
    SLOG(SLOG_INPUT, "Had $0 predicted points", predicted_points.size());
    for (size_t i = 0; i < predicted_points.size(); i++) {
      predicted_points[i].time =
          last_input_sent_to_model_.time + ((i + 1) * min_sample_dt_);
    }
  }

  if (predicted_points.size() < 2) {
    SLOG(SLOG_INPUT,
         "Prediction not ready, using last modeled input and last input");
    predicted_points =
        std::vector<InputData>{last_input_sent_to_model_, last_input_received_};
  }
  return predicted_points;
}

}  // namespace input
}  // namespace ink
