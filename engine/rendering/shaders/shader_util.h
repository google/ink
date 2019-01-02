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

#ifndef INK_ENGINE_RENDERING_SHADERS_SHADER_UTIL_H_
#define INK_ENGINE_RENDERING_SHADERS_SHADER_UTIL_H_

#include <cstdint>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"
#include "ink/engine/rendering/shaders/shader.h"

namespace ink {

template <typename M>
void DrawMesh(const ion::gfx::GraphicsManagerPtr& gl,
              const std::shared_ptr<MeshVBOProvider>& mesh_vbo_provider,
              const M& mesh, const InterleavedAttributeSet& attrs) {
  EXPECT(mesh_vbo_provider->HasVBO(mesh) || mesh.verts.empty());
  if (!mesh_vbo_provider->HasVBO(mesh)) {
    return;
  }
  IndexedVBO* vbo = mesh_vbo_provider->GetVBO(mesh);
  uint32_t index_count = vbo->GetNumIndices();
  if (index_count > 0) {
    vbo->Bind();
    attrs.BindVBO();
    gl->DrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);
    vbo->Unbind();
  }
}

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_SHADERS_SHADER_UTIL_H_
