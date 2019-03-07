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

#ifndef INK_ENGINE_GEOMETRY_MESH_GL_INDEXED_VBO_H_
#define INK_ENGINE_GEOMETRY_MESH_GL_INDEXED_VBO_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/geometry/mesh/gl/vbo.h"
#include "ink/engine/gl.h"

namespace ink {

class IndexedVBO {
 public:
  template <typename VertexType>
  IndexedVBO(ion::gfx::GraphicsManagerPtr gl,
             const std::vector<uint16_t>& indices,
             const std::vector<VertexType>& verts, GLenum usage)
      : index_vbo_(gl, indices, usage, GL_ELEMENT_ARRAY_BUFFER),
        vertex_vbo_(gl, verts, usage, GL_ARRAY_BUFFER) {
    ASSERT(*std::max_element(indices.begin(), indices.end()) < verts.size());
  }

  // Disallow copy and assign.
  IndexedVBO(const IndexedVBO&) = delete;
  IndexedVBO& operator=(const IndexedVBO&) = delete;
  // But allow move and move assignment.
  IndexedVBO(IndexedVBO&&) = default;
  IndexedVBO& operator=(IndexedVBO&&) = default;

  void Bind() const;
  void Unbind() const;

  // Sets indices in indexVBO_ and elements in vertexVBO_ by appending all
  // elements beyond current size of each VBO.  VBOs capacity will grow as
  // needed to accommodate new elements.
  //
  // Warning: indices/elements should contain all existing data in the VBO,
  // plus new data.
  // If there is insufficient capacity and indexedVBO_/elementsVBO_ must be
  // grown, the indices/elements that were already in the VBOs need to be
  // rebuffered.
  template <typename VertexType>
  void SetData(const std::vector<uint16_t>& indices,
               const std::vector<VertexType>& elements) {
    index_vbo_.SetData(indices);
    vertex_vbo_.SetData(elements);
    ASSERT(*std::max_element(indices.begin(), indices.end()) <
           vertex_vbo_.GetTypedSize<VertexType>());
  }

  const VBO* GetIndices() const;
  const VBO* GetVertices() const;

  // Returns the number of indices if each index has size = sizeof(IndexType)
  size_t GetNumIndices() const { return index_vbo_.GetTypedSize<uint16_t>(); }

  // Returns the number of vertices if each vertex has size = sizeof(VertexType)
  template <typename VertexType>
  size_t GetNumVertices() const {
    return vertex_vbo_.GetTypedSize<VertexType>();
  }

  void RemoveAll() {
    vertex_vbo_.RemoveAll();
    index_vbo_.RemoveAll();
  }

 private:
  VBO index_vbo_;
  VBO vertex_vbo_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_GL_INDEXED_VBO_H_
