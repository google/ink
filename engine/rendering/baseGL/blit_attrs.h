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

#ifndef INK_ENGINE_RENDERING_BASEGL_BLIT_ATTRS_H_
#define INK_ENGINE_RENDERING_BASEGL_BLIT_ATTRS_H_

#include "third_party/absl/types/variant.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

namespace blit_attrs {

enum class FilterEffect : uint32_t {
  None = 0,
  BlackWhite = 1,
  Sepia = 2,
  Nightvision = 3,
  Burn = 4,
  Mimas = 5,
  Saturate = 6,
  MaxValue = 7,  // Used to get the length (not a valid value).
};

// Functions that let you advance through the effects cyclically:
// PreviousEffect(None) == Saturate
FilterEffect NextEffect(FilterEffect f);
FilterEffect PreviousEffect(FilterEffect f);

std::string FragShaderNameForEffect(FilterEffect e);

struct Blit {
  Blit() : effect(FilterEffect::None) {}
  explicit Blit(FilterEffect e) : effect(e) {}

  FilterEffect effect;
};

struct BlitMask {
  bool mask_to_background;
  glm::vec4 mask_to_color{0, 0, 0, 0};

  BlitMask() {}
  explicit BlitMask(bool mask_to_background)
      : mask_to_background(mask_to_background) {
    ASSERT(mask_to_background);
  }
  explicit BlitMask(glm::vec4 mask_to_color)
      : mask_to_background(false), mask_to_color(mask_to_color) {}
};

struct BlitColorOverride {
  // dst = src * colorMultiplier
  glm::vec4 color_multiplier{0, 0, 0, 0};

  BlitColorOverride() {}
  explicit BlitColorOverride(glm::vec4 color_multiplier)
      : color_multiplier(color_multiplier) {}
};

struct BlitMotionBlur {
  glm::mat4 transform_new_to_old{1};

  BlitMotionBlur() {}
  explicit BlitMotionBlur(glm::mat4 new_to_old)
      : transform_new_to_old(new_to_old) {}
};

using BlitAttrs =
    absl::variant<Blit, BlitMask, BlitColorOverride, BlitMotionBlur>;

}  // namespace blit_attrs

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_BASEGL_BLIT_ATTRS_H_
