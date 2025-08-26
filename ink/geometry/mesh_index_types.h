// Copyright 2025 Google LLC
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

#ifndef INK_GEOMETRY_MESH_INDEX_TYPES_H_
#define INK_GEOMETRY_MESH_INDEX_TYPES_H_

#include <cstdint>

namespace ink {

// A pair of indices identifying a vertex in a partitioned mesh, by referring to
// a vertex in one of the partitions.
struct VertexIndexPair {
  // The index of the mesh that the vertex belongs to.
  uint16_t mesh_index;
  // The index of the vertex within the mesh.
  uint16_t vertex_index;
};

// A pair of indices identifying a triangle in a partitioned mesh, by referring
// to a triangle in one of the partitions.
struct TriangleIndexPair {
  // The index of the mesh that the triangle belongs to.
  uint16_t mesh_index;
  // The index of the triangle within the mesh.
  uint16_t triangle_index;
};

}  // namespace ink

#endif  // INK_GEOMETRY_MESH_INDEX_TYPES_H_
