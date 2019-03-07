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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_DBRENDER_TARGET_H_
#define INK_ENGINE_RENDERING_COMPOSITING_DBRENDER_TARGET_H_

#include <memory>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/baseGL/render_target.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/time/logging_perf_timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

class DBRenderTarget {
 public:
  DBRenderTarget(std::shared_ptr<WallClockInterface> wall_clock_,
                 std::shared_ptr<GLResourceManager> gl_resources,
                 TextureMapping front_buffer_mapping = TextureMapping::Nearest);
  void Resize(glm::ivec2 size);
  glm::ivec2 GetSize() const { return front_.GetSize(); }
  Rect Bounds() const { return front_.Bounds(); }

  // Takes the image from the front buffer at buffer_source, and draws it at
  // world_dest on the bound surface. buffer_source is assumed to lie within the
  // front buffer's Bounds().
  void DrawFront(const Camera& cam, const blit_attrs::BlitAttrs& attrs,
                 const RotRect& buffer_source, const RotRect& world_dest) const;

  // This convenience function draws the entire front buffer such that it
  // covers the camera's visible window.
  void DrawFront(const Camera& cam, const blit_attrs::BlitAttrs& attrs) const;

  // Copies the contents of the back buffer into the front buffer.
  void BlitBackToFront(absl::optional<Rect> area = absl::nullopt);

  // Binds the back buffer as the current drawing surface.
  void BindBack() { back_.Bind(); }

  // Clears the back buffer, setting every pixel to {0, 0, 0, 0}.
  void ClearBack() { back_.Clear(); }

 private:
  RenderTarget front_, back_;

  // perf
  LoggingPerfTimer fb_blit_timer_;
  LoggingPerfTimer front_blit_timer_;
  LoggingPerfTimer front_combined_blit_timer_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_COMPOSITING_DBRENDER_TARGET_H_
