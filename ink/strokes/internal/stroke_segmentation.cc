// Copyright 2026 Google LLC
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

#include "ink/strokes/internal/stroke_segmentation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::strokes_internal {
namespace {

// A disjoint-set data structure over vertex indices, to help union-find
// connected components. See
// https://en.wikipedia.org/wiki/Disjoint-set_data_structure.
class ConnectedComponents {
  // The connected components are represented as a forest of trees, with each
  // tree corresponding to a connected component. The forest is stored as a list
  // (`parent_`) mapping each vertex to its parent vertex in the forest, with
  // root vertices mapping to themselves.

 public:
  explicit ConnectedComponents(uint32_t num_vertices) : parent_(num_vertices) {
    // Initialize each vertex to its own component.
    for (uint32_t i = 0; i < num_vertices; ++i) parent_[i] = i;
  }

  // Union the two sets containing `i` and `j`.
  void Union(uint32_t i, uint32_t j) {
    uint32_t root_i = Root(i);
    uint32_t root_j = Root(j);

    // When merging the two components, always choose the smaller index to be
    // the root. This ensures the convenient property that the component index
    // of every vertex is always less than or equal to the vertex index.
    if (root_i < root_j) {
      parent_[root_j] = root_i;
    } else if (root_j < root_i) {
      parent_[root_i] = root_j;
    }
  }

  // Returns a list mapping each vertex to its connected component index.
  // Component indices are in [0,...,num_components-1], but ordered arbitrarily.
  absl::Span<const uint32_t> Components() {
    for (uint32_t i = 0; i < parent_.size(); ++i) Root(i);
    uint32_t next_id = 0;

    // Here we exploit the fact that component_[i] <= i for all i, which means
    // that we always encounter the root of a component (the i for which
    // i == parent_[i]) before we encounter any other vertex in the
    // component.
    for (uint32_t i = 0; i < parent_.size(); ++i) {
      uint32_t root = parent_[i];
      if (root == i) {
        parent_[i] = next_id++;
      } else {
        parent_[i] = parent_[root];
      }
    }
    return parent_;
  }

 private:
  // Returns the root of the tree `i` belongs to, and compresses the path from
  // `i` to its root.
  uint32_t Root(uint32_t i) {
    uint32_t root = i;

    // Traverse up the tree to find the root.
    while (parent_[root] != root) root = parent_[root];

    // Traverse up the tree again to flatten the path.
    uint32_t curr = i;
    while (curr != root) {
      uint32_t next = parent_[curr];
      parent_[curr] = root;
      curr = next;
    }
    return root;
  }

  std::vector<uint32_t> parent_;
};

std::vector<Point> GetAllVertices(const PartitionedMesh& shape,
                                  const AffineTransform& transform) {
  size_t total_vertices = 0;
  for (uint32_t g = 0; g < shape.RenderGroupCount(); ++g) {
    for (const Mesh& mesh : shape.RenderGroupMeshes(g)) {
      total_vertices += mesh.VertexCount();
    }
  }
  std::vector<Point> ret;
  ret.reserve(total_vertices);
  for (uint32_t g = 0; g < shape.RenderGroupCount(); ++g) {
    for (const Mesh& mesh : shape.RenderGroupMeshes(g)) {
      for (uint32_t v = 0; v < mesh.VertexCount(); ++v) {
        ret.push_back(transform.Apply(mesh.VertexPosition(v)));
      }
    }
  }
  return ret;
}

// Updates `components` by unioning vertices that share a triangle in `shape`.
void ConnectByTriangles(const PartitionedMesh& shape,
                        ConnectedComponents& components) {
  uint32_t offset = 0;
  for (uint32_t g = 0; g < shape.RenderGroupCount(); ++g) {
    for (const Mesh& mesh : shape.RenderGroupMeshes(g)) {
      for (uint32_t t = 0; t < mesh.TriangleCount(); ++t) {
        std::array<uint32_t, 3> indices = mesh.TriangleIndices(t);
        components.Union(offset + indices[0], offset + indices[1]);
        components.Union(offset + indices[1], offset + indices[2]);
      }
      offset += mesh.VertexCount();
    }
  }
}

// Updates `components` by unioning vertices that are within `tolerance` of each
// other.
void ConnectBySpatialProximity(const std::vector<Point>& vertices,
                               float tolerance,
                               ConnectedComponents& components) {
  // Linesweep approach: sort vertices by x-coordinate to help prune the
  // vertices within `tolerance`.
  std::vector<uint32_t> indices(vertices.size());
  for (uint32_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }

  absl::c_sort(indices, [&](uint32_t a, uint32_t b) {
    return vertices[a].x < vertices[b].x;
  });

  float tolerance_squared = tolerance * tolerance;
  for (uint32_t i = 0; i < vertices.size(); ++i) {
    uint32_t u = indices[i];
    Point p_u = vertices[u];
    for (uint32_t j = i + 1; j < vertices.size(); ++j) {
      uint32_t v = indices[j];
      Point p_v = vertices[v];
      if (p_v.x - p_u.x > tolerance) break;
      if ((p_u - p_v).MagnitudeSquared() <= tolerance_squared) {
        components.Union(u, v);
      }
    }
  }
}

// Returns PartitionedMeshes for each connected component in `shape` as
// specified by `vertex_components`.
absl::StatusOr<std::vector<PartitionedMesh>> ConstructMeshes(
    const PartitionedMesh& shape,
    absl::Span<const uint32_t> vertex_components) {
  if (vertex_components.empty())
    return absl::InvalidArgumentError("vertex_components cannot be empty.");

  uint32_t num_components =
      *std::max_element(vertex_components.begin(), vertex_components.end()) + 1;
  if (num_components == 1) return std::vector<PartitionedMesh>{shape};

  // Initialize new meshes for each connected component.
  std::vector<std::vector<MutableMesh>> meshes(num_components);
  for (auto& comp_meshes : meshes) {
    comp_meshes.reserve(shape.RenderGroupCount());
    for (uint32_t i = 0; i < shape.RenderGroupCount(); ++i) {
      comp_meshes.emplace_back(shape.RenderGroupFormat(i));
    }
  }

  std::vector<uint32_t> indices(vertex_components.size());
  uint32_t offset = 0;

  for (uint32_t g = 0; g < shape.RenderGroupCount(); ++g) {
    const MeshFormat& format = shape.RenderGroupFormat(g);
    for (const Mesh& mesh : shape.RenderGroupMeshes(g)) {
      // Copy each vertex of `mesh` into the right component in `meshes`.
      for (uint32_t v = 0; v < mesh.VertexCount(); ++v) {
        uint32_t comp = vertex_components[offset + v];
        MutableMesh& mutable_mesh = meshes[comp][g];
        indices[offset + v] = mutable_mesh.VertexCount();
        mutable_mesh.AppendVertex(mesh.VertexPosition(v));
        for (uint32_t a = 0; a < format.Attributes().size(); ++a) {
          if (a == format.PositionAttributeIndex()) continue;
          mutable_mesh.SetFloatVertexAttribute(indices[offset + v], a,
                                               mesh.FloatVertexAttribute(v, a));
        }
      }

      // Copy each triangle into the right component in `meshes`.
      for (uint32_t t = 0; t < mesh.TriangleCount(); ++t) {
        std::array<uint32_t, 3> tri = mesh.TriangleIndices(t);
        uint32_t comp = vertex_components[offset + tri[0]];
        MutableMesh& mutable_mesh = meshes[comp][g];
        mutable_mesh.AppendTriangleIndices({indices[offset + tri[0]],
                                            indices[offset + tri[1]],
                                            indices[offset + tri[2]]});
      }
      offset += mesh.VertexCount();
    }
  }

  // Construct `PartitionedMesh`s from `meshes`.
  std::vector<PartitionedMesh> result_meshes;
  result_meshes.reserve(meshes.size());
  for (const auto& group_mutable_meshes : meshes) {
    std::vector<PartitionedMesh::MutableMeshGroup> groups;
    groups.reserve(group_mutable_meshes.size());
    std::vector<StrokeVertex::CustomPackingArray> packing_arrays;
    packing_arrays.reserve(group_mutable_meshes.size());

    for (const MutableMesh& mutable_mesh : group_mutable_meshes) {
      packing_arrays.push_back(
          StrokeVertex::MakeCustomPackingArray(mutable_mesh.Format()));
    }
    for (size_t i = 0; i < group_mutable_meshes.size(); ++i) {
      groups.push_back({
          .mesh = &group_mutable_meshes[i],
          .outlines = {},
          .packing_params = packing_arrays[i].Values(),
      });
    }

    absl::StatusOr<PartitionedMesh> partitioned_mesh =
        PartitionedMesh::FromMutableMeshGroups(groups);
    if (!partitioned_mesh.ok()) return partitioned_mesh.status();
    result_meshes.push_back(*std::move(partitioned_mesh));
  }

  return result_meshes;
}

}  // namespace

absl::StatusOr<std::vector<PartitionedMesh>> SegmentSpatially(
    const PartitionedMesh& shape, const AffineTransform& transform,
    float tolerance) {
  // The approach is to first compute the connected component for each vertex
  // in `shape`, and then to copy each vertex and triangle from `shape` into
  // the PartitionedMesh corresponding to its component.

  // Union-find to compute connected components: initialize each vertex in its
  // own component, then merge vertices that share a triangle, then merge
  // vertices that are within `tolerance` of each other.
  std::vector<Point> vertices = GetAllVertices(shape, transform);
  ConnectedComponents components(vertices.size());
  ConnectByTriangles(shape, components);
  ConnectBySpatialProximity(vertices, tolerance, components);

  // Construct new partitioned meshes for each component.
  return ConstructMeshes(shape, components.Components());
}
}  // namespace ink::strokes_internal
