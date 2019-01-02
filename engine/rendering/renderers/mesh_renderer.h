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

#ifndef INK_ENGINE_RENDERING_RENDERERS_MESH_RENDERER_H_
#define INK_ENGINE_RENDERING_RENDERERS_MESH_RENDERER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/rendering/shaders/packed_mesh_shaders.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class MeshRenderer {
 public:
  explicit MeshRenderer(const service::UncheckedRegistry& registry);
  explicit MeshRenderer(std::shared_ptr<GLResourceManager> gl_resources);
  void Draw(const Camera& cam, FrameTimeS draw_time, const Mesh& mesh) const;
  void Draw(const Camera& cam, FrameTimeS draw_time,
            const OptimizedMesh& mesh) const;

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_RENDERERS_MESH_RENDERER_H_
