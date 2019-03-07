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

#include "ink/engine/realtime/modifiers/line_animation.h"

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

LinearPathAnimation::LinearPathAnimation(const glm::vec4& rgba,
                                         const glm::vec4& rgba_from,
                                         DurationS rgba_seconds,
                                         float dilation_from,
                                         DurationS dilation_seconds)
    : rgba_(RGBtoRGBPremultiplied(rgba)),
      rgba_from_(RGBtoRGBPremultiplied(rgba_from)),
      rgba_seconds_(rgba_seconds),
      dilation_from_(dilation_from),
      dilation_seconds_(dilation_seconds) {}

void LinearPathAnimation::ApplyToVert(Vertex* vert, glm::vec2 center_pt,
                                      float radius, DurationS time_since_tdown,
                                      const LineModParams& line_mod_params) {
  if (rgba_seconds_ != 0.0) {
    vert->color_from = rgba_from_;
    vert->color_timings =
        glm::vec2(static_cast<float>(time_since_tdown),
                  static_cast<float>(time_since_tdown + rgba_seconds_));
    vert->color = rgba_;
  }
  if (dilation_seconds_ != 0.0) {
    glm::vec2 center_to_current = vert->position - center_pt;
    vert->position_timings =
        glm::vec2(static_cast<float>(time_since_tdown),
                  static_cast<float>(time_since_tdown + dilation_seconds_));
    vert->position_from = center_pt + center_to_current * dilation_from_;
  }
}
}  // namespace ink
