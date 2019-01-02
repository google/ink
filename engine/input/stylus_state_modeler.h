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

#ifndef INK_ENGINE_INPUT_STYLUS_STATE_MODELER_H_
#define INK_ENGINE_INPUT_STYLUS_STATE_MODELER_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/input_data.h"

namespace ink {
namespace input {

struct StylusState {
  float pressure;
  float tilt;
  float orientation;
};

static constexpr StylusState kStylusStateUnknown = StylusState({-1, 0, 0});

// Stylus state modeler takes in raw input and allows for querying for the
// pressure/tilt/orientation of novel (modeled) points.
class StylusStateModeler {
 public:
  StylusStateModeler() {}

  // Add the given raw input data to the model.
  void AddInputToModel(const input::InputData& input);

  // Clear stored input.
  void Clear();

  // Return the StylusState for the given point based on available data, or
  // kStateUnknown if no data is available. Points MUST be searched in
  // chronological order, otherwise behavior is undefined.
  StylusState Query(glm::vec2 point);

 private:
  // Returns the squared distance from the point to the nearest point in the
  // segment from points_[index] to points_[index + 1].
  float SquaredDistanceToSegment(int index, glm::vec2 point);

  std::vector<glm::vec2> points_;
  std::vector<StylusState> states_;
  // Array index of the most recent query match in this line.
  int index_ = 0;

  // Bookkeeping used to collect the first two non-identical points for
  // pseudo-stylus orientation computation.
  enum class OrientationState { kEmpty, kOnePoint, kReady };
  OrientationState orientation_state_ = OrientationState::kEmpty;
  glm::vec2 orientation_point_{0, 0};
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_STYLUS_STATE_MODELER_H_
