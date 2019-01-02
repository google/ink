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

#include "ink/engine/realtime/modifiers/line_modifier_factory.h"

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/realtime/modifiers/ballpoint.h"
#include "ink/engine/realtime/modifiers/eraser.h"
#include "ink/engine/realtime/modifiers/highlighter.h"
#include "ink/engine/realtime/modifiers/line_animation.h"
#include "ink/engine/realtime/modifiers/tiled_texture.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

std::unique_ptr<LineModifier> LineModifierFactory::Make(
    const BrushParams& brush_params, glm::vec4 rgba) {
  std::unique_ptr<LineModifier> res;
  switch (brush_params.line_modifier) {
    case BrushParams::LineModifier::HIGHLIGHTER:
      res = absl::make_unique<HighlighterModifier>(rgba);
      break;
    case BrushParams::LineModifier::ERASER:
      res = absl::make_unique<EraserModifier>(rgba);
      break;
    case BrushParams::LineModifier::BALLPOINT:
      res = absl::make_unique<BallpointModifier>(rgba);
      break;
    case BrushParams::LineModifier::PENCIL: {
      float scale = 1.0 / 125;
      glm::mat4 trans = glm::scale(glm::mat4{1}, glm::vec3{scale, scale, 1});
      res = absl::make_unique<TiledTextureModifier>(rgba, trans,
                                                    "inkbrush:pencil_000.png");
      break;
    }
    case BrushParams::LineModifier::CHARCOAL: {
      float scale = 1.0 / 100;
      glm::mat4 trans = glm::scale(glm::mat4{1}, glm::vec3{scale, scale, 1});
      res = absl::make_unique<TiledTextureModifier>(
          rgba, trans, "inkbrush:charcoal_000.png");
      break;
    }
    case BrushParams::LineModifier::NONE:
    default:
      res = absl::make_unique<LineModifier>(LineModParams(), rgba);
      break;
  }
  if (brush_params.animated) {
    res->animation_ = absl::make_unique<LinearPathAnimation>(
        rgba, brush_params.rgba_from, brush_params.rgba_seconds,
        brush_params.dilation_from, brush_params.dilation_seconds);
  }
  res->brush_params_ = brush_params;
  return res;
}

}  // namespace ink
