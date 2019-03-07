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

#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"

#include <cstddef>
#include <vector>

#include "third_party/absl/types/variant.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

// This is a helper class for TexturedQuadRenderer. Its purpose is:
// 1) Provide visitor functions for blit_attrs::BlitAttrs
// 2) Provide a capturing context for additional arguments to the
//    BlitAttrs visitor functions. (Use as you would std::bind)
//
// Functions on this class should be thought of as private members of
// TexturedQuadRenderer, private data as function parameters.
//
// Instances of this class should not be kept around. Expensive-to-create
// data should be held in TexturedQuadRenderer and accessed through the
// renderer_ pointer
class TexturedQuadRenderWorker {
 public:
  TexturedQuadRenderWorker(TexturedQuadRenderer const* renderer, Camera cam,
                           const Texture* texture, const RotRect& quad_world,
                           const glm::mat4& world_to_uv)
      : renderer_(renderer),
        cam_(cam),
        texture_(texture),  // Safe to store the ptr since we are short lived.
        quad_world_(quad_world),
        world_to_uv_(world_to_uv) {}

  void operator()(const blit_attrs::Blit& params) const {
    MakeRectangleMesh(&renderer_->mesh_, quad_world_, glm::vec4{1},
                      world_to_uv_);
    renderer_->gl_resources_->mesh_vbo_provider->ReplaceVBOs(&renderer_->mesh_,
                                                             GL_DYNAMIC_DRAW);

    renderer_->gl_resources_->gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    texture_->Bind(GL_TEXTURE0);

    const auto& shader =
        renderer_->gl_resources_->shader_manager->GetShaderForEffect(
            params.effect);
    shader.Use(cam_);
    shader.Draw(renderer_->mesh_);
    shader.Unuse();
  }

  void operator()(const blit_attrs::BlitMask& params) const {
    auto color = params.mask_to_background
                     ? renderer_->gl_resources_->background_state->GetColor()
                     : params.mask_to_color;
    MakeRectangleMesh(&renderer_->mesh_, quad_world_, color, world_to_uv_);
    renderer_->gl_resources_->mesh_vbo_provider->ReplaceVBOs(&renderer_->mesh_,
                                                             GL_DYNAMIC_DRAW);

    renderer_->gl_resources_->gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    texture_->Bind(GL_TEXTURE0);
    const auto& shader =
        renderer_->gl_resources_->shader_manager->TexturedMaskShader();
    shader.Use(cam_);
    shader.Draw(renderer_->mesh_);
    shader.Unuse();
  }

  void operator()(const blit_attrs::BlitColorOverride& params) const {
    MakeRectangleMesh(&renderer_->mesh_, quad_world_, params.color_multiplier,
                      world_to_uv_);
    renderer_->gl_resources_->mesh_vbo_provider->ReplaceVBOs(&renderer_->mesh_,
                                                             GL_DYNAMIC_DRAW);

    renderer_->gl_resources_->gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    texture_->Bind(GL_TEXTURE0);
    const auto& shader =
        renderer_->gl_resources_->shader_manager->TexturedColorTintShader();
    shader.Use(cam_);
    shader.Draw(renderer_->mesh_);
    shader.Unuse();
  }

  void operator()(const blit_attrs::BlitMotionBlur& params) const {
    MakeRectangleMesh(&renderer_->mesh_, quad_world_, glm::vec4{1},
                      world_to_uv_);

    auto world_to_prev_uv = world_to_uv_ * params.transform_new_to_old;
    for (auto& vertex : renderer_->mesh_.verts)
      vertex.texture_coords_from =
          geometry::Transform(vertex.position, world_to_prev_uv);
    renderer_->gl_resources_->mesh_vbo_provider->ReplaceVBOs(&renderer_->mesh_,
                                                             GL_DYNAMIC_DRAW);

    GLASSERT_NO_ERROR(renderer_->gl_resources_->gl);
    renderer_->gl_resources_->gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    texture_->Bind(GL_TEXTURE0);
    const auto& shader =
        renderer_->gl_resources_->shader_manager->TexturedMotionBlurShader();
    shader.Use(cam_);
    shader.Draw(renderer_->mesh_);
    shader.Unuse();
  }

 private:
  TexturedQuadRenderer const* renderer_;
  Camera cam_;
  const Texture* texture_;
  RotRect quad_world_;
  glm::mat4 world_to_uv_{1};
};

TexturedQuadRenderer::TexturedQuadRenderer(
    std::shared_ptr<GLResourceManager> gl_resources)
    : gl_resources_(gl_resources) {}

void TexturedQuadRenderer::Draw(const Camera& cam, const Texture* texture,
                                const blit_attrs::BlitAttrs& attrs,
                                const RotRect& quad_world,
                                const glm::mat4& world_to_uv) const {
  GLASSERT_NO_ERROR(gl_resources_->gl);
  absl::visit(
      TexturedQuadRenderWorker(this, cam, texture, quad_world, world_to_uv),
      attrs);
  GLASSERT_NO_ERROR(gl_resources_->gl);
}

}  // namespace ink
