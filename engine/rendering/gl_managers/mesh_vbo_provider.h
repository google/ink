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
#include "third_party/absl/container/inlined_vector.h"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/unique_void_ptr.h"

namespace ink {

namespace vbo_provider_testing {
class TemporaryVBOSplitThreshold;
}  // namespace vbo_provider_testing

// Attaches IndexedVBOs to Meshes, without Meshes having to know about
// IndexedVBOs.
class MeshVBOProvider {
 public:
  // Most meshes have a single IndexedVBO. Some, having been modified with
  // stroke-editing eraser, require multiple VBOs.
  using VboVec = absl::InlinedVector<IndexedVBO, 1>;

  explicit MeshVBOProvider(const ion::gfx::GraphicsManagerPtr& gl);

  // Check fails if m has verts but is not indexed, or m already has a VboVec.
  void GenVBOs(Mesh* m, GLenum usage);

  // Appends all vertices and indices to m's VBO that have been added since last
  // call of GenVBO/UpdateVBO.  Resizes VBO as necessary.  If no VBO has been
  // created, creates a VBO with the given usage.
  // Does not support meshes with more than 64K vertices, and will assert-fail
  // if attempted.
  void ExtendVBOs(Mesh* m, GLenum usage);

  void ReplaceVBOs(Mesh* m, GLenum usage);

  void GenVBOs(OptimizedMesh* m, GLenum usage);

  void EnsureOnlyInVBO(OptimizedMesh* mesh, GLenum type) {
    if (!HasVBOs(*mesh)) {
      GenVBOs(mesh, type);
    }
    mesh->ClearCpuMemoryVerts();
  }

  template <typename M>
  bool HasVBOs(const M& m) {
    return m.backend_vert_data.get();
  }

  template <typename M>
  VboVec* GetVBOs(const M& mesh) {
    ASSERT(HasVBOs(mesh));
    return static_cast<VboVec*>(mesh.backend_vert_data.get());
  }

 private:
  template <typename M>
  void SetVBOs(M* mesh, std::unique_ptr<VboVec> vbos) {
    mesh->backend_vert_data = util::make_unique_void(vbos.release());
  }

  std::unique_ptr<VboVec> GenVBOs(PackedVertList* verts,
                                  const std::vector<uint16_t>& indices,
                                  GLenum usage);

  ion::gfx::GraphicsManagerPtr gl_;

  // The number of vertices that should trigger a partitioning into multiple
  // VBOs. For production use, this should always be set to
  // std::numeric_limits<uint16_t>::max().
  uint16_t vbo_split_threshold_;

  friend class vbo_provider_testing::TemporaryVBOSplitThreshold;
};

namespace vbo_provider_testing {
// RAII class to temporarily change the VBO split threshold of a
// MeshVBOProvider, for testing. When the object goes out of scope, the
// provider's split threshold is restored.
class TemporaryVBOSplitThreshold {
 public:
  TemporaryVBOSplitThreshold(MeshVBOProvider* provider, uint16_t threshold)
      : provider_(provider), saved_threshold_(provider_->vbo_split_threshold_) {
    provider_->vbo_split_threshold_ = threshold;
  }
  ~TemporaryVBOSplitThreshold() {
    provider_->vbo_split_threshold_ = saved_threshold_;
  }

 private:
  MeshVBOProvider* provider_;
  uint32_t saved_threshold_;
};
}  // namespace vbo_provider_testing

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_MESH_VBO_PROVIDER_H_
