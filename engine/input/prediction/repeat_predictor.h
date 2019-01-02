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

#ifndef INK_ENGINE_INPUT_PREDICTION_REPEAT_PREDICTOR_H_
#define INK_ENGINE_INPUT_PREDICTION_REPEAT_PREDICTOR_H_

#include "ink/engine/input/input_data.h"
#include "ink/engine/input/prediction/input_predictor.h"
#include "ink/engine/service/dependencies.h"

namespace ink {
namespace input {

// Predicts input by repeating the last known input point
// (predict_interval / min_sample_dt) times.
class RepeatPredictor : public InputPredictor {
 protected:
  void ResetImpl(const Camera& cam, DurationS predict_interval,
                 DurationS min_sample_dt) override;
  bool HasPrediction() const override { return true; }
  std::vector<InputData> GeneratePredictedPoints(
      glm::vec2 last_modeled_point, glm::vec2 model_velocity) const override;

 private:
  size_t n_points_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_PREDICTION_REPEAT_PREDICTOR_H_
