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

#include "ink/engine/rendering/renderers/mesh_renderer.h"

#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/shader_manager.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/rendering/shaders/packed_mesh_shaders.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

MeshRenderer::MeshRenderer(const service::UncheckedRegistry& registry)
    : MeshRenderer(registry.GetShared<GLResourceManager>()) {}

MeshRenderer::MeshRenderer(std::shared_ptr<GLResourceManager> gl_resources)
    : gl_resources_(std::move(gl_resources)) {}

void MeshRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                        const OptimizedMesh& mesh) const {
  mesh.Validate();
  const auto& shader = gl_resources_->shader_manager->PackedShader();
  shader.Use(cam, mesh);
  shader.Draw(mesh);
  shader.Unuse(mesh);
}

void MeshRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                        const Mesh& mesh) const {
  if (mesh.shader_metadata.IsParticle()) {
    const auto& shader = gl_resources_->shader_manager->ParticleShader();
    shader.Use(cam, draw_time - mesh.shader_metadata.InitTime());
    shader.Draw(mesh);
    shader.Unuse();
  } else if (mesh.shader_metadata.IsEraser()) {
    if (gl_resources_->background_state->IsImage()) {
      const auto& shader =
          gl_resources_->shader_manager->TexturedEraserShader();
      shader.Use(cam);
      shader.Draw(mesh);
      shader.Unuse();
    } else {
      const auto& shader = gl_resources_->shader_manager->SolidEraserShader();
      shader.Use(cam);
      shader.Draw(mesh);
      shader.Unuse();
    }
  } else if (mesh.texture != nullptr) {
    const auto& shader = gl_resources_->shader_manager->VertTexturedShader();
    shader.Use(cam);
    shader.Draw(mesh);
    shader.Unuse();
  } else if (mesh.shader_metadata.IsAnimated()) {
    const auto& shader = gl_resources_->shader_manager->AnimatedShader();
    shader.Use(cam, draw_time - mesh.shader_metadata.InitTime());
    shader.Draw(mesh);
    shader.Unuse();
  } else {
    const auto& shader = gl_resources_->shader_manager->VertColoredShader();
    shader.Use(cam);
    shader.Draw(mesh);
    shader.Unuse();
  }
}

}  // namespace ink
