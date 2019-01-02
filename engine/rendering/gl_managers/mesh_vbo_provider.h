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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_MESH_VBO_PROVIDER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_MESH_VBO_PROVIDER_H_

#include <memory>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/unique_void_ptr.h"

namespace ink {

// Attaches IndexedVBOs to Meshes, without Meshes having to know about
// IndexedVBOs.
class MeshVBOProvider {
 public:
  explicit MeshVBOProvider(const ion::gfx::GraphicsManagerPtr& gl) : gl_(gl) {}

  // Check fails if m has verts but is not indexed, or m already has IndexedVBO.
  void GenVBO(Mesh* m, GLenum usage);

  // Appends all vertices and indices to m's VBO that have been added since last
  // call of GenVBO/UpdateVBO.  Resizes VBO as necessary.  If no VBO has been
  // created, creates a VBO with the given usage.
  void ExtendVBO(Mesh* m, GLenum usage);

  void ReplaceVBO(Mesh* m, GLenum usage);

  void GenVBO(OptimizedMesh* m, GLenum usage);

  void EnsureOnlyInVBO(OptimizedMesh* mesh, GLenum type) {
    if (!HasVBO(*mesh)) {
      GenVBO(mesh, type);
    }
    mesh->ClearCpuMemoryVerts();
  }

  template <typename M>
  bool HasVBO(const M& m) {
    return m.backend_vert_data.get();
  }

  template <typename M>
  IndexedVBO* GetVBO(const M& mesh) {
    ASSERT(HasVBO(mesh));
    return static_cast<IndexedVBO*>(mesh.backend_vert_data.get());
  }

  template <typename M>
  void SetVBO(M* mesh, IndexedVBO* vbo) {
    mesh->backend_vert_data = util::make_unique_void<IndexedVBO>(vbo);
  }

 private:
  IndexedVBO* GenVBO(PackedVertList* verts,
                     const std::vector<uint16_t>& indices, GLenum usage);

  ion::gfx::GraphicsManagerPtr gl_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_MESH_VBO_PROVIDER_H_
