#include "ink/strokes/internal/mutable_multi_mesh.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {

namespace {

// By default, fill each `MutableMesh` about 15/16 full before starting the next
// partition. It's important to leave some extra room, because we will sometimes
// need to go back and add some extra vertices to an already "full" partition.
constexpr uint16_t kDefaultPartitionAfter = 0xf000;

}  // namespace

MutableMultiMesh::MutableMultiMesh(const MeshFormat& format)
    : MutableMultiMesh(format, kDefaultPartitionAfter) {}

MutableMultiMesh::MutableMultiMesh(const MeshFormat& format,
                                   uint16_t partition_after)
    : format_(format), partition_after_(partition_after) {}

uint32_t MutableMultiMesh::TriangleCount() const {
  ABSL_DCHECK_EQ(meshes_.size(), partitions_.size());
  return meshes_.empty() ? 0
                         : meshes_.back().TriangleCount() +
                               partitions_.back().previous_triangle_count;
}

Point MutableMultiMesh::VertexPosition(uint32_t vertex_index) const {
  auto [partition_index, mesh_vertex_index] = GetPartitionVertex(vertex_index);
  return meshes_[partition_index].VertexPosition(mesh_vertex_index);
}

Triangle MutableMultiMesh::GetTriangle(uint32_t triangle_index) const {
  auto [partition_index, mesh_triangle_index] =
      GetPartitionTriangle(triangle_index);
  return meshes_[partition_index].GetTriangle(mesh_triangle_index);
}

std::array<uint32_t, 3> MutableMultiMesh::TriangleIndices(
    uint32_t triangle_index) const {
  auto [partition_index, mesh_triangle_index] =
      GetPartitionTriangle(triangle_index);
  std::array<uint32_t, 3> mesh_vertex_indices =
      meshes_[partition_index].TriangleIndices(mesh_triangle_index);
  const Partition& partition = partitions_[partition_index];
  return {
      partition.vertex_indices[mesh_vertex_indices[0]],
      partition.vertex_indices[mesh_vertex_indices[1]],
      partition.vertex_indices[mesh_vertex_indices[2]],
  };
}

void MutableMultiMesh::AppendVertex(Point position) {
  // Ensure that there is at least one partition, and that the last partition
  // has room for a new vertex.
  if (meshes_.empty()) {
    AddNewPartition();
  } else if (meshes_.back().VertexCount() >= partition_after_) {
    AddNewPartition();
  }

  MutableMesh& mesh = meshes_.back();
  uint16_t partition_index = partitions_.size() - 1;
  uint32_t vertex_index = mesh_vertex_indices_.size();
  uint16_t mesh_vertex_index = mesh.VertexCount();

  mesh.AppendVertex(position);
  partitions_[partition_index].vertex_indices.push_back(vertex_index);
  mesh_vertex_indices_.push_back(
      {VertexIndexPair{partition_index, mesh_vertex_index}});
}

void MutableMultiMesh::SetVertexPosition(uint32_t vertex_index,
                                         Point position) {
  ABSL_DCHECK_LT(vertex_index, VertexCount());
  for (auto [partition_index, mesh_vertex_index] :
       mesh_vertex_indices_[vertex_index]) {
    meshes_[partition_index].SetVertexPosition(mesh_vertex_index, position);
  }
}

void MutableMultiMesh::SetFloatVertexAttribute(uint32_t vertex_index,
                                               uint32_t attribute_index,
                                               SmallArray<float, 4> value) {
  ABSL_DCHECK_LT(vertex_index, VertexCount());
  for (auto [partition_index, mesh_vertex_index] :
       mesh_vertex_indices_[vertex_index]) {
    meshes_[partition_index].SetFloatVertexAttribute(mesh_vertex_index,
                                                     attribute_index, value);
  }
}

void MutableMultiMesh::AppendTriangleIndices(
    const std::array<uint32_t, 3>& vertex_indices) {
  // The vertices must already exist in the mesh.
  ABSL_DCHECK_LT(vertex_indices[0], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[1], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[2], VertexCount());
  // Since the mesh contains at least these three vertices, it must contain at
  // least one partition already.
  ABSL_DCHECK(!partitions_.empty());

  uint16_t partition_index = partitions_.size() - 1;
  std::array<uint32_t, 3> mesh_vertex_indices =
      CopyVerticesIntoPartition(vertex_indices, partition_index);
  meshes_[partition_index].AppendTriangleIndices(mesh_vertex_indices);
}

void MutableMultiMesh::SetTriangleIndices(
    uint32_t triangle_index, const std::array<uint32_t, 3>& vertex_indices) {
  // The triangle must already exist in the mesh.
  ABSL_DCHECK_LT(triangle_index, TriangleCount());
  // The vertices must already exist in the mesh.
  ABSL_DCHECK_LT(vertex_indices[0], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[1], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[2], VertexCount());

  auto [partition_index, mesh_triangle_index] =
      GetPartitionTriangle(triangle_index);
  std::array<uint32_t, 3> mesh_vertex_indices =
      CopyVerticesIntoPartition(vertex_indices, partition_index);
  meshes_[partition_index].SetTriangleIndices(mesh_triangle_index,
                                              mesh_vertex_indices);
}

void MutableMultiMesh::InsertTriangleIndices(
    uint32_t triangle_index, const std::array<uint32_t, 3>& vertex_indices) {
  // The triangle must already exist in the mesh, or be the new final triangle.
  ABSL_DCHECK_LE(triangle_index, TriangleCount());
  // The vertices must already exist in the mesh.
  ABSL_DCHECK_LT(vertex_indices[0], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[1], VertexCount());
  ABSL_DCHECK_LT(vertex_indices[2], VertexCount());

  // Inserting a triangle at the very end is the same as appending it.
  if (triangle_index == TriangleCount()) {
    AppendTriangleIndices(vertex_indices);
    return;
  }

  auto [partition_index, mesh_triangle_index] =
      GetPartitionTriangle(triangle_index);
  std::array<uint32_t, 3> mesh_vertex_indices =
      CopyVerticesIntoPartition(vertex_indices, partition_index);
  meshes_[partition_index].InsertTriangleIndices(mesh_triangle_index,
                                                 mesh_vertex_indices);
  for (size_t p = partition_index + 1; p < partitions_.size(); ++p) {
    ++partitions_[p].previous_triangle_count;
  }
}

void MutableMultiMesh::Resize(uint32_t new_vertex_count,
                              uint32_t new_triangle_count) {
  if (new_vertex_count == 0 && new_triangle_count == 0) {
    meshes_.clear();
    partitions_.clear();
    mesh_vertex_indices_.clear();
    return;
  }
  // Step 1: Resize triangles.
  uint32_t old_triangle_count = TriangleCount();
  if (new_triangle_count > old_triangle_count) {
    ResizeAppendTriangles(new_triangle_count - old_triangle_count);
  } else if (new_triangle_count < old_triangle_count) {
    ResizeTruncateTriangles(new_triangle_count);
  }
  // Step 2: Resize vertices.
  uint32_t old_vertex_count = VertexCount();
  if (new_vertex_count > old_vertex_count) {
    ResizeAppendVertices(new_vertex_count - old_vertex_count);
  } else if (new_vertex_count < old_vertex_count) {
    ResizeTruncateVertices(new_vertex_count);
  }
  // Step 3: Eliminate empty partitions.
  ABSL_DCHECK_EQ(meshes_.size(), partitions_.size());
  while (!meshes_.empty() && meshes_.back().IsEmpty()) {
    meshes_.pop_back();
    partitions_.pop_back();
  }
}

void MutableMultiMesh::ResizeAppendTriangles(uint32_t num_new_triangles) {
  ABSL_DCHECK_GT(num_new_triangles, 0);
  if (meshes_.empty()) {
    AddNewPartition();
  }
  MutableMesh& mesh = meshes_.back();
  mesh.Resize(mesh.VertexCount(), mesh.TriangleCount() + num_new_triangles);
}

void MutableMultiMesh::ResizeTruncateTriangles(uint32_t new_triangle_count) {
  // This private method only gets called with a `new_triangle_count` smaller
  // than `TriangleCount()`. This implies that this method is only called when
  // `TriangleCount()` is nonzero, and therefore when there is at least one
  // partition.
  ABSL_DCHECK_LT(new_triangle_count, TriangleCount());
  ABSL_DCHECK_EQ(meshes_.size(), partitions_.size());
  ABSL_DCHECK_GT(meshes_.size(), 0);
  // Starting from the last partition and working backwards, eliminate
  // triangles until the total triangle count reaches `new_triangle_count`.
  size_t first_truncated_partition_index = partitions_.size();
  for (int p = static_cast<int>(partitions_.size()) - 1; p >= 0; --p) {
    first_truncated_partition_index = p;
    MutableMesh& mesh = meshes_[p];
    uint32_t previous_triangle_count = partitions_[p].previous_triangle_count;
    if (new_triangle_count > previous_triangle_count) {
      mesh.Resize(mesh.VertexCount(),
                  new_triangle_count - previous_triangle_count);
      break;
    }
    mesh.Resize(mesh.VertexCount(), 0);
  }
  // Fix `previous_triangle_count` for all affected partitions.
  uint32_t cumulative_triangle_count =
      partitions_[first_truncated_partition_index].previous_triangle_count;
  for (size_t p = first_truncated_partition_index; p < partitions_.size();
       ++p) {
    partitions_[p].previous_triangle_count = cumulative_triangle_count;
    cumulative_triangle_count += meshes_[p].TriangleCount();
  }
}

void MutableMultiMesh::ResizeAppendVertices(uint32_t num_new_vertices) {
  // Add the new vertices to the last partition; if there are too many to fit,
  // keep adding more partitions until all new vertices have been added.
  mesh_vertex_indices_.reserve(mesh_vertex_indices_.size() + num_new_vertices);
  while (num_new_vertices > 0) {
    if (meshes_.empty() || meshes_.back().VertexCount() >= partition_after_) {
      AddNewPartition();
    }
    uint16_t partition_index = partitions_.size() - 1;
    // Add new vertices to the last partition's mesh.
    MutableMesh& mesh = meshes_[partition_index];
    uint32_t mesh_space_remaining = partition_after_ - mesh.VertexCount();
    uint32_t num_new_mesh_vertices =
        std::min(num_new_vertices, mesh_space_remaining);
    mesh.Resize(mesh.VertexCount() + num_new_mesh_vertices,
                mesh.TriangleCount());
    // Add these new vertices to the forward and reverse vertex index mappings.
    Partition& partition = partitions_[partition_index];
    partition.vertex_indices.reserve(mesh.VertexCount());
    for (uint32_t i = 0; i < num_new_mesh_vertices; ++i) {
      uint16_t vertex_index = mesh_vertex_indices_.size();
      uint16_t mesh_vertex_index = partition.vertex_indices.size();
      partition.vertex_indices.push_back(vertex_index);
      mesh_vertex_indices_.push_back(
          {VertexIndexPair{partition_index, mesh_vertex_index}});
    }
    // Keep going until there are no more new vertices to add.
    num_new_vertices -= num_new_mesh_vertices;
  }
}

void MutableMultiMesh::ResizeTruncateVertices(uint32_t new_vertex_count) {
  ABSL_DCHECK_LT(new_vertex_count, VertexCount());
  ABSL_DCHECK_EQ(meshes_.size(), partitions_.size());
  // For each partition, remove vertices from the end of the mesh that
  // correspond to vertices removed from the multi-mesh. Because vertices can
  // copied out-of-order into other partitions, it's possible that some vertices
  // we'd like to remove will be in the middle of the mesh's vertex list, and
  // won't be removed here. However, we can't easily reorder the vertices in the
  // mesh, because that would break any triangles that still depend on those
  // vertices. So any vertices that we fail to remove will just have to be
  // orphaned; they will be culled later when the `MutableMesh`es are packed
  // into `Mesh`es.
  for (uint16_t partition_index = 0; partition_index < partitions_.size();
       ++partition_index) {
    Partition& partition = partitions_[partition_index];
    while (!partition.vertex_indices.empty() &&
           partition.vertex_indices.back() >= new_vertex_count) {
      partition.vertex_indices.pop_back();
    }
    MutableMesh& mesh = meshes_[partition_index];
    mesh.Resize(partition.vertex_indices.size(), mesh.TriangleCount());
  }
  mesh_vertex_indices_.resize(new_vertex_count);
}

void MutableMultiMesh::AddNewPartition() {
  ABSL_DCHECK_EQ(meshes_.size(), partitions_.size());
  ABSL_DCHECK_LT(meshes_.size(), std::numeric_limits<uint16_t>::max());
  uint32_t triangle_count = TriangleCount();
  meshes_.push_back(MutableMesh(format_));
  partitions_.push_back(Partition{
      .previous_triangle_count = triangle_count,
  });
}

MutableMultiMesh::TriangleIndexPair MutableMultiMesh::GetPartitionTriangle(
    uint32_t triangle_index) const {
  // TODO: b/295166196 - Consider using a binary search here.
  for (int partition_index = partitions_.size() - 1; partition_index >= 0;
       --partition_index) {
    uint32_t previous_triangle_count =
        partitions_[partition_index].previous_triangle_count;
    if (triangle_index >= previous_triangle_count) {
      return TriangleIndexPair{
          static_cast<uint16_t>(partition_index),
          static_cast<uint16_t>(triangle_index - previous_triangle_count),
      };
    }
  }
  ABSL_CHECK(false) << "triangle_index out of bounds";
}

std::array<uint32_t, 3> MutableMultiMesh::CopyVerticesIntoPartition(
    const std::array<uint32_t, 3>& vertex_indices, uint16_t partition_index) {
  return {CopyVertexIntoPartition(vertex_indices[0], partition_index),
          CopyVertexIntoPartition(vertex_indices[1], partition_index),
          CopyVertexIntoPartition(vertex_indices[2], partition_index)};
}

uint16_t MutableMultiMesh::CopyVertexIntoPartition(uint32_t vertex_index,
                                                   uint16_t partition_index) {
  ABSL_DCHECK_LT(vertex_index, VertexCount());
  ABSL_DCHECK_LT(partition_index, partitions_.size());
  auto& mesh_vertex_indices = mesh_vertex_indices_[vertex_index];
  // If this vertex already exists in the requested partition, then we're done.
  for (auto [other_partition_index, other_mesh_vertex_index] :
       mesh_vertex_indices) {
    if (other_partition_index == partition_index) {
      return other_mesh_vertex_index;
    }
  }
  // Copy this vertex into the requested partition.
  auto [other_partition_index, other_mesh_vertex_index] =
      mesh_vertex_indices[0];
  const MutableMesh& other_mesh = meshes_[other_partition_index];
  MutableMesh& mesh = meshes_[partition_index];
  uint16_t mesh_vertex_index = mesh.VertexCount();
  mesh_vertex_indices.push_back(
      VertexIndexPair{partition_index, mesh_vertex_index});
  partitions_[partition_index].vertex_indices.push_back(vertex_index);
  mesh.AppendVertex(other_mesh.VertexPosition(other_mesh_vertex_index));
  // TODO: b/306149329 - Investigate memcpy-ing the vertex data instead of
  // repeatedly calling `SetFloatVertexAttribute()`.
  for (size_t i = 0; i < format_.Attributes().size(); ++i) {
    mesh.SetFloatVertexAttribute(
        mesh_vertex_index, i,
        other_mesh.FloatVertexAttribute(other_mesh_vertex_index, i));
  }
  return mesh_vertex_index;
}

}  // namespace ink::strokes_internal
