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

#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "geo/render/ion/gfx/graphicsmanager.h"
#include "third_party/absl/base/optimization.h"
#include "third_party/absl/container/flat_hash_map.h"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/util/unique_void_ptr.h"

namespace ink {

namespace {
constexpr uint16_t kMaxVboIndex = std::numeric_limits<uint16_t>::max();

// Using the given mesh's index, partition the given vertices, in triples, into
// separate IndexedVBOs, such that no resulting VBO has more than
// vbo_split_threshold verts.
template <typename MeshType, typename VertexType>
std::unique_ptr<MeshVBOProvider::VboVec> Partition(
    uint32_t vbo_split_threshold, ion::gfx::GraphicsManagerPtr gl,
    const MeshType &mesh, const std::vector<VertexType> &verts, GLenum usage) {
  ASSERT(vbo_split_threshold <= kMaxVboIndex);
  ASSERT(mesh.IndexSize() % 3 == 0);
  auto vbo_vec = absl::make_unique<MeshVBOProvider::VboVec>();

  // Trivial partition: everything goes into 1 IndexedVBO.
  if (mesh.IndexSize() < vbo_split_threshold) {
    vbo_vec->emplace_back(gl, mesh.Index16(), verts, usage);
    return vbo_vec;
  }

  SLOG(SLOG_GPU_OBJ_CREATION, "partitioning a mesh");
  size_t triangle_count = mesh.IndexSize() / 3;
  std::vector<VertexType> sub_verts;
  sub_verts.reserve(vbo_split_threshold);
  std::vector<uint16_t> sub_index;
  absl::flat_hash_map<uint32_t, uint16_t> global_to_local;
  global_to_local.reserve(vbo_split_threshold);
  auto map_vert = [&global_to_local, &verts, &sub_verts](uint32_t global) {
    auto p = global_to_local.find(global);
    if (p != global_to_local.end()) {
      return p->second;
    }
    auto result = static_cast<uint16_t>(sub_verts.size());
    global_to_local.insert({global, result});
    sub_verts.emplace_back(verts[global]);
    return result;
  };
  for (size_t t = 0; t < triangle_count; t++) {
    sub_index.emplace_back(map_vert(mesh.IndexAt(t * 3)));
    sub_index.emplace_back(map_vert(mesh.IndexAt(t * 3 + 1)));
    sub_index.emplace_back(map_vert(mesh.IndexAt(t * 3 + 2)));
    if (sub_verts.size() > vbo_split_threshold - 3) {
      // No room for another triangle.
      vbo_vec->emplace_back(gl, sub_index, sub_verts, usage);
      sub_verts.clear();
      sub_index.clear();
      global_to_local.clear();
    }
  }
  if (!sub_verts.empty()) {
    vbo_vec->emplace_back(gl, sub_index, sub_verts, usage);
  }
  return vbo_vec;
}
}  // namespace

MeshVBOProvider::MeshVBOProvider(const ion::gfx::GraphicsManagerPtr &gl)
    : gl_(gl), vbo_split_threshold_(kMaxVboIndex) {}

void MeshVBOProvider::GenVBOs(Mesh *m, GLenum usage) {
  ASSERT(!HasVBOs(*m));
  if (ABSL_PREDICT_TRUE(!m->verts.empty())) {
    ASSERT(!m->idx.empty());
    SetVBOs(m, Partition(vbo_split_threshold_, gl_, *m, m->verts, usage));
  }
}

void MeshVBOProvider::ExtendVBOs(Mesh *m, GLenum usage) {
  if (!HasVBOs(*m)) {
    GenVBOs(m, usage);
    return;
  }
  // Update indices and vertices.
  VboVec *vbos = GetVBOs(*m);
  ASSERT(vbos->size() == 1);
  (*vbos)[0].SetData(m->Index16(), m->verts);
}

void MeshVBOProvider::ReplaceVBOs(Mesh *m, GLenum usage) {
  if (HasVBOs(*m)) {
    GetVBOs(*m)->clear();
  }
  GenVBOs(m, usage);
}

void MeshVBOProvider::GenVBOs(OptimizedMesh *m, GLenum usage) {
  switch (m->verts.GetFormat()) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      SetVBOs(m, Partition(vbo_split_threshold_, gl_, *m, m->verts.Vec3Data(),
                           usage));
      break;
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      SetVBOs(m, Partition(vbo_split_threshold_, gl_, *m, m->verts.Vec2Data(),
                           usage));
      break;
    case VertFormat::x12y12:
      SetVBOs(m, Partition(vbo_split_threshold_, gl_, *m, m->verts.FloatData(),
                           usage));
      break;
  }
}

}  // namespace ink
