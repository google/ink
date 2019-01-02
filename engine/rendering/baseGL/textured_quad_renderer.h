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

#ifndef INK_ENGINE_RENDERING_BASEGL_TEXTURED_QUAD_RENDERER_H_
#define INK_ENGINE_RENDERING_BASEGL_TEXTURED_QUAD_RENDERER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/shaders/textured_shader.h"

namespace ink {

// Helper class for blitting textured quads
class TexturedQuadRenderer {
 public:
  explicit TexturedQuadRenderer(
      std::shared_ptr<GLResourceManager> gl_resources);

  // Disallow copy and assign.
  TexturedQuadRenderer(const TexturedQuadRenderer&) = delete;
  TexturedQuadRenderer& operator=(const TexturedQuadRenderer&) = delete;

  // Draws quad_world with the given texture. Each vertex's texture-coordinates
  // are calculated by applying world_to_uv to its world-coordinate position.
  // Note: For textures created from bitmaps, the point (0, 0) in
  // texture-coordinates is actually the top-left corner. To display it in the
  // correct orientation, the world_to_uv transform must invert the y-axis.
  void Draw(const Camera& cam, const Texture* texture,
            const blit_attrs::BlitAttrs& attrs, const RotRect& quad_world,
            const glm::mat4& world_to_uv) const;

  // This convenience function draws quad_world, calculating the appropriate
  // transformation such that texture_bounds_world corresponds to the rectangle
  // from (0, 0) to (1, 1) in texture-coordinates.
  // Informally, you can think of stretching the texture to cover
  // texture_bounds_world and cutting out quad_world.
  // Note: As above, textures created from bitmaps may be upside-down -- this
  // can be corrected by passing in texture_bounds_world.InvertYAxis() instead
  // of texture_bounds_world.
  void Draw(const Camera& cam, const Texture* texture,
            const blit_attrs::BlitAttrs& attrs, const RotRect& quad_world,
            const RotRect& texture_bounds_world) const {
    Draw(cam, texture, attrs, quad_world,
         texture_bounds_world.CalcTransformTo(RotRect({.5, .5}, {1, 1}, 0)));
  }

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;

  // cached to avoid gl reallocs
  mutable Mesh mesh_;

  friend class TexturedQuadRenderWorker;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_BASEGL_TEXTURED_QUAD_RENDERER_H_
