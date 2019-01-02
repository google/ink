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

#include "ink/engine/realtime/modifiers/highlighter.h"

#include <memory>

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/realtime/modifiers/line_animation.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

HighlighterModifier::HighlighterModifier(const glm::vec4& rgba)
    : LineModifier(LineModParams(), rgba) {
  // mess with the passed in highlighter parameters to make it look
  // highlighter-y.
  rgba_.a = 0.2;

  glm::vec4 from_color = RGBtoHSV(RGBtoRGBPremultiplied(rgba_));
  from_color.y *= 1.3f;  // oversaturate
  from_color = RGBPremultipliedtoRGB(HSVtoRGB(from_color));

  animation_.reset(new LinearPathAnimation(rgba_, from_color,
                                           0.35,   // rgba_seconds
                                           1.05f,  // dilation from
                                           0.3));  // dilation seconds
}
}  // namespace ink
