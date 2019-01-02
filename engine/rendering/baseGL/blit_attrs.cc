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

#include "ink/engine/rendering/baseGL/blit_attrs.h"

namespace ink {

namespace blit_attrs {

static FilterEffect AdvanceEffect(FilterEffect f, int advance) {
  int num_shaders = static_cast<int>(FilterEffect::MaxValue);
  int f_idx = static_cast<int>(f);
  int target_idx = (f_idx + advance + num_shaders) % num_shaders;
  return static_cast<FilterEffect>(target_idx);
}

FilterEffect NextEffect(FilterEffect f) { return AdvanceEffect(f, 1); }

FilterEffect PreviousEffect(FilterEffect f) { return AdvanceEffect(f, -1); }

std::string FragShaderNameForEffect(FilterEffect e) {
  switch (e) {
    case FilterEffect::None:
      return "TextureShaders/Textured.frag";
    case FilterEffect::BlackWhite:
      return "TextureShaders/TexturedBlackWhite.frag";
    case FilterEffect::Sepia:
      return "TextureShaders/TexturedSepia.frag";
    case FilterEffect::Nightvision:
      return "TextureShaders/TexturedNightvision.frag";
    case FilterEffect::Burn:
      return "TextureShaders/TexturedBurn.frag";
    case FilterEffect::Mimas:
      return "TextureShaders/TexturedMimas.frag";
    case FilterEffect::Saturate:
      return "TextureShaders/TexturedSaturate.frag";
    case FilterEffect::MaxValue:
      break;
  }

  ASSERT(false);
  return "TextureShaders/Textured.frag";
}

}  // namespace blit_attrs
}  // namespace ink
