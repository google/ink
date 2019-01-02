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

#ifndef INK_ENGINE_INPUT_PREDICTION_INPUT_PREDICTOR_H
#define INK_ENGINE_INPUT_PREDICTION_INPUT_PREDICTOR_H

#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace input {

// Interface for input predictors that generate points based on past input.
//
// For each line, reset the predictor with the current camera and parameters for
// the input type used for that line.  Then update with inputs from the raw
// input stream.  Predicted points will always contain at least the last
// received input unless accessed without at least one call to Update.
class InputPredictor {
 public:
  virtual ~InputPredictor() {}

  // Reset once per line with that line's starting camera, the desired interval
  // to predict into the future, and the minimum time period between samples.
  void Reset(const Camera& cam, DurationS predict_interval,
             DurationS min_sample_dt);

  // Update the predictor's model with each new input point for the line.
  // sent_to_modeler will be false if the input was filtered out by the input
  // modeler's sampling heuristics.
  void Update(const InputData& point, bool sent_to_modeler);

  // Retrieve the current prediction.
  //
  // This will always contain at least the most recent modeled input and most
  // recent input point if there is no valid prediction, and will otherwise
  // return all of the predicted inputs.
  //
  // This method is not valid unless Update has been called at least once with
  // an input sent to model.
  //
  // The time stamps on the returned InputData will count from the latest input
  // sent to the modeler and increment by min_sample_dt as specified in Reset.
  //
  // Predictor may make use of the given screen location of the last model
  // output along with its velocity vector.
  std::vector<InputData> PredictedPoints(glm::vec2 last_modeled_point,
                                         glm::vec2 model_velocity) const;

  // If true, this predictor expects its output points to be sent through
  // TipDynamics' model. If not, the unmodified points should go to the
  // LineTool.
  virtual bool PredictionExpectsModeling() const { return true; }

 protected:
  virtual void UpdateImpl(const InputData& point, bool sent_to_modeler) {}
  virtual void ResetImpl(const Camera& cam, DurationS predict_interval,
                         DurationS min_sample_dt) {}

  virtual bool HasPrediction() const = 0;
  virtual std::vector<InputData> GeneratePredictedPoints(
      glm::vec2 last_modeled_point, glm::vec2 model_velocity) const = 0;

  InputData LastInput() const { return last_input_received_; }

 private:
  bool has_modeled_input_ = false;
  InputData last_input_received_;
  InputData last_input_sent_to_model_;
  DurationS min_sample_dt_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_PREDICTION_INPUT_PREDICTOR_H
