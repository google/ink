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

#ifndef INK_ENGINE_INPUT_MODELED_INPUT_H_
#define INK_ENGINE_INPUT_MODELED_INPUT_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/size/tip_size_world.h"
#include "ink/engine/input/stylus_state_modeler.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace input {

struct ModeledInput {
  glm::vec2 world_pos{0, 0};
  InputTimeS time;
  TipSizeWorld tip_size;
  StylusState stylus_state;

  ModeledInput()
      : ModeledInput(glm::vec2(0, 0), InputTimeS(0),
                     TipSizeWorld::FixedRadius(0), StylusState{0, 0, 0}) {}

  ModeledInput(glm::vec2 world_pos_in, InputTimeS time_in,
               TipSizeWorld tip_size_in, StylusState stylus_state_in)
      : world_pos(world_pos_in),
        time(time_in),
        tip_size(tip_size_in),
        stylus_state(stylus_state_in) {}
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_MODELED_INPUT_H_
