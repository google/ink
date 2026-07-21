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
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_index_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/numbers.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::ComputeShapeOutline;
using ::ink::geometry_internal::ComputeSubtraction;
using ::ink::geometry_internal::ComputeTriangulation;
using ::ink::geometry_internal::Intersects;
using ::ink::geometry_internal::ShapeOutline;
using ::ink::numbers::kPi;

using TriangleAttributes =
    absl::InlinedVector<std::array<SmallArray<float, 4>, 3>, 4>;
using EdgeVertexMap =
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                        absl::InlinedVector<std::pair<Point, uint32_t>, 2>>;

float DistanceSquared(Point a, Point b) { return (a - b).MagnitudeSquared(); }

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

// A helper function to compute the altitudes of a triangle, given the triangle
// and its determinant (twice the area).
std::array<double, 3> ComputeHeights(const Triangle& tri, double det) {
  Vec v0 = tri.p2 - tri.p1;
  Vec v1 = tri.p0 - tri.p2;
  Vec v2 = tri.p1 - tri.p0;
  return {det / v0.Magnitude(), det / v1.Magnitude(), det / v2.Magnitude()};
}

// Maps HSL shifts to coordinates where they interpolate linearly.
//
// The interpolation scheme for HSL shift values is defined via their mapping to
// linear RGB values, which are interpolated linearly during rasterization.
//
// The RGB values are obtained by applying the shift to a uniform (vertex
// independent) base RGB color. Although the shift acts as an affine
// transformation in RGB space, the transformation's matrix elements are
// nonlinear in the HSL shift values. This function maps the HSL shift
// into coordinates in terms of which the affine transformation is linear.
//
// LINT.IfChange(hsl_shift_linear_space)
SmallArray<float, 4> HslShiftToLinearSpace(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float hue_shift = 2 * kPi * val[0], saturation_shift = val[1];
  val[0] = (saturation_shift + 1) * std::cos(hue_shift);
  val[1] = (saturation_shift + 1) * std::sin(hue_shift);
  return val;
}

SmallArray<float, 4> LinearSpaceToHslShift(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float dx = val[0], dy = val[1];
  val[0] = std::atan2(dy, dx) / (2.0f * kPi);
  val[1] = std::hypot(dx, dy) - 1.0f;
  return val;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hsl_and_opacity_shift)

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
  const MeshFormat& format = mutable_mesh.Format();
  ABSL_DCHECK_EQ(triangle_attrs.size(), format.Attributes().size());
  for (uint32_t attr = 0; attr < triangle_attrs.size(); ++attr) {
    MeshFormat::AttributeId id = format.Attributes()[attr].id;
    // Set derivatives to 1.0 to avoid divisions-by-zero in the shader.
    if (id == MeshFormat::AttributeId::kSideDerivative ||
        id == MeshFormat::AttributeId::kForwardDerivative) {
      mutable_mesh.SetFloatVertexAttribute(new_index, attr, {1.0f, 1.0f});
      continue;
    }

    const auto& vals = triangle_attrs[attr];
    if (vals[0].Size() == 0) continue;
    uint8_t count = vals[0].Size();
    SmallArray<float, 4> interp_val(count);
    for (uint8_t c = 0; c < count; ++c) {
      interp_val[c] = weights[0] * vals[0][c] + weights[1] * vals[1][c] +
                      weights[2] * vals[2][c];
    }

    // Don't forget to map the HSL shift back to proper coordinates.
    if (id == MeshFormat::AttributeId::kColorShiftHsl) {
      interp_val = LinearSpaceToHslShift(interp_val);
    }

    mutable_mesh.SetFloatVertexAttribute(new_index, attr, interp_val);
  }
}

// Finds an existing vertex or adds one, for a point along a triangle edge.
uint32_t GetOrAddEdgeVertex(MutableMesh& mutable_mesh, Point p, int i,
                            const std::array<uint32_t, 3>& indices,
                            const std::array<double, 3>& weights,
                            const TriangleAttributes& triangle_attrs,
                            float epsilon, EdgeVertexMap& edge_vertex_map) {
  int j = (i == 2) ? 0 : i + 1;
  int k = (i == 0) ? 2 : i - 1;
  uint32_t v1 = indices[j];
  uint32_t v2 = indices[k];

  if (v1 > v2) std::swap(v1, v2);

  auto& edge_points = edge_vertex_map[{v1, v2}];
  for (const auto& [existing_p, existing_index] : edge_points) {
    if (DistanceSquared(p, existing_p) <= epsilon * epsilon)
      return existing_index;
  }
  uint32_t new_index = mutable_mesh.VertexCount();
  AddVertex(mutable_mesh, p, triangle_attrs, weights);
  edge_points.push_back({p, new_index});
  return new_index;
}

// Finds an existing vertex or adds one at the given point in the mutable mesh.
uint32_t GetOrAddVertex(MutableMesh& mutable_mesh, Point p,
                        const std::array<uint32_t, 3>& indices,
                        const std::array<double, 9>& transform,
                        const std::array<double, 3>& heights,
                        const TriangleAttributes& triangle_attrs, float epsilon,
                        EdgeVertexMap& edge_vertex_map) {
  std::array<double, 3> weights = ComputeBarycentricCoordinates(p, transform);
  std::array<bool, 3> near_zero = {weights[0] * heights[0] < epsilon,
                                   weights[1] * heights[1] < epsilon,
                                   weights[2] * heights[2] < epsilon};

  if (near_zero[1] && near_zero[2]) return indices[0];
  if (near_zero[2] && near_zero[0]) return indices[1];
  if (near_zero[0] && near_zero[1]) return indices[2];

  for (int i = 0; i < 3; ++i) {
    if (near_zero[i]) {
      return GetOrAddEdgeVertex(mutable_mesh, p, i, indices, weights,
                                triangle_attrs, epsilon, edge_vertex_map);
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
                                  const std::vector<Point>& pts, float epsilon,
                                  EdgeVertexMap& edge_vertex_map) {
  std::array<double, 9> transform = ComputeBarycentricTransform(tri);
  std::array<double, 3> heights = ComputeHeights(tri, transform[8]);

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

    if (id == MeshFormat::AttributeId::kColorShiftHsl) {
      for (int i = 0; i < 3; ++i) {
        triangle_attrs[attr][i] = HslShiftToLinearSpace(
            mutable_mesh.FloatVertexAttribute(indices[i], attr));
      }
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
    pt_indices.push_back(GetOrAddVertex(mutable_mesh, p, indices, transform,
                                        heights, triangle_attrs, epsilon,
                                        edge_vertex_map));
  }

  return pt_indices;
}

// A helper function to add the oriented boundary of `triangle` (consisting of
// its three edges) to the `boundary` adjacency list. See also the comment in
// `SubtractMeshes`.
void UpdateBoundary(const std::array<uint32_t, 3>& triangle,
                    std::vector<absl::InlinedVector<uint32_t, 4>>& boundary) {
  uint32_t u = triangle[2];
  for (uint32_t v : triangle) {
    // If [v,u] = -[u,v] already exists in the boundary, remove it. Otherwise
    // add [u,v].
    auto it = std::find(boundary[v].begin(), boundary[v].end(), u);
    if (it != boundary[v].end()) {
      boundary[v].erase(it);
    } else {
      boundary[u].push_back(v);
    }
    u = v;
  }
}

// Adds the `subtracted_triangulation` (the fragments of `tri` that remain after
// subtraction) to the `mutable_mesh`. Uses `edge_vertex_map` to de-duplicate
// any vertices newly created along `tri`'s edges. Finally, updates `boundary`
// with the edges of the new fragments.
void AddSubtractedTriangle(
    const std::array<uint32_t, 3>& indices, const Triangle& tri,
    const std::pair<std::vector<Point>, std::vector<std::array<uint32_t, 3>>>&
        subtracted_triangulation,
    float epsilon, EdgeVertexMap& edge_vertex_map, MutableMesh& mutable_mesh,
    std::vector<absl::InlinedVector<uint32_t, 4>>& boundary) {
  const auto& [pts, tris] = subtracted_triangulation;

  if (pts.empty() || tris.empty()) return;

  std::vector<uint32_t> mapped_indices =
      AddVertices(mutable_mesh, indices, tri, pts, epsilon, edge_vertex_map);
  // Resize the boundary adjacency map to accommodate newly added vertices.
  boundary.resize(mutable_mesh.VertexCount());

  for (const auto& t : tris) {
    std::array<uint32_t, 3> idxs = {mapped_indices[t[0]], mapped_indices[t[1]],
                                    mapped_indices[t[2]]};
    if (idxs[0] == idxs[1] || idxs[1] == idxs[2] || idxs[2] == idxs[0])
      continue;
    mutable_mesh.AppendTriangleIndices(idxs);
    UpdateBoundary(idxs, boundary);
  }
}

// Traces the connected components of the given `boundary` adjacency list to
// extract the closed loops as discrete clockwise oriented outlines.
// Note that this consumes the contents of the adjacency list in the process.
std::vector<std::vector<uint32_t>> ExtractOutlines(
    std::vector<absl::InlinedVector<uint32_t, 4>> boundary) {
  std::vector<std::vector<uint32_t>> new_outlines;
  for (uint32_t start = 0; start < boundary.size(); ++start) {
    while (!boundary[start].empty()) {
      std::vector<uint32_t> loop;
      uint32_t curr = start;
      do {
        loop.push_back(curr);
        if (boundary[curr].empty()) break;
        uint32_t next = boundary[curr].back();
        boundary[curr].pop_back();
        curr = next;
      } while (curr != start);

      if (loop.size() > 2) {
        std::reverse(loop.begin(), loop.end());
        new_outlines.push_back(std::move(loop));
      }
    }
  }
  return new_outlines;
}

// Returns a mutable mesh with the given format representing the subtraction of
// `shape_b` from `meshes` along with its updated outlines.
std::pair<MutableMesh, std::vector<std::vector<uint32_t>>> SubtractMeshes(
    absl::Span<const Mesh> meshes, const MeshFormat& format,
    const ShapeOutline& shape_b, float epsilon) {
  // To compute the subtraction `meshes` - `shape_b`, we process each
  // triangle in `meshes` individually. For each triangle, we first handle the
  // geometry by computing a triangulation of the shape of `triangle` -
  // `shape_b`. Next, we add all the vertices from the triangulation to the
  // `mutable_mesh` result (making sure to re-use existing vertices to properly
  // glue the triangulations together along shared vertices and edges) and
  // set their attributes by interpolating from the original triangle vertices.
  // Finally, we add all the triangles from the triangulation, using the mapped
  // indices of the corresponding vertices in the resulting `mutable_mesh`.

  MutableMesh mutable_mesh(format);
  // Copy over all of the vertices. Vertices not belonging to any triangle
  // will be removed by PartitionedMesh::FromMutableMeshGroups.
  for (const Mesh& mesh : meshes) {
    for (uint32_t vert_idx = 0; vert_idx < mesh.VertexCount(); ++vert_idx) {
      CopyVertex(mesh, vert_idx, mutable_mesh, format);
    }
  }

  // As we process the subtraction, we also compute the outline of the result,
  // by fist incrementally computing its simplicial boundary (the set of
  // directed boundary edges of triangles that aren't "cancelled" out by an
  // adjacent triangle), and then finally tracing the boundary edges to extract
  // the outline.
  //
  // TODO(b/523326691): Consider initializing `boundary` with the existing mesh
  // outlines to avoid recomputing the outline for untouched parts of the mesh.
  //
  // We store the simplicial boundary in a vertex adjacency list `boundary`,
  // representing directed edges of the mesh. As we add triangle boundaries, we
  // make sure to remove cancelled edges (i.e. if (v, u) is already present in
  // the adjacency list, adding (u, v) removes it). The capacity of 4 is chosen
  // as 4 is the typical degree of a vertex in a triangle strip.
  std::vector<absl::InlinedVector<uint32_t, 4>> boundary(
      mutable_mesh.VertexCount());

  // Process the triangles.
  uint32_t vertex_offset = 0;
  for (const Mesh& mesh : meshes) {
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
        AddSubtractedTriangle(indices, tri, SubtractTriangle(tri, shape_b),
                              epsilon, edge_vertex_map, mutable_mesh, boundary);
      } else {
        UpdateBoundary(indices, boundary);
        mutable_mesh.AppendTriangleIndices(indices);
      }
    }

    vertex_offset += mesh.VertexCount();
  }

  return {std::move(mutable_mesh), ExtractOutlines(std::move(boundary))};
}

// Returns a `ShapeOutline` representing the silhouette of `mesh` when
// transformed by `transform`, with consecutive points closer than `epsilon`
// distance filtered out.
ShapeOutline GetShapeB(const PartitionedMesh& mesh,
                       const AffineTransform& transform, float epsilon) {
  std::vector<std::vector<Point>> loops;
  float epsilon_squared = epsilon * epsilon;
  for (uint32_t group = 0; group < mesh.RenderGroupCount(); ++group) {
    for (uint32_t outline = 0; outline < mesh.OutlineCount(group); ++outline) {
      uint32_t num_vertices = mesh.OutlineVertexCount(group, outline);
      if (num_vertices == 0) continue;
      std::vector<Point> loop;
      loop.reserve(num_vertices);

      // Iterate backwards, to reverse the loop, because Ink outlines are
      // clockwise oriented.
      loop.push_back(transform.Apply(
          mesh.OutlinePosition(group, outline, num_vertices - 1)));
      for (int i = static_cast<int>(num_vertices) - 2; i >= 0; --i) {
        Point p = transform.Apply(mesh.OutlinePosition(group, outline, i));
        if (DistanceSquared(p, loop.back()) >= epsilon_squared) {
          loop.push_back(p);
        }
      }

      // Remove points at the end of the loop that are within `epsilon` of the
      // start. A while loop is necessary to handle cases where multiple points
      // at the end are spaced `epsilon` apart from each other sequentially, but
      // yet still within `epsilon` of the start.
      while (loop.size() > 2 &&
             DistanceSquared(loop.back(), loop.front()) < epsilon_squared) {
        loop.pop_back();
      }

      if (loop.size() < 3) continue;

      loops.push_back(std::move(loop));
    }
  }
  return ComputeShapeOutline(loops);
}
}  // namespace

absl::StatusOr<PartitionedMesh> Subtract(const PartitionedMesh& mesh_a,
                                         const AffineTransform& transform_a,
                                         const PartitionedMesh& mesh_b,
                                         const AffineTransform& transform_b,
                                         float epsilon) {
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
  ShapeOutline shape_b = GetShapeB(mesh_b, b_to_a, epsilon);

  uint32_t num_groups = mesh_a.RenderGroupCount();

  std::vector<PartitionedMesh::MutableMeshGroup> groups(num_groups);
  std::vector<MutableMesh> group_mutable_meshes(num_groups);
  std::vector<std::vector<std::vector<uint32_t>>> groups_outlines(num_groups);
  std::vector<std::vector<absl::Span<const uint32_t>>> groups_outline_spans(
      num_groups);
  std::vector<StrokeVertex::CustomPackingArray> packing_arrays(num_groups);

  for (uint32_t group = 0; group < num_groups; ++group) {
    // Each coat is handled independently.
    const MeshFormat& format = mesh_a.RenderGroupFormat(group);
    auto [mesh, outlines] = SubtractMeshes(mesh_a.RenderGroupMeshes(group),
                                           format, shape_b, epsilon);
    group_mutable_meshes[group] = std::move(mesh);
    groups_outlines[group] = std::move(outlines);
    for (const auto& outline : groups_outlines[group]) {
      groups_outline_spans[group].push_back(outline);
    }

    packing_arrays[group] = StrokeVertex::MakeCustomPackingArray(format);

    groups[group] = PartitionedMesh::MutableMeshGroup{
        .mesh = &group_mutable_meshes[group],
        .outlines = groups_outline_spans[group],
        .packing_params = packing_arrays[group].Values(),
    };
  }

  return PartitionedMesh::FromMutableMeshGroups(groups);
}

}  // namespace ink::strokes_internal
