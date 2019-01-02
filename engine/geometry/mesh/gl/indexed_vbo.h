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
  template <typename T>
  IndexedVBO(ion::gfx::GraphicsManagerPtr gl,
             const std::vector<uint16_t>& indices, const std::vector<T>& verts,
             GLenum usage)
      : index_vbo_(gl, indices, usage, GL_ELEMENT_ARRAY_BUFFER),
        vertex_vbo_(gl, verts, usage, GL_ARRAY_BUFFER) {
    ASSERT(*std::max_element(indices.begin(), indices.end()) < verts.size());
  }

  IndexedVBO(const ion::gfx::GraphicsManagerPtr& gl,
             size_t num_indices_capacity, size_t vertex_capacity_bytes,
             GLenum usage)
      : index_vbo_(gl, num_indices_capacity * sizeof(uint16_t), usage,
                   GL_ELEMENT_ARRAY_BUFFER),
        vertex_vbo_(gl, vertex_capacity_bytes, usage, GL_ARRAY_BUFFER) {}

  // Disallow copy and assign.
  IndexedVBO(const IndexedVBO&) = delete;
  IndexedVBO& operator=(const IndexedVBO&) = delete;

  void Bind();
  void Unbind();

  // Sets indices in indexVBO_ and elements in vertexVBO_ by appending all
  // elements beyond current size of each VBO.  VBOs capacity will grow as
  // needed to accommodate new elements.
  //
  // Warning: indices/elements should contain all existing data in the VBO,
  // plus new data.
  // If there is insufficient capacity and indexedVBO_/elementsVBO_ must be
  // grown, the indices/elements that were already in the VBOs need to be
  // rebuffered.
  template <typename T>
  void SetData(const std::vector<uint16_t>& indices,
               const std::vector<T>& elements) {
    index_vbo_.SetData(indices);
    vertex_vbo_.SetData(elements);
    ASSERT(*std::max_element(indices.begin(), indices.end()) <
           vertex_vbo_.GetTypedSize<T>());
  }

  VBO* GetIndices();
  VBO* GetVertices();

  // Returns the number of indices stored (in uint16_t, not bytes)
  size_t GetNumIndices() { return index_vbo_.GetTypedSize<uint16_t>(); }

  // Returns the number of vertices if each vertex has size = sizeof(T)
  template <typename T>
  size_t GetNumVertices() {
    return vertex_vbo_.GetTypedSize<T>();
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
