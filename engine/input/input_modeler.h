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

#ifndef INK_ENGINE_INPUT_INPUT_MODELER_H_
#define INK_ENGINE_INPUT_INPUT_MODELER_H_

#include <deque>
#include <string>
#include <vector>

#include "ink/engine/brushes/brushes.h"
#include "ink/engine/brushes/tip_dynamics.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_model_params.h"
#include "ink/engine/input/modeled_input.h"
#include "ink/engine/input/prediction/input_predictor.h"

namespace ink {
namespace input {

// Class to model input.
//
// Given an input stream produces a new input stream.
//
// Assumes that input is well ordered, from a single source.
class InputModeler {
 public:
  virtual ~InputModeler() {}

  // Reset to initial state.
  virtual void Reset(const Camera& cam, InputModelParams params) = 0;

  virtual void SetParams(BrushParams::TipShapeParams params,
                         float base_world_radius) = 0;

  virtual void AddInputToModel(InputData data) = 0;

  // Adding input to the model via "addInputToModel" creates a stream of input
  // results. There is not a 1:1 mapping between added input and result input,
  // meaning any one call to addInput will need n calls to popNextModelResult
  // to exhaust the output stream.
  //
  // All model outputs are stable and will not change in response to new input.
  virtual bool HasModelResult() const = 0;
  virtual ModeledInput PopNextModelResult() = 0;

  // Model the given input prediction without changing the internal model state.
  virtual std::vector<ModeledInput> PredictModelResults() const = 0;

  virtual std::string ToString() const = 0;

  // Subclasses must store the camera provided by Reset and return it here.
  virtual const Camera& camera() const = 0;
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_MODELER_H_
