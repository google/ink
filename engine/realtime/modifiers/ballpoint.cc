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

#include "ink/engine/realtime/modifiers/ballpoint.h"

#include "ink/engine/colors/colors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

using glm::vec2;
using glm::vec4;

BallpointModifier::BallpointModifier(const vec4& rgba)
    : LineModifier(MakeLineModParams(), rgba),
      opacity_interpolator_({vec2(0, 0.2), vec2(0.4, 0.7), vec2(0.6, 0.9),
                             vec2(0.9, 0.9), vec2(1.0, 1.0)}) {}

void BallpointModifier::OnAddVert(Vertex* vert, glm::vec2 center_pt,
                                  float radius, float pressure) {
  if (pressure < 0) {
    vert->color = RGBtoRGBPremultiplied(rgba_);
  } else {
    // If we have pressure data, interpolate opacity value.
    vec4 modified_rgba(rgba_);
    modified_rgba.a *= opacity_interpolator_.GetValue(pressure);
    vert->color = RGBtoRGBPremultiplied(modified_rgba);
  }
}

}  // namespace ink
