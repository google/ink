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

#ifndef INK_ENGINE_RENDERING_RENDERERS_RECTANGLES_RENDERER_H_
#define INK_ENGINE_RENDERING_RENDERERS_RECTANGLES_RENDERER_H_

#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
class RectanglesRenderer {
 public:
  explicit RectanglesRenderer(
      const std::shared_ptr<GLResourceManager>& gl_resources);

  // Fills the given rectangles with the given solid color.
  void DrawRectangles(const std::vector<Rect>& rects, const glm::vec4& color,
                      const Camera& cam, FrameTimeS draw_time) const;

 private:
  Rect rect_mesh_bounds_;
  std::unique_ptr<MeshRenderer> renderer_;
  std::unique_ptr<OptimizedMesh> rect_mesh_;
  std::shared_ptr<GLResourceManager> gl_resources_;
};
}  // namespace ink
#endif  // INK_ENGINE_RENDERING_RENDERERS_RECTANGLES_RENDERER_H_
