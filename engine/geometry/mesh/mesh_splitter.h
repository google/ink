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

#ifndef INK_ENGINE_GEOMETRY_MESH_MESH_SPLITTER_H_
#define INK_ENGINE_GEOMETRY_MESH_MESH_SPLITTER_H_

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/spatial/rtree.h"

namespace ink {

// This object is used to split a mesh with one or more other meshes, removing
// the areas where they intersect.
class MeshSplitter {
 public:
  // Constructs from the mesh to be split. All triangles in the mesh are
  // expected to be oriented counter-clockwise (see
  // Mesh::NormalizeTriangleOrientation()).
  // Note that base_mesh is an OptimizedMesh, instead of a Mesh. This is done
  // because the stroke-editing eraser may have many instances of MeshSplitter
  // at once -- enough that it can take up all of the available memory in a web
  // client without memory growth.
  explicit MeshSplitter(const OptimizedMesh &base_mesh);

  // Removes the areas of the base mesh that intersect the cutting mesh. All
  // triangles in the mesh are expected to be oriented counter-clockwise (see
  // Mesh::NormalizeTriangleOrientation()). Note that the texture, color, and
  // animation data on the cutting mesh are ignored.
  void Split(const Mesh &cutting_mesh);

  // Returns true if the base mesh was affected by the split operations.
  bool IsMeshChanged() const { return is_base_mesh_changed_; }

  // Returns true if the result mesh contains no triangles.
  bool IsResultEmpty() const { return rtree_->Size() == 0; }

  // Fetches the result of any splits performed so far. Returns true if the base
  // mesh was affected by the split operations; otherwise, returns false without
  // populating result_mesh.
  bool GetResult(Mesh *result_mesh) const;

 private:
  struct IndexedTriangle {
    // The triangle in the R-Tree -- this may be a triangle from the base mesh,
    // or the result of a split.
    geometry::Triangle triangle;
    // The index of the original triangle in base mesh, before it was split.
    // This is required to preserve the color and texture-coordinates.
    int original_index;
  };

  void InitializeRTree();

  OptimizedMesh base_mesh_;
  bool is_base_mesh_changed_;
  std::unique_ptr<spatial::RTree<IndexedTriangle>> rtree_;
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_MESH_MESH_SPLITTER_H_
