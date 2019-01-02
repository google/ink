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

#ifndef INK_ENGINE_INPUT_PHYSICS_INPUT_MODELER_H_
#define INK_ENGINE_INPUT_PHYSICS_INPUT_MODELER_H_

#include <deque>
#include <string>
#include <vector>

#include "ink/engine/brushes/brushes.h"
#include "ink/engine/brushes/tip_dynamics.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_model_params.h"
#include "ink/engine/input/input_modeler.h"
#include "ink/engine/input/modeled_input.h"
#include "ink/engine/input/prediction/input_predictor.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/signal_filters/time_variant_moving_avg.h"

namespace ink {
namespace input {

// Class to model input.
//
// Given an input stream produces a new input stream following a physics-based
// model. The new input stream attempts to both smooth and correct user input
// generally trying to follow what input "should" look like vs minimizing error
// against what input "did" look like.
//
// The model runs behind actual input, to catch up to or go beyond use
// the "PredictModelResults" API, but be aware that any results from
// this are not stable across model updates.
//
// Assumes that input is well ordered, from a single source.
class PhysicsInputModeler : public InputModeler {
 public:
  using SharedDeps = service::Dependencies<InputPredictor>;

  explicit PhysicsInputModeler(std::shared_ptr<InputPredictor> predictor);

  void Reset(const Camera& cam, InputModelParams params) override;

  void SetParams(BrushParams::TipShapeParams params,
                 float base_world_radius) override;

  void AddInputToModel(InputData data) override;

  // Adding input to the model via "addInputToModel" creates a stream of input
  // results. There is not a 1:1 mapping between added input and result input,
  // meaning any one call to addInput will need n calls to popNextModelResult
  // to exhaust the output stream.
  //
  // All model outputs are stable and will not change in response to new input
  bool HasModelResult() const override;
  ModeledInput PopNextModelResult() override;

  // Model the given input prediction without changing the internal model state.
  std::vector<ModeledInput> PredictModelResults() const override;

  const Camera& camera() const override { return cam_; }
  std::string ToString() const override;

  template <typename OutputIt>
  static void GenModeledInput(const Camera& cam, const InputModelParams& params,
                              TipDynamics* dynamics, const InputData& data,
                              const InputData& last_input_sent_to_model,
                              OutputIt output);

 private:
  void Init();
  void FilterWobble(InputData* data);

  std::shared_ptr<InputPredictor> predictor_;

  Camera cam_;

  InputModelParams params_;
  TipDynamics dynamics_;
  InputData last_input_received_;
  InputData last_input_sent_to_model_;
  ModeledInput last_modeled_input_;

  // Results that haven't been observed yet
  std::deque<ModeledInput> modeled_input_;

  signal_filters::TimeVariantMovingAvg<glm::vec2, InputTimeS> avg_world_pos_;
  signal_filters::TimeVariantMovingAvg<float, InputTimeS> avg_cm_speed_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_PHYSICS_INPUT_MODELER_H_
