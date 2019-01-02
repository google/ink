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

#include "ink/engine/input/prediction/repeat_predictor.h"

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace input {

void RepeatPredictor::ResetImpl(const Camera& cam, DurationS predict_interval,
                                DurationS min_sample_dt) {
  n_points_ = static_cast<size_t>(std::max(
      std::trunc(static_cast<double>(predict_interval / min_sample_dt)), 1.0));
}

std::vector<input::InputData> RepeatPredictor::GeneratePredictedPoints(
    glm::vec2 last_modeled_point, glm::vec2 model_velocity) const {
  return std::vector<InputData>(n_points_, LastInput());
}

}  // namespace input
}  // namespace ink
