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

#include "ink/strokes/internal/stroke_subtraction.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::ComputeShapeOutline;
using ::ink::geometry_internal::ComputeSubtraction;
using ::ink::geometry_internal::ComputeTriangulation;
using ::ink::geometry_internal::Intersects;
using ::ink::geometry_internal::ShapeOutline;

using TriangleAttributes =
    absl::InlinedVector<std::array<SmallArray<float, 4>, 3>, 4>;
using EdgeVertexMap =
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                        absl::InlinedVector<std::pair<float, uint32_t>, 2>>;

// The fractional distance between two vertices on an edge to be considered
// identical.
// TODO(b/521448869): Use an adaptive tolerance based on the relevant scales
// and epsilons.
constexpr float kVertexDedupeTol = 1e-9;

bool IsLeftOrBelow(Point a, Point b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

// Returns a monotonic representation of the given triangle.
ShapeOutline GetTriangleShape(const Triangle& tri) {
  Point a = tri.p0, b = tri.p1, c = tri.p2;
  if (IsLeftOrBelow(a, b) && IsLeftOrBelow(a, c)) {
    if (IsLeftOrBelow(b, c)) {
      return ShapeOutline({{{a, b, c}, 1}, {{a, c}, -1}});
    }
    return ShapeOutline({{{a, b}, 1}, {{a, c, b}, -1}});
  }
  if (IsLeftOrBelow(b, a) && IsLeftOrBelow(b, c)) {
    if (IsLeftOrBelow(c, a)) {
      return ShapeOutline({{{b, c, a}, 1}, {{b, a}, -1}});
    }
    return ShapeOutline({{{b, c}, 1}, {{b, a, c}, -1}});
  }
  if (IsLeftOrBelow(a, b)) return ShapeOutline({{{c, a, b}, 1}, {{c, b}, -1}});
  return ShapeOutline({{{c, a}, 1}, {{c, b, a}, -1}});
}

// Computes the geometric boolean difference `tri` - `shape_b` as a
// triangulated polygon.
std::pair<std::vector<Point>, std::vector<std::array<uint32_t, 3>>>
SubtractTriangle(const Triangle& tri, const ShapeOutline& shape_b) {
  ShapeOutline shape_a = GetTriangleShape(tri);
  ShapeOutline remaining = ComputeSubtraction(shape_a, shape_b);
  return ComputeTriangulation(remaining);
}

// Returns the homogenous transform from world space to the barycentric coords
// for the given triangle.
// TODO(b/932647697): Consider defining this as a separate an externally visible
// utility function, or reusing similar existing functions in ink/geometry.
std::array<double, 9> ComputeBarycentricTransform(const Triangle& tri) {
  Vec v0 = tri.p1 - tri.p0;
  Vec v1 = tri.p2 - tri.p0;
  double det = double{v0.x} * v1.y - double{v0.y} * v1.x;

  if (det == 0.0) {
    return {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
  }

  return {v1.y,  -v1.x, double{v1.x} * tri.p0.y - double{v1.y} * tri.p0.x,
          -v0.y, v0.x,  double{v0.y} * tri.p0.x - double{v0.x} * tri.p0.y,
          0.0,   0.0,   det};
}

std::array<double, 3> ComputeBarycentricCoordinates(
    Point p, const std::array<double, 9>& transform) {
  double v_det = p.x * transform[0] + p.y * transform[1] + transform[2];
  double w_det = p.x * transform[3] + p.y * transform[4] + transform[5];
  double v = v_det / transform[8];
  double w = w_det / transform[8];
  double u = 1.0 - v - w;
  return {u, v, w};
}

// Copies the vertex (and its attributes) from `mesh` with index `vertex_index`
// and appends it to `mutable_mesh`.
void CopyVertex(const Mesh& mesh, uint32_t vertex_index,
                MutableMesh& mutable_mesh, const MeshFormat& format) {
  uint32_t new_index = mutable_mesh.VertexCount();
  mutable_mesh.AppendVertex(mesh.VertexPosition(vertex_index));
  for (uint32_t attribute_index = 0;
       attribute_index < format.Attributes().size(); ++attribute_index) {
    if (attribute_index == format.PositionAttributeIndex()) continue;
    mutable_mesh.SetFloatVertexAttribute(
        new_index, attribute_index,
        mesh.FloatVertexAttribute(vertex_index, attribute_index));
  }
}

// Adds a vertex at `position` to `mutable_mesh`, interpolating the given
// `triangle_attrs` using the given barycentric `weights`.
void AddVertex(MutableMesh& mutable_mesh, Point position,
               const TriangleAttributes& triangle_attrs,
               const std::array<double, 3>& weights) {
  uint32_t new_index = mutable_mesh.VertexCount();
  mutable_mesh.AppendVertex(position);
  for (uint32_t attr = 0; attr < triangle_attrs.size(); ++attr) {
    const auto& vals = triangle_attrs[attr];
    if (vals[0].Size() == 0) continue;
    uint8_t count = vals[0].Size();
    SmallArray<float, 4> interp_val(count);
    for (uint8_t c = 0; c < count; ++c) {
      // TODO(b/516793993): This interpolation is incorrect for Ink color
      // attributes, which are not in a linear space.
      interp_val[c] = weights[0] * vals[0][c] + weights[1] * vals[1][c] +
                      weights[2] * vals[2][c];
    }
    mutable_mesh.SetFloatVertexAttribute(new_index, attr, interp_val);
  }
}

// Finds an existing vertex or adds one, for a point along a triangle edge.
uint32_t GetOrAddEdgeVertex(MutableMesh& mutable_mesh, Point p, int i,
                            const std::array<uint32_t, 3>& indices,
                            const std::array<double, 3>& weights,
                            const TriangleAttributes& triangle_attrs,
                            EdgeVertexMap& edge_vertex_map) {
  int j = (i == 2) ? 0 : i + 1;
  int k = (i == 0) ? 2 : i - 1;
  uint32_t v1 = indices[j];
  uint32_t v2 = indices[k];

  float t = weights[k];
  if (v1 > v2) {
    std::swap(v1, v2);
    t = weights[j];
  }

  auto& edge_points = edge_vertex_map[{v1, v2}];
  for (const auto& [existing_t, existing_index] : edge_points) {
    if (std::abs(t - existing_t) < kVertexDedupeTol) return existing_index;
  }
  uint32_t new_index = mutable_mesh.VertexCount();
  AddVertex(mutable_mesh, p, triangle_attrs, weights);
  edge_points.push_back({t, new_index});
  return new_index;
}

// Finds an existing vertex or adds one at the given point in the mutable mesh.
uint32_t GetOrAddVertex(MutableMesh& mutable_mesh, Point p,
                        const std::array<uint32_t, 3>& indices,
                        const std::array<double, 3>& weights,
                        const TriangleAttributes& triangle_attrs,
                        EdgeVertexMap& edge_vertex_map) {
  std::array<bool, 3> near_zero = {weights[0] < kVertexDedupeTol,
                                   weights[1] < kVertexDedupeTol,
                                   weights[2] < kVertexDedupeTol};

  if (near_zero[1] && near_zero[2]) return indices[0];
  if (near_zero[2] && near_zero[0]) return indices[1];
  if (near_zero[0] && near_zero[1]) return indices[2];

  for (int i = 0; i < 3; ++i) {
    if (near_zero[i]) {
      return GetOrAddEdgeVertex(mutable_mesh, p, i, indices, weights,
                                triangle_attrs, edge_vertex_map);
    }
  }

  uint32_t new_index = mutable_mesh.VertexCount();
  AddVertex(mutable_mesh, p, triangle_attrs, weights);
  return new_index;
}

// Adds the `pts` from the partially erased triangle `tri` to `mutable_mesh` and
// returns their resulting indices. These vertices may existing vertices in the
// mutable_mesh, either from vertices in the original mesh or from intersection
// vertices on the boundaries of triangle edges.
std::vector<uint32_t> AddVertices(MutableMesh& mutable_mesh,
                                  const std::array<uint32_t, 3>& indices,
                                  const Triangle& tri,
                                  const std::vector<Point>& pts,
                                  EdgeVertexMap& edge_vertex_map) {
  std::array<double, 9> transform = ComputeBarycentricTransform(tri);

  const MeshFormat& format = mutable_mesh.Format();
  TriangleAttributes triangle_attrs(format.Attributes().size());
  for (uint32_t attr = 0; attr < format.Attributes().size(); ++attr) {
    MeshFormat::AttributeId id = format.Attributes()[attr].id;
    if (id == MeshFormat::AttributeId::kPosition ||
        id == MeshFormat::AttributeId::kSideLabel ||
        id == MeshFormat::AttributeId::kSideDerivative ||
        id == MeshFormat::AttributeId::kForwardLabel ||
        id == MeshFormat::AttributeId::kForwardDerivative) {
      continue;
    }
    triangle_attrs[attr] = {
        mutable_mesh.FloatVertexAttribute(indices[0], attr),
        mutable_mesh.FloatVertexAttribute(indices[1], attr),
        mutable_mesh.FloatVertexAttribute(indices[2], attr)};
  }

  std::vector<uint32_t> pt_indices;
  pt_indices.reserve(pts.size());

  for (Point p : pts) {
    std::array<double, 3> weights = ComputeBarycentricCoordinates(p, transform);
    pt_indices.push_back(GetOrAddVertex(mutable_mesh, p, indices, weights,
                                        triangle_attrs, edge_vertex_map));
  }

  return pt_indices;
}

// Adds a triangulation (defined by `pts` and `tris`) resulting from the
// subtraction of a triangle `tri` from `mutable_mesh`.
void AddSubtractedTriangle(MutableMesh& mutable_mesh,
                           const std::array<uint32_t, 3>& indices,
                           const Triangle& tri, const std::vector<Point>& pts,
                           const std::vector<std::array<uint32_t, 3>>& tris,
                           EdgeVertexMap& edge_vertex_map) {
  if (pts.empty() || tris.empty()) return;

  std::vector<uint32_t> mapped_indices =
      AddVertices(mutable_mesh, indices, tri, pts, edge_vertex_map);
  if (mapped_indices.empty()) return;

  for (const auto& t : tris) {
    mutable_mesh.AppendTriangleIndices(
        {mapped_indices[t[0]], mapped_indices[t[1]], mapped_indices[t[2]]});
  }
}

// Returns a mutable mesh with the given format representing the subtraction of
// `shape_b` from `meshes`.
MutableMesh SubtractMeshes(absl::Span<const Mesh> meshes,
                           const MeshFormat& format,
                           const ShapeOutline& shape_b) {
  // To compute the subtraction `meshes` - `shape_b`, we process each
  // triangle in `meshes` individually. For each triangle, we first handle the
  // geometry by computing a triangulation of the shape of `triangle` -
  // `shape_b`. Next, we add all the vertices from the triangulation to the
  // `mutable_mesh` result (making sure to re-use existing vertices to properly
  // glue the triangulations together along shared vertices and edges) and
  // setting attributes by interpolating from the original triangle vertices.
  // Finally, we add all the triangles from the triangulation, using the mapped
  // indices of the corresponding vertices in the resulting `mutable_mesh`.

  MutableMesh mutable_mesh(format);
  for (const Mesh& mesh : meshes) {
    uint32_t vertex_offset = mutable_mesh.VertexCount();

    // Copy over all of the vertices. Vertices not belonging to any triangle
    // will be removed by PartitionedMesh::FromMutableMeshGroups.
    for (uint32_t vert_idx = 0; vert_idx < mesh.VertexCount(); ++vert_idx) {
      CopyVertex(mesh, vert_idx, mutable_mesh, format);
    }

    // A map to help weld triangles back together (i.e. de-duplicate vertices).
    // It maps each edge in `mesh` (specified by the ordered pair of
    // corresponding vertex indices in `mutable_mesh`) to the list of new
    // vertices created along that edge (represented by a pair, {parameter along
    // the edge, vertex index in `mutable_mesh`}).
    EdgeVertexMap edge_vertex_map;

    for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
      std::array<uint32_t, 3> indices = mesh.TriangleIndices(tri_idx);
      for (int i = 0; i < 3; ++i) indices[i] += vertex_offset;

      Triangle tri = mesh.GetTriangle(tri_idx);

      if (Intersects(shape_b, Envelope(tri).AsRect().value())) {
        auto [subtracted_pts, subtracted_tris] = SubtractTriangle(tri, shape_b);
        AddSubtractedTriangle(mutable_mesh, indices, tri, subtracted_pts,
                              subtracted_tris, edge_vertex_map);
      } else {
        mutable_mesh.AppendTriangleIndices(indices);
      }
    }
  }

  return mutable_mesh;
}

ShapeOutline GetShapeB(const PartitionedMesh& mesh,
                       const AffineTransform& transform) {
  std::vector<std::vector<Point>> loops;
  for (uint32_t group = 0; group < mesh.RenderGroupCount(); ++group) {
    for (uint32_t outline = 0; outline < mesh.OutlineCount(group); ++outline) {
      uint32_t n = mesh.OutlineVertexCount(group, outline);
      std::vector<Point> loop(n);
      for (uint32_t i = 0; i < n; ++i) {
        // Ink stroke outlines are CW oriented, so reverse the points.
        loop[i] =
            transform.Apply(mesh.OutlinePosition(group, outline, n - 1 - i));
      }
      loops.push_back(std::move(loop));
    }
  }
  return ComputeShapeOutline(loops);
}
}  // namespace

absl::StatusOr<PartitionedMesh> Subtract(const PartitionedMesh& mesh_a,
                                         const AffineTransform& transform_a,
                                         const PartitionedMesh& mesh_b,
                                         const AffineTransform& transform_b) {
  // The approach in this function is to first compute a silhouette of `mesh_b`.
  // Then, for each coat of `mesh_a`, we compute a new mutable mesh representing
  // for the coat minus the silhouette of b. Finally, we assemble the resulting
  // coats into a PartitionedMesh.

  std::optional<AffineTransform> inv_transform_a = transform_a.Inverse();
  if (!inv_transform_a.has_value())
    return absl::InvalidArgumentError("transform_a must be invertible.");

  if (!transform_b.Inverse().has_value())
    return absl::InvalidArgumentError("transform_b must be invertible.");

  // TODO(b/521448869): For now we use `mesh_a`'s coordinate system. If in the
  // future, the outline `shape_b` is cached in `mesh_b`, we should consider
  // working in a different coordinate system.
  AffineTransform b_to_a = *inv_transform_a * transform_b;
  ShapeOutline shape_b = GetShapeB(mesh_b, b_to_a);

  std::vector<PartitionedMesh::MutableMeshGroup> groups;
  groups.reserve(mesh_a.RenderGroupCount());

  std::vector<MutableMesh> group_mutable_meshes;
  group_mutable_meshes.reserve(mesh_a.RenderGroupCount());
  std::vector<StrokeVertex::CustomPackingArray> packing_arrays;
  packing_arrays.reserve(mesh_a.RenderGroupCount());

  for (uint32_t group = 0; group < mesh_a.RenderGroupCount(); ++group) {
    // Each coat is handled independently.
    const MeshFormat& format = mesh_a.RenderGroupFormat(group);
    absl::Span<const Mesh> meshes = mesh_a.RenderGroupMeshes(group);
    group_mutable_meshes.push_back(SubtractMeshes(meshes, format, shape_b));
    MutableMesh& mutable_mesh = group_mutable_meshes.back();

    packing_arrays.push_back(StrokeVertex::MakeCustomPackingArray(format));

    // TODO(b/509625875): Compute the mesh outline.
    groups.push_back(PartitionedMesh::MutableMeshGroup{
        .mesh = &mutable_mesh,
        .outlines = {},
        .packing_params = packing_arrays.back().Values(),
    });
  }

  return PartitionedMesh::FromMutableMeshGroups(groups);
}

}  // namespace ink::strokes_internal
