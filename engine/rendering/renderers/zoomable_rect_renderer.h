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

#ifndef INK_ENGINE_RENDERING_RENDERERS_ZOOMABLE_RECT_RENDERER_H_
#define INK_ENGINE_RENDERING_RENDERERS_ZOOMABLE_RECT_RENDERER_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/rendering/shaders/packed_mesh_shaders.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class ZoomableRectRenderer {
 public:
  explicit ZoomableRectRenderer(
      std::shared_ptr<GLResourceManager> gl_resources);

  void Draw(const Camera& cam, FrameTimeS draw_time,
            const Rect& object_worldspace_bounds,
            absl::string_view base_texture_uri) const;

  // ZoomableRectRenderer is neither copyable nor movable.
  ZoomableRectRenderer(const ZoomableRectRenderer&) = delete;
  ZoomableRectRenderer& operator=(const ZoomableRectRenderer&) = delete;

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;
  ShapeRenderer shape_renderer_;
  Rect rectangle_mesh_bounds_;
  std::unique_ptr<OptimizedMesh> rectangle_mesh_;
  mutable Shape white_rect_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_RENDERERS_ZOOMABLE_RECT_RENDERER_H_
