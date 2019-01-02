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

#ifndef INK_ENGINE_RENDERING_RENDERERS_BACKGROUND_RENDERER_H_
#define INK_ENGINE_RENDERING_RENDERERS_BACKGROUND_RENDERER_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class BackgroundRenderer : public IDrawable {
 public:
  BackgroundRenderer(std::shared_ptr<GLResourceManager> gl_resources,
                     std::shared_ptr<PageBounds> page_bounds_);

  // Disallow copy and assign.
  BackgroundRenderer(const BackgroundRenderer&) = delete;
  BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<PageBounds> page_bounds_;
  TexturedQuadRenderer renderer_;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_RENDERERS_BACKGROUND_RENDERER_H_
