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

#ifndef INK_ENGINE_BRUSHES_TIP_SIZE_SCREEN_H_
#define INK_ENGINE_BRUSHES_TIP_SIZE_SCREEN_H_

#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

// TipSizeScreen defines the screen-coords size of the stroke from the input at
// a given moment. This is typically derived from TipSizeWorld's ToScreen()
// method.
struct TipSizeScreen {
  // Radius in screen coordinates.
  float radius;

  // For non-circular tips (e.g. chisel), the tip size may be determined by a
  // major and minor radius. Note that this doesn't necessarily mean to imply
  // that the tip is an ellipse, just that it has a shape with major and minor
  // axes.
  float radius_minor;

  static TipSizeScreen Smoothstep(TipSizeScreen from, TipSizeScreen to,
                                  float amount) {
    return TipSizeScreen(
        {util::Smoothstep(from.radius, to.radius, amount),
         util::Smoothstep(from.radius_minor, to.radius_minor, amount)});
  }
};

}  // namespace ink
#endif  // INK_ENGINE_BRUSHES_TIP_SIZE_SCREEN_H_
