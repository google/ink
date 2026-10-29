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
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/internal/outline_triangulation.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/mesh_attribute_interpolation.h"
#include "ink/strokes/internal/stroke_anti_aliasing.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::ComputeSubtraction;
using ::ink::geometry_internal::ComputeTriangulation;
using ::ink::geometry_internal::Intersects;
using ::ink::geometry_internal::ShapeOutline;
using ::ink::geometry_internal::Triangulation;

// A set of directed boundary edges, where each edge is represented by an
// ordered pair of vertex indices (u, v) oriented counterclockwise around the
// triangle it belongs to.
using BoundaryEdgeSet = absl::flat_hash_set<std::pair<uint32_t, uint32_t>>;

float DistanceSquared(Point a, Point b) { return (a - b).MagnitudeSquared(); }

// Remaps the three vertex indices of a triangle using `index_map`.
std::array<uint32_t, 3> MapIndices(const std::array<uint32_t, 3>& tri,
                                   absl::Span<const uint32_t> index_map) {
  return {index_map[tri[0]], index_map[tri[1]], index_map[tri[2]]};
}

// Returns the homogenous transform from world space to the barycentric coords
// for the given triangle.
// TODO(b/932647697): Consider defining this as a separate externally visible
// utility function, or reusing similar existing functions in ink/geometry.
std::array<double, 9> ComputeBarycentricTransform(const Triangle& tri) {
  Vec v0 = tri.p1 - tri.p0;
  Vec v1 = tri.p2 - tri.p0;
  double det = double{v0.x} * v1.y - double{v0.y} * v1.x;

  if (det == 0.0) return {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};

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

// Returns a map from each vertex to the canonical vertex it should be welded
// to, found by pairing coincident boundary edges with opposite orientations.
std::vector<uint32_t> ComputeSeamWeldMap(const BoundaryEdgeSet& boundary_edges,
                                         const MutableMesh& mutable_mesh) {
  // This function welds together triangles that are geometrically adjacent, but
  // whose shared edge is duplicated in the mesh (i.e., they have distinct, but
  // coincident, vertices).
  //
  // To do this, we organize all of the `boundary_edges` by their geometric
  // positions, and then look for directed edges that are coincident and have
  // opposite orientations, and weld those together by merging their endpoint
  // vertices.

  // Initialize the index_map with identity mapping.
  std::vector<uint32_t> index_map(mutable_mesh.VertexCount());
  std::iota(index_map.begin(), index_map.end(), 0);

  // Union-find type helpers to merge vertices.
  auto find = [&](uint32_t v) {
    while (v != index_map[v]) {
      index_map[v] = index_map[index_map[v]];
      v = index_map[v];
    }
    return v;
  };
  auto merge = [&](uint32_t a, uint32_t b) { index_map[find(a)] = find(b); };

  absl::flat_hash_map<std::pair<Point, Point>,
                      absl::InlinedVector<std::pair<uint32_t, uint32_t>, 1>>
      geometric_boundary;
  for (const auto& [u, v] : boundary_edges) {
    Point pu = mutable_mesh.VertexPosition(u);
    Point pv = mutable_mesh.VertexPosition(v);
    geometric_boundary[{pu, pv}].push_back({u, v});
  }

  for (auto& [seg, edges] : geometric_boundary) {
    if (edges.empty()) continue;

    // Look for coincident edges oriented in the opposite direction.
    auto opp_it = geometric_boundary.find({seg.second, seg.first});
    if (opp_it == geometric_boundary.end()) continue;
    auto& opp_edges = opp_it->second;

    // Pair up and weld the edges.
    while (!edges.empty() && !opp_edges.empty()) {
      auto [u, v] = edges.back();
      edges.pop_back();
      auto [u_opp, v_opp] = opp_edges.back();
      opp_edges.pop_back();
      merge(u, v_opp);
      merge(v, u_opp);
    }
  }

  // Compress paths in index_map.
  for (uint32_t& id : index_map) id = find(id);
  return index_map;
}

// Updates the boundary labels of `root` when welding vertex `v` to `root` in
// `mutable_mesh`.
void MergeWeldedVertexLabels(uint32_t root, uint32_t v,
                             MutableMesh& mutable_mesh) {
  // `root` keeps only the side and forward label categories that match `v` (and
  // otherwise becomes interior). If either category changes, both margins on
  // `root` are set to zero so that anti-aliasing does not outset the vertex.
  StrokeVertex::FormatAttributeIndices attrs =
      StrokeVertex::FindAttributeIndices(mutable_mesh.Format());

  // Returns the encoded label for `attr` at vertex `u` with a margin of 0.
  auto zero_margin_label = [&](uint32_t u, int attr) {
    return StrokeVertex::Label{mutable_mesh.FloatVertexAttribute(u, attr)[0]}
        .WithMargin(0)
        .encoded_value;
  };
  float side = zero_margin_label(root, attrs.side_label);
  float fwd = zero_margin_label(root, attrs.forward_label);
  bool same_side = (side == zero_margin_label(v, attrs.side_label));
  bool same_fwd = (fwd == zero_margin_label(v, attrs.forward_label));
  if (same_side && same_fwd) return;
  mutable_mesh.SetFloatVertexAttribute(root, attrs.side_label,
                                       {same_side ? side : 0.0f});
  mutable_mesh.SetFloatVertexAttribute(root, attrs.forward_label,
                                       {same_fwd ? fwd : 0.0f});
}

// Properties of an input mesh triangle, including the `triangle` geometry, the
// `indices` of its vertices in the result mesh, the `transform` from the mesh's
// coordinate space to the triangle's barycentric coordinates, the geometric
// `heights` (altitudes) of the triangle, and the `attributes` of its vertices.
struct TriangleData {
  Triangle triangle;
  std::array<uint32_t, 3> indices;
  std::array<double, 9> transform;
  std::array<double, 3> heights;
  std::array<InterpolationAttributes, 3> attributes;
};

// A helper class to incrementally construct a `MutableMesh` during mesh
// subtraction. It provides methods for adding vertices (with attribute
// interpolation) and triangles, and maintains an edge-vertex map to weld
// vertices back together along shared edges.
class MeshBuilder {
 public:
  explicit MeshBuilder(const MeshFormat& format) : mutable_mesh_(format) {}

  // Initializes the builder with vertices from `meshes`, welds any coincident
  // boundary edges, and returns a mapping of each vertex in `meshes` to its
  // canonical copy in `mutable_mesh_`.
  std::vector<std::vector<uint32_t>> Initialize(absl::Span<const Mesh> meshes) {
    const MeshFormat& format = mutable_mesh_.Format();

    BoundaryEdgeSet boundary_edges;
    std::vector<std::vector<uint32_t>> index_maps(meshes.size());

    for (size_t m = 0; m < meshes.size(); ++m) {
      const Mesh& mesh = meshes[m];
      index_maps[m].reserve(mesh.VertexCount());

      // Copy the vertices.
      for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
        uint32_t new_index = mutable_mesh_.VertexCount();
        mutable_mesh_.AppendVertex(mesh.VertexPosition(i));
        for (uint32_t attr = 0; attr < format.Attributes().size(); ++attr) {
          if (attr == format.PositionAttributeIndex()) continue;
          mutable_mesh_.SetFloatVertexAttribute(
              new_index, attr, mesh.FloatVertexAttribute(i, attr));
        }
        index_maps[m].push_back(new_index);
      }

      for (uint32_t k = 0; k < mesh.TriangleCount(); ++k) {
        Triangle tri = mesh.GetTriangle(k);
        // To avoid having to deal with subtracting degenerate triangles, we
        // omit their edges, which temporarily creates a topological seam in the
        // mesh that's stitched back together below.
        if (tri.p0 == tri.p1 || tri.p1 == tri.p2 || tri.p2 == tri.p0) continue;

        std::array<uint32_t, 3> t =
            MapIndices(mesh.TriangleIndices(k), index_maps[m]);
        for (int i = 0; i < 3; ++i) {
          uint32_t u = t[i], v = t[(i + 1) % 3];
          if (!boundary_edges.erase({v, u})) boundary_edges.insert({u, v});
        }
      }
    }

    // Weld coincident seam edges and update `index_maps`.
    std::vector<uint32_t> index_remap =
        ComputeSeamWeldMap(boundary_edges, mutable_mesh_);

    // Properly recompute labels for vertices that get welded.
    if (HasAntiAliasingAttributes(format)) {
      for (uint32_t v = 0; v < index_remap.size(); ++v) {
        if (index_remap[v] != v) {
          MergeWeldedVertexLabels(index_remap[v], v, mutable_mesh_);
        }
      }
    }

    // Set the index map for welded vertices.
    for (auto& index_map : index_maps) {
      for (uint32_t& v : index_map) v = index_remap[v];
    }

    return index_maps;
  }

  uint32_t VertexCount() const { return mutable_mesh_.VertexCount(); }

  // Finds an existing vertex or adds one for a point `p` contained in the
  // given `triangle` (whose vertices are assumed to have already been added
  // to the result mesh), and returns its index. The `epsilon` parameter
  // indicates the scale of the geometry.
  uint32_t GetOrAddVertex(Point p, const TriangleData& triangle,
                          float epsilon) {
    if (p == triangle.triangle.p0) return triangle.indices[0];
    if (p == triangle.triangle.p1) return triangle.indices[1];
    if (p == triangle.triangle.p2) return triangle.indices[2];

    std::array<double, 3> weights =
        ComputeBarycentricCoordinates(p, triangle.transform);
    std::array<double, 3> dist = {weights[0] * triangle.heights[0],
                                  weights[1] * triangle.heights[1],
                                  weights[2] * triangle.heights[2]};

    auto min_it = std::min_element(dist.begin(), dist.end());
    if (*min_it < epsilon) {
      return GetOrAddEdgeVertex(p, min_it - dist.begin(), triangle, weights);
    }

    return AddVertex(p, triangle.attributes,
                     std::array{static_cast<float>(weights[0]),
                                static_cast<float>(weights[1]),
                                static_cast<float>(weights[2])});
  }

  // Adds a triangle to the subtraction result mesh.
  void AddTriangle(const std::array<uint32_t, 3>& triangle) {
    ABSL_DCHECK_NE(triangle[0], triangle[1]);
    ABSL_DCHECK_NE(triangle[1], triangle[2]);
    ABSL_DCHECK_NE(triangle[2], triangle[0]);
    mutable_mesh_.AppendTriangleIndices(triangle);
  }

  // Extracts the underlying MutableMesh by moving it, consuming the
  // MeshBuilder.
  MutableMesh ExtractMesh() && { return std::move(mutable_mesh_); }

 private:
  // Adds a vertex at `position` to the subtraction result mesh, with attributes
  // obtained by interpolating the given `vertex_attrs` with the given
  // `weights`, and returns the index of the newly added vertex.
  uint32_t AddVertex(Point position,
                     absl::Span<const InterpolationAttributes> vertex_attrs,
                     absl::Span<const float> weights) {
    uint32_t new_index = mutable_mesh_.VertexCount();
    mutable_mesh_.AppendVertex(position);
    SetInterpolatedAttributes(mutable_mesh_, new_index, vertex_attrs, weights);
    return new_index;
  }

  // Finds an existing vertex or adds one for a point `p` lying along the side
  // `side` of `triangle`, and returns its index.
  uint32_t GetOrAddEdgeVertex(Point p, int side, const TriangleData& triangle,
                              const std::array<double, 3>& weights) {
    // Get vertices of the endpoints of `side`.
    uint32_t v1 = triangle.indices[(side == 2) ? 0 : side + 1];
    uint32_t v2 = triangle.indices[(side == 0) ? 2 : side - 1];

    auto& edge_points = edge_vertex_map_[std::minmax(v1, v2)];
    for (const auto& [existing_p, existing_index] : edge_points) {
      // The subtraction in `outline_processing.h` guarantees identical
      // intersection points for shared edges, regardless of edge orientation.
      if (existing_p == p) return existing_index;
    }

    uint32_t new_index = AddVertex(p, triangle.attributes,
                                   std::array{static_cast<float>(weights[0]),
                                              static_cast<float>(weights[1]),
                                              static_cast<float>(weights[2])});
    edge_points.push_back({p, new_index});
    return new_index;
  }

  MutableMesh mutable_mesh_;

  // A map to help weld triangles back together along split edges. It maps
  // ordered pairs of vertex indices (representing edges in the initial meshes)
  // to a list of newly created vertices along that edge (represented by pairs
  // of their 2D position and vertex index in the output mesh).
  absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                      absl::InlinedVector<std::pair<Point, uint32_t>, 2>>
      edge_vertex_map_;
};

// Computes the geometric boolean difference `tri` - `shape_b` as a
// triangulated polygon.
Triangulation SubtractTriangle(const Triangle& tri,
                               const ShapeOutline& shape_b) {
  ShapeOutline remaining = ComputeSubtraction(ShapeOutline(tri), shape_b);
  auto [vertices, triangles] = ComputeTriangulation(remaining);
  return Triangulation{.vertices = std::move(vertices),
                       .triangles = std::move(triangles)};
}

struct SubtractedMesh {
  MutableMesh mesh;
  std::vector<std::vector<uint32_t>> outlines;
};

// Returns a `SubtractedMesh` representing the subtraction of `shape_b` from
// `meshes`.
SubtractedMesh SubtractMeshes(absl::Span<const Mesh> meshes,
                              const MeshFormat& format,
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

  MeshBuilder sub_mesh(format);

  // Initialize by copying over all vertices from `meshes`, preprocessing the
  // input meshes to filter out degenerate triangles and stitch seams, and
  // obtain a mapping from vertices in `meshes` to their copy in `sub_mesh`.
  std::vector<std::vector<uint32_t>> index_maps = sub_mesh.Initialize(meshes);

  // To help focus (anti-aliasing attribute computations) on only vertices
  // in a neighborhood of the subtraction, keep track of which vertices are
  // created or modified (by an incident triangle being subtracted).
  uint32_t num_input_vertices = sub_mesh.VertexCount();
  std::vector<bool> modified_vertices(num_input_vertices, false);

  // Process the triangles.
  for (size_t mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) {
    const Mesh& mesh = meshes[mesh_idx];
    const std::vector<uint32_t>& index_map = index_maps[mesh_idx];

    for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
      Triangle tri = mesh.GetTriangle(tri_idx);

      // Skip degenerate triangles.
      // TODO(b/521449017): Triangles with coincident vertices arise frequently
      // due to quantization during encoding. Degenerate triangles that have no
      // coincident vertices are expected to be uncommon, but should be handled.
      if (tri.p0 == tri.p1 || tri.p1 == tri.p2 || tri.p2 == tri.p0) continue;

      std::array<uint32_t, 3> old_indices = mesh.TriangleIndices(tri_idx);
      std::array<uint32_t, 3> indices = MapIndices(old_indices, index_map);

      // If there is no intersection with the bounding box, add the triangle
      // and move on.
      if (!Intersects(shape_b, Envelope(tri).AsRect().value())) {
        sub_mesh.AddTriangle(indices);
        continue;
      }

      // Otherwise, compute the subtraction and get a triangulation of the
      // leftover geometry of the triangle.
      Triangulation fragments = SubtractTriangle(tri, shape_b);

      // Mark the corners for anti-aliasing recomputation.
      for (uint32_t v : indices) modified_vertices[v] = true;

      // Early skip if the triangle was entirely erased.
      if (fragments.triangles.empty()) continue;

      // Compute and store some properties of the triangle helpful for
      // interpolation.
      std::array<double, 9> transform = ComputeBarycentricTransform(tri);
      const TriangleData triangle = {
          .triangle = tri,
          .indices = indices,
          .transform = transform,
          .heights = ComputeHeights(tri, transform[8]),
          .attributes = {GetInterpolationAttributes(mesh, old_indices[0]),
                         GetInterpolationAttributes(mesh, old_indices[1]),
                         GetInterpolationAttributes(mesh, old_indices[2])}};

      // Add all the vertices of the fragments and get their indices.
      std::vector<uint32_t> mapped_indices(fragments.vertices.size());
      for (size_t i = 0; i < fragments.vertices.size(); ++i) {
        mapped_indices[i] =
            sub_mesh.GetOrAddVertex(fragments.vertices[i], triangle, epsilon);
      }

      // Add all the triangle fragments.
      for (const auto& frag_tri : fragments.triangles) {
        sub_mesh.AddTriangle(MapIndices(frag_tri, mapped_indices));
      }
    }
  }

  MutableMesh result = std::move(sub_mesh).ExtractMesh();

  // Outline and anti-aliasing attributes computation

  // Vertices created by the subtraction get new labels.
  std::vector<bool> new_vertices(result.VertexCount(), true);
  std::fill_n(new_vertices.begin(), num_input_vertices, false);
  std::vector<std::vector<uint32_t>> outlines =
      ComputeOutlinesAndLabels(result, new_vertices);

  // Vertices that were modified or created (including by
  // `ComputeOutlinesAndLabels`) get new derivatives and margins.
  modified_vertices.resize(result.VertexCount(), true);
  ComputeDerivativesAndMargins(result, modified_vertices);

  return SubtractedMesh{
      .mesh = std::move(result),
      .outlines = std::move(outlines),
  };
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
  return ShapeOutline(loops);
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
    SubtractedMesh subtracted = SubtractMeshes(mesh_a.RenderGroupMeshes(group),
                                               format, shape_b, epsilon);

    group_mutable_meshes[group] = std::move(subtracted.mesh);
    groups_outlines[group] = std::move(subtracted.outlines);
    for (const std::vector<uint32_t>& outline : groups_outlines[group]) {
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
