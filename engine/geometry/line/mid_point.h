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

#ifndef INK_ENGINE_GEOMETRY_LINE_MID_POINT_H_
#define INK_ENGINE_GEOMETRY_LINE_MID_POINT_H_

#include "ink/engine/brushes/size/tip_size_screen.h"
#include "ink/engine/input/stylus_state_modeler.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// A MidPoint is a modeled input in screen coordinates that is being extruded
// as part of a FatLine. It includes the screen position of the extruded shape
// along with the necessary additional information used by TipModel and its
// childen to compute the FatLine's outline vertices.
struct MidPoint {
  glm::vec2 screen_position{0, 0};
  TipSizeScreen tip_size;
  InputTimeS time_sec;
  input::StylusState stylus_state;

  MidPoint() = default;
  MidPoint(glm::vec2 screen_position_in, TipSizeScreen tip_size_in,
           InputTimeS time_sec_in,
           input::StylusState stylus_state_in = input::kStylusStateUnknown)
      : screen_position(screen_position_in),
        tip_size(tip_size_in),
        time_sec(time_sec_in),
        stylus_state(stylus_state_in) {}
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_LINE_MID_POINT_H_
