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

namespace ink {

void MeshVBOProvider::GenVBO(Mesh *m, GLenum usage) {
  ASSERT(!HasVBO(*m));
  if (!m->verts.empty()) {
    ASSERT(!m->idx.empty());
    SetVBO(m, new IndexedVBO(gl_, m->idx, m->verts, usage));
  }
}

void MeshVBOProvider::ExtendVBO(Mesh *m, GLenum usage) {
  if (!HasVBO(*m)) {
    GenVBO(m, usage);
    return;
  }
  // Update indices and vertices.
  GetVBO(*m)->SetData(m->idx, m->verts);
}

void MeshVBOProvider::ReplaceVBO(Mesh *m, GLenum usage) {
  if (HasVBO(*m)) {
    GetVBO(*m)->RemoveAll();
  }
  ExtendVBO(m, usage);
}

void MeshVBOProvider::GenVBO(OptimizedMesh *m, GLenum usage) {
  SetVBO(m, GenVBO(&m->verts, m->idx, usage));
}

IndexedVBO *MeshVBOProvider::GenVBO(PackedVertList *verts,
                                    const std::vector<uint16_t> &indices,
                                    GLenum usage) {
  switch (verts->GetFormat()) {
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return new IndexedVBO(gl_, indices, verts->vec2s, usage);
    case VertFormat::x12y12:
      return new IndexedVBO(gl_, indices, verts->floats, usage);
    case VertFormat::uncompressed:
      return new IndexedVBO(gl_, indices, verts->uncompressed_verts, usage);
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(verts->GetFormat()));
}

}  // namespace ink
