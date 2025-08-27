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

#ifndef INK_STROKES_INTERNAL_MUTABLE_MULTI_MESH_H_
#define INK_STROKES_INTERNAL_MUTABLE_MULTI_MESH_H_

#include <array>
#include <cstdint>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_index_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {

// Wraps a set of `MutableMesh`es (with 16-bit indices), presenting them as if
// they were a single mutable mesh with 32-bit indices, and splitting off
// partitions as necessary to prevent any one `MutableMesh` from getting too
// full.
class MutableMultiMesh {
 public:
  // Constructs an empty set of mutable meshes that will use the given format.
  explicit MutableMultiMesh(const MeshFormat& format);
  // Constructs an empty set of mutable meshes that will use the given format,
  // and that will split off a new partition whenever the last mesh has at least
  // the given number of vertices.
  MutableMultiMesh(const MeshFormat& format, uint16_t partition_after);

  const MeshFormat& Format() const { return format_; }
  absl::Span<const MutableMesh> GetMeshes() const { return meshes_; }

  // Removes all triangles, vertices, and partitions.
  void Clear();

  // Returns the number of vertices in the multi-mesh. Each vertex may exist in
  // more than one partition, so this may be less than the sum of the vertex
  // counts of `GetMeshes()`.
  uint32_t VertexCount() const;
  // Returns the number of triangles in the multi-mesh. Each triangle exists in
  // exactly one partition, so this will always be equal to the sum of the
  // triangle counts of `GetMeshes()`.
  uint32_t TriangleCount() const;

  Point VertexPosition(uint32_t vertex_index) const;
  Triangle GetTriangle(uint32_t triangle_index) const;
  std::array<uint32_t, 3> TriangleIndices(uint32_t triangle_index) const;

  void AppendVertex(Point position);
  void SetVertexPosition(uint32_t vertex_index, Point position);
  void SetFloatVertexAttribute(uint32_t vertex_index, uint32_t attribute_index,
                               SmallArray<float, 4> value);

  void AppendTriangleIndices(const std::array<uint32_t, 3>& vertex_indices);
  void SetTriangleIndices(uint32_t triangle_index,
                          const std::array<uint32_t, 3>& vertex_indices);
  void InsertTriangleIndices(uint32_t triangle_index,
                             const std::array<uint32_t, 3>& vertex_indices);

  void TruncateTriangles(uint32_t new_triangle_count);
  void TruncateVertices(uint32_t new_vertex_count);

 private:
  struct Partition {
    // This vector maps the mesh's 16-bit vertex indices to the 32-bit vertex
    // indices that appear in this partition. Note that the same 32-bit vertex
    // index may appear in multiple partitions.
    std::vector<uint32_t> vertex_indices;
    // The total number of triangles in all previous partitions.
    uint32_t previous_triangle_count;
  };

  void AddNewPartition();
  // Returns the partition index and mesh-local vertex index for one of the
  // partitions that contains the specified vertex in the multi-mesh.
  VertexIndexPair GetPartitionVertex(uint32_t vertex_index) const;
  // Returns the partition index and mesh-local triangle index for the partition
  // that contains the specified triangle in the multi-mesh.
  TriangleIndexPair GetPartitionTriangle(uint32_t triangle_index) const;
  // TODO: b/295166196 - Once `MutableMesh` uses uint16_t for its vertex
  // indices, change this return type to `std::array<uint16_t, 3>`.
  std::array<uint32_t, 3> CopyVerticesIntoPartition(
      const std::array<uint32_t, 3>& vertex_indices, uint16_t partition_index);
  uint16_t CopyVertexIntoPartition(uint32_t vertex_index,
                                   uint16_t partition_index);

  std::vector<MutableMesh> meshes_;
  std::vector<Partition> partitions_;
  // This vector maps 32-bit vertex indices to sets of (partition_index,
  // mesh_vertex_index) pairs. It maps to sets of these pairs (instead of to a
  // single such pair) because the same vertex can appear in multiple
  // partitions. In practice, it can appear in at most two partitions, so we
  // limit the set size to 2.
  using MeshVertexIndices = absl::InlinedVector<VertexIndexPair, 1>;
  std::vector<MeshVertexIndices> mesh_vertex_indices_;
  // The format to use for all of the underlying meshes.
  MeshFormat format_;
  // Once a mesh has at least this many vertices, start a new partition.
  uint16_t partition_after_;
};

inline uint32_t MutableMultiMesh::VertexCount() const {
  return mesh_vertex_indices_.size();
}

inline VertexIndexPair MutableMultiMesh::GetPartitionVertex(
    uint32_t vertex_index) const {
  ABSL_DCHECK_LT(vertex_index, VertexCount());
  return mesh_vertex_indices_[vertex_index][0];
}

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_MUTABLE_MULTI_MESH_H_
