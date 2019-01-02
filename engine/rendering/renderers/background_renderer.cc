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

#include "ink/engine/rendering/renderers/background_renderer.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

using glm::vec2;
using glm::vec4;

BackgroundRenderer::BackgroundRenderer(
    std::shared_ptr<GLResourceManager> gl_resources,
    std::shared_ptr<PageBounds> page_bounds)
    : gl_resources_(gl_resources),
      page_bounds_(page_bounds),
      renderer_(gl_resources) {}

void BackgroundRenderer::Draw(const Camera& cam, FrameTimeS draw_time) const {
  ImageBackgroundState* image_background = nullptr;
  if (gl_resources_->background_state->GetImage(&image_background)) {
    if (!image_background->HasFirstInstanceWorldCoords()) {
      SLOG(SLOG_DRAWING, "not drawing a background image with no location");
      return;
    }
    const TextureInfo& texture_info = image_background->TextureHandle();
    Texture* texture = nullptr;
    if (!gl_resources_->texture_manager->GetTexture(texture_info, &texture)) {
      // Background texture isn't available.
      return;
    }
    renderer_.Draw(
        cam, texture, blit_attrs::Blit(image_background->ImageFilterEffect()),
        cam.WorldRotRect(),
        RotRect(image_background->FirstInstanceWorldCoords()).InvertYAxis());
  } else {
    vec4 bgc = gl_resources_->background_state->GetColor();
    gl_resources_->gl->ClearColor(bgc.r, bgc.g, bgc.b, bgc.a);
    gl_resources_->gl->Clear(GL_COLOR_BUFFER_BIT);
  }
}
}  // namespace ink
