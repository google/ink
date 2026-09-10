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
#include <limits>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
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
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/brush_tip_extruder/derivative_calculator.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/numbers.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using AverageDerivative =
    ::ink::brush_tip_extruder_internal::DerivativeCalculator::AverageDerivative;
using ::ink::geometry_internal::ComputeSubtraction;
using ::ink::geometry_internal::ComputeTriangulation;
using ::ink::geometry_internal::Intersects;
using ::ink::geometry_internal::ShapeOutline;
using ::ink::numbers::kPi;

using TriangleAttributes =
    absl::InlinedVector<std::array<SmallArray<float, 4>, 3>, 4>;

// Relative error margin for 32-bit floating point operations.
constexpr float kFloatTolerance = 1e-6f;
constexpr float kInfinity = std::numeric_limits<float>::infinity();

// A map of undirected edges to their adjacent triangles. Each edge {u, v} is
// canonically represented as a pair (u, v) with u < v. Each adjacent triangle
// is represented by a pair consisting of its triangle index and a ±1
// orientation indicating whether (u, v) aligns with the triangle's
// counterclockwise order.
using EdgeTriangleAdjacencyMap =
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                        absl::InlinedVector<std::pair<uint32_t, int>, 2>>;

// A representation of a triangulation, consisting of a list of `vertices` and
// `triangles` represented by triplets of indices of vertices.
struct Triangulation {
  std::vector<Point> vertices;
  std::vector<std::array<uint32_t, 3>> triangles;
};

float DistanceSquared(Point a, Point b) { return (a - b).MagnitudeSquared(); }

// Associates the triangle `tri_index` with its directed edge (u,v) in
// `edge_tri_map`, (assuming (u, v) is an edge of counter clockwise oriented
// triangle).
void AddEdgeToAdjacencyMap(uint32_t u, uint32_t v, uint32_t tri_index,
                           EdgeTriangleAdjacencyMap& edge_tri_map) {
  if (u < v) {
    edge_tri_map[{u, v}].push_back({tri_index, 1});
  } else {
    edge_tri_map[{v, u}].push_back({tri_index, -1});
  }
}

// Associates the triangle to its three edges in the given adjacency map.
void AddTriangleToAdjacencyMap(const std::array<uint32_t, 3>& tri,
                               uint32_t tri_index,
                               EdgeTriangleAdjacencyMap& edge_tri_map) {
  AddEdgeToAdjacencyMap(tri[0], tri[1], tri_index, edge_tri_map);
  AddEdgeToAdjacencyMap(tri[1], tri[2], tri_index, edge_tri_map);
  AddEdgeToAdjacencyMap(tri[2], tri[0], tri_index, edge_tri_map);
}

// Remaps the three vertex indices of a triangle using `index_map`.
std::array<uint32_t, 3> MapIndices(const std::array<uint32_t, 3>& tri,
                                   absl::Span<const uint32_t> index_map) {
  return {index_map[tri[0]], index_map[tri[1]], index_map[tri[2]]};
}

// Returns the homogenous transform from world space to the barycentric coords
// for the given triangle.
// TODO(b/932647697): Consider defining this as a separate an externally visible
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

// Maps HCL shifts to coordinates where they interpolate linearly.
//
// The interpolation scheme for HCL shift values is defined via their mapping to
// linear Oklab values, which are interpolated linearly during rasterization.
//
// The per-vertex Oklab colors are obtained by applying the cylindrical shift to
// a uniform (vertex independent) base Oklab color. This function maps the HCL
// shift into coordinates in terms of which linear interpolation will produce
// better results.
//
// LINT.IfChange(hcl_shift_linear_space)
SmallArray<float, 4> HclShiftToLinearSpace(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float hue_shift = 2.0f * kPi * val[0], chroma_shift = val[1];
  val[0] = (chroma_shift + 1) * std::cos(hue_shift);
  val[1] = (chroma_shift + 1) * std::sin(hue_shift);
  return val;
}

SmallArray<float, 4> LinearSpaceToHclShift(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float dx = val[0], dy = val[1];
  val[0] = std::atan2(dy, dx) / (2.0f * kPi);
  val[1] = std::hypot(dx, dy) - 1.0f;
  return val;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hcl_and_opacity_shift)

// Helper function to extract and linearize triangle vertex attributes.
TriangleAttributes GetTriangleAttributes(
    const Mesh& mesh, const std::array<uint32_t, 3>& indices) {
  absl::InlinedVector<std::array<SmallArray<float, 4>, 3>, 4> attributes(
      mesh.Format().Attributes().size());
  for (uint32_t i = 0; i < attributes.size(); ++i) {
    MeshFormat::AttributeId id = mesh.Format().Attributes()[i].id;

    // Since the position is already stored elsewhere, skip loading it.
    if (id == MeshFormat::AttributeId::kPosition) {
      continue;
    }

    // These attributes should not be interpolated.
    if (id == MeshFormat::AttributeId::kSideLabel ||
        id == MeshFormat::AttributeId::kForwardLabel) {
      continue;
    }
    if (id == MeshFormat::AttributeId::kColorShiftHcl) {
      attributes[i] = {
          HclShiftToLinearSpace(mesh.FloatVertexAttribute(indices[0], i)),
          HclShiftToLinearSpace(mesh.FloatVertexAttribute(indices[1], i)),
          HclShiftToLinearSpace(mesh.FloatVertexAttribute(indices[2], i))};
    } else {
      attributes[i] = {mesh.FloatVertexAttribute(indices[0], i),
                       mesh.FloatVertexAttribute(indices[1], i),
                       mesh.FloatVertexAttribute(indices[2], i)};
    }
  }
  return attributes;
}

// LINT.IfChange(boundary_label_encoding)

// A flattened list of all vertex boundary labels. See stroke_vertex.h.
enum BoundaryLabel : int {
  kUndefined = -1,
  kInterior = 0,
  kLeft = 1,
  kRight = 2,
  kFront = 3,
  kLeftFront = 4,
  kRightFront = 5,
  kBack = 6,
  kLeftBack = 7,
  kRightBack = 8,
};

bool IsLeft(BoundaryLabel label) { return label % 3 == 1; }
bool IsRight(BoundaryLabel label) { return label % 3 == 2; }
bool IsFront(BoundaryLabel label) { return label / 3 == 1; }
bool IsBack(BoundaryLabel label) { return label / 3 == 2; }

// Converts encoded float values to a BoundaryLabel enum value.
BoundaryLabel DecodeBoundaryLabel(float side, float fwd) {
  int side_idx = (side > 0.0f) ? 2 : (side < 0.0f ? 1 : 0);
  int fwd_idx = (fwd > 0.0f) ? 2 : (fwd < 0.0f ? 1 : 0);
  return static_cast<BoundaryLabel>(3 * fwd_idx + side_idx);
}

// Converts a BoundaryLabel enum value to its encoded float values.
std::pair<float, float> EncodeBoundaryLabel(BoundaryLabel label) {
  // Encoded float values for interior (0.0), left/front (-127.0), and
  // right/back (127.0) boundary labels.
  constexpr float kLabelValues[] = {0.0f, -127.0f, 127.0f};
  return {kLabelValues[label % 3], kLabelValues[label / 3]};
}

// Returns an edge label (kInterior, kLeft, kRight, kFront, kBack) given the
// labels of its endpoint vertices.
BoundaryLabel GetEdgeLabel(BoundaryLabel u, BoundaryLabel v) {
  if (u == kUndefined || v == kUndefined) return kUndefined;
  int u_side = u % 3, v_side = v % 3;
  int u_fwd = u / 3, v_fwd = v / 3;
  bool same_side = (u_side != 0 && u_side == v_side);
  bool same_fwd = (u_fwd != 0 && u_fwd == v_fwd);
  if (same_side && same_fwd) return kUndefined;
  if (same_side) return static_cast<BoundaryLabel>(u_side);
  if (same_fwd) return static_cast<BoundaryLabel>(3 * u_fwd);
  return kInterior;
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/stroke_vertex.h:margin_encoding,
//     //depot/google3/third_party/ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset,
//     //depot/google3/third_party/ink/rendering/webgpu/StrokeShader.wgsl:calculate_antialiasing_and_position_outset)

// Returns true if `format` has the attributes required for anti-aliasing.
bool HasAntiAliasingAttributes(const MeshFormat& format) {
  StrokeVertex::FormatAttributeIndices attr_indices =
      StrokeVertex::FindAttributeIndices(format);
  return attr_indices.side_label != -1 && attr_indices.forward_label != -1 &&
         attr_indices.side_derivative != -1 &&
         attr_indices.forward_derivative != -1;
}

// Welds coincident topological boundary edges with opposite orientations and
// returns an index map with merged canonical vertex roots.
std::vector<uint32_t> StitchSeamEdges(
    const EdgeTriangleAdjacencyMap& edge_tri_map,
    const MutableMesh& mutable_mesh) {
  // This function welds together triangles that are geometrically adjacent, but
  // whose shared edge is duplicated in the mesh (i.e., they have distinct, but
  // coincident, vertices).
  //
  // The approach is to first compute the topological boundary, consisting of
  // all directed edges adjacent to only one triangle. We then organize the
  // boundary edges by their geometric positions, look for directed edges that
  // are coincident and have opposite orientations, and weld those together by
  // merging their endpoint vertices.

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
  for (const auto& [edge, incident] : edge_tri_map) {
    if (incident.size() != 1) continue;
    auto [u, v] = edge;
    // The `edge_tri_map` stores undirected edges with u < v; orient the
    // directed edge (u -> v) to match the adjacent triangle.
    if (incident[0].second < 0) std::swap(u, v);
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

// Properties of an input mesh triangle, including the `triangle` geometry, the
// `indices` of its vertices in the result mesh, the `transform` from the mesh's
// coordinate space to the triangle's barycentric coordinates, the geometric
// `heights` (altitudes) of the triangle, and the `attributes` of its vertices.
struct TriangleData {
  Triangle triangle;
  std::array<uint32_t, 3> indices;
  std::array<double, 9> transform;
  std::array<double, 3> heights;
  TriangleAttributes attributes;
};

// A helper class to incrementally construct a `MutableMesh` during mesh
// subtraction. It provides methods for adding vertices (with attribute
// interpolation) and triangles, and maintains an edge-vertex map to weld
// vertices back together along shared edges.
class MeshBuilder {
 public:
  explicit MeshBuilder(const MeshFormat& format)
      : mutable_mesh_(format),
        attr_indices_(StrokeVertex::FindAttributeIndices(format)) {}

  // Initializes the builder with vertices from `meshes`, welds any coincident
  // boundary edges, and returns a mapping of each vertex in `meshes` to its
  // canonical copy in `mutable_mesh_`.
  std::vector<std::vector<uint32_t>> Initialize(absl::Span<const Mesh> meshes) {
    const MeshFormat& format = mutable_mesh_.Format();

    EdgeTriangleAdjacencyMap edge_tri_adj;
    std::vector<std::vector<uint32_t>> index_maps(meshes.size());

    // A global index, across all meshes, for each triangle.
    uint32_t tri_idx = 0;
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
        tri_idx += 1;
        Triangle tri = mesh.GetTriangle(k);
        std::array<uint32_t, 3> tri_indices =
            MapIndices(mesh.TriangleIndices(k), index_maps[m]);
        // To avoid having to deal with subtracting degenerate triangles, we
        // omit adding them to the edge adjacency map, which temporarily creates
        // a topological seam in the mesh that's stitched back together below.
        if (tri.p0 == tri.p1 || tri.p1 == tri.p2 || tri.p2 == tri.p0) continue;

        AddTriangleToAdjacencyMap(tri_indices, tri_idx, edge_tri_adj);
      }
    }

    // Stitch seam edges together by welding their vertices.
    std::vector<uint32_t> index_remap =
        StitchSeamEdges(edge_tri_adj, mutable_mesh_);
    for (auto& index_map : index_maps) {
      for (uint32_t& v : index_map) v = index_remap[v];
    }

    return index_maps;
  }

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

    return AddVertex(p, triangle, weights);
  }

  Point GetPosition(uint32_t vertex_index) const {
    return mutable_mesh_.VertexPosition(vertex_index);
  }

  void SetLabel(uint32_t vertex_index, BoundaryLabel label) {
    auto [side, forward] = EncodeBoundaryLabel(label);
    mutable_mesh_.SetFloatVertexAttribute(vertex_index,
                                          attr_indices_.side_label, {side});
    mutable_mesh_.SetFloatVertexAttribute(
        vertex_index, attr_indices_.forward_label, {forward});
  }

  BoundaryLabel GetLabel(uint32_t vertex_index) const {
    float side = mutable_mesh_.FloatVertexAttribute(
        vertex_index, attr_indices_.side_label)[0];
    float fwd = mutable_mesh_.FloatVertexAttribute(
        vertex_index, attr_indices_.forward_label)[0];
    return DecodeBoundaryLabel(side, fwd);
  }

  Vec GetSideDerivative(uint32_t vertex_index) const {
    auto sd = mutable_mesh_.FloatVertexAttribute(vertex_index,
                                                 attr_indices_.side_derivative);
    return {sd[0], sd[1]};
  }

  void SetDerivatives(uint32_t vertex_index, Vec side, Vec forward) {
    // Override zero vectors with a small non-zero value to avoid undefined
    // zero-divided-by-zero in the rendering pipeline.
    // TODO(b/555375080): Remove this once the shader issue is sorted out.
    if (side == Vec{0, 0}) side = {kFloatTolerance, kFloatTolerance};
    if (forward == Vec{0, 0}) forward = {kFloatTolerance, kFloatTolerance};
    mutable_mesh_.SetFloatVertexAttribute(
        vertex_index, attr_indices_.side_derivative, {side.x, side.y});
    mutable_mesh_.SetFloatVertexAttribute(
        vertex_index, attr_indices_.forward_derivative, {forward.x, forward.y});
  }

  const MutableMesh& GetMesh() const { return mutable_mesh_; }

  const auto& GetEdgeTriangleAdjacencyMap() const { return edge_tri_adj_map_; }

  // Extracts the underlying MutableMesh by moving it, consuming the
  // MeshBuilder.
  MutableMesh ExtractMesh() && { return std::move(mutable_mesh_); }

  // Adds a triangle to the subtraction result mesh.
  void AddTriangle(const std::array<uint32_t, 3>& triangle) {
    ABSL_DCHECK_NE(triangle[0], triangle[1]);
    ABSL_DCHECK_NE(triangle[1], triangle[2]);
    ABSL_DCHECK_NE(triangle[2], triangle[0]);
    uint32_t tri_index = mutable_mesh_.TriangleCount();
    mutable_mesh_.AppendTriangleIndices(triangle);
    AddTriangleToAdjacencyMap(triangle, tri_index, edge_tri_adj_map_);
  }

 private:
  // Adds a vertex at `position` to the subtraction result mesh, with attributes
  // obtained by interpolating the given `triangle` attributes with the given
  // barycentric `weights`, and returns the index of the newly added vertex.
  uint32_t AddVertex(Point position, const TriangleData& triangle,
                     const std::array<double, 3>& weights) {
    uint32_t new_index = mutable_mesh_.VertexCount();
    mutable_mesh_.AppendVertex(position);
    const MeshFormat& format = mutable_mesh_.Format();
    ABSL_DCHECK_EQ(triangle.attributes.size(), format.Attributes().size());
    for (uint32_t attr = 0; attr < triangle.attributes.size(); ++attr) {
      MeshFormat::AttributeId id = format.Attributes()[attr].id;

      const std::array<SmallArray<float, 4>, 3>& vals =
          triangle.attributes[attr];
      if (vals[0].Size() == 0) continue;

      SmallArray<float, 4> interp_val(vals[0].Size());
      for (uint8_t c = 0; c < vals[0].Size(); ++c) {
        interp_val[c] = weights[0] * vals[0][c] + weights[1] * vals[1][c] +
                        weights[2] * vals[2][c];
      }

      // Don't forget to map the HCL shift back to proper coordinates.
      if (id == MeshFormat::AttributeId::kColorShiftHcl) {
        interp_val = LinearSpaceToHclShift(interp_val);
      }
      mutable_mesh_.SetFloatVertexAttribute(new_index, attr, interp_val);
    }
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

    uint32_t new_index = AddVertex(p, triangle, weights);
    edge_points.push_back({p, new_index});
    return new_index;
  }

  MutableMesh mutable_mesh_;
  StrokeVertex::FormatAttributeIndices attr_indices_;

  // A map to help weld triangles back together along split edges. It maps
  // ordered pairs of vertex indices (representing edges in the initial meshes)
  // to a list of newly created vertices along that edge (represented by pairs
  // of their 2D position and vertex index in the output mesh).
  absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                      absl::InlinedVector<std::pair<Point, uint32_t>, 2>>
      edge_vertex_map_;

  // Maps an undirected edge {u, v} (represented as (min(u, v), max(u, v))) to
  // its incident triangles.
  EdgeTriangleAdjacencyMap edge_tri_adj_map_;
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

// Computes outlines and returns the (clockwise oriented) outlines of the given
// `mesh`.
std::vector<std::vector<uint32_t>> ComputeOutlines(
    const MeshBuilder& mesh_builder) {
  const MutableMesh& mesh = mesh_builder.GetMesh();

  // To compute the outline, we first iterate through all boundary edges (those
  // with only one adjacent triangle) and build an adjacency map `boundary` that
  // maps each boundary vertex to its next vertex in a counterclockwise walk of
  // the boundary.
  std::vector<int> boundary(mesh.VertexCount(), -1);

  // TODO(b/523326691): Consider initializing `boundary` with the existing mesh
  // outlines to avoid recomputing the outline for untouched parts of the mesh.
  // TODO(b/521449017): Handle pinch points where multiple boundary loops meet
  // at a common vertex.

  for (const auto& [edge, tris] : mesh_builder.GetEdgeTriangleAdjacencyMap()) {
    if (tris.size() != 1) continue;
    auto [u, v] = edge;
    // Orient the edge counter-clockwise.
    if (tris[0].second < 0) std::swap(u, v);
    boundary[u] = v;
  }

  // We now traverse the `boundary` to extract the outline as a sequence of
  // vertices.
  std::vector<std::vector<uint32_t>> outlines;
  for (uint32_t start = 0; start < boundary.size(); ++start) {
    if (boundary[start] < 0) continue;

    std::vector<uint32_t> loop = {start};
    uint32_t curr = start;
    while (boundary[curr] >= 0) {
      uint32_t next = boundary[curr];
      boundary[curr] = -1;
      if (next == start) break;
      loop.push_back(next);
      curr = next;
    }

    if (loop.size() > 2) {
      std::reverse(loop.begin(), loop.end());
      outlines.push_back(std::move(loop));
    }
  }
  return outlines;
}

// LINT.IfChange(compute_labels)

// The alignment cost measures how well the edge label `label` aligns with
// `edge`.
float AlignmentCost(BoundaryLabel label, Vec edge) {
  if (label == kLeft) return edge.x;
  if (label == kRight) return -edge.x;
  if (label == kFront) return edge.y;
  if (label == kBack) return -edge.y;
  if (label == kInterior) return 0.0f;
  return kInfinity;
}

// A helper function to assign boundary labels for the vertices (strictly)
// between indices `i` and `j % n` in `outline`, writing the computed
// labels into `labels`, and returning the total optimal alignment cost.
float ComputeLabels(absl::Span<const uint32_t> outline, size_t i, size_t j,
                    std::vector<BoundaryLabel>& labels,
                    const MeshBuilder& mesh_builder) {
  // Our approach for labeling is to try to assign vertex labels so that the
  // induced edge labels (left, right, front, back) are aligned with their
  // geometric orientation (relative to the original mesh frame, defined by its
  // side derivatives).
  //
  // In practice, this is complicated by the fact that not all edge labels can
  // be lifted to vertex labels, and it's not always clear how to relax the
  // alignment goal to find a feasible edge labeling. Instead, we formulate
  // labeling as an optimization problem: we assign an "alignment cost" to each
  // edge, and choose vertex labels that minimize the total cost across all
  // boundary edges.
  //
  // In terms of the vertex labels (l1, l2, l3, ...), the total cost function
  // has the form
  //    S = cost(l1,l2) + cost(l2,l3) + ...
  // and can be minimized by standard dynamic programming.

  // Accumulated minimum cost to reach each of the 9 candidate BoundaryLabel
  // states at the current step.
  std::array<float, 9> cost;
  // Backpointer table for DP backtracking: bptr[step][curr] stores the index of
  // the preceding label (prev) that yielded the minimum cost for state `curr`.
  std::vector<std::array<int, 9>> bptr(j - i + 1);

  // Initialize the costs.
  cost.fill(kInfinity);
  cost[labels[i]] = 0.0f;

  const size_t n = outline.size();

  Point u = mesh_builder.GetPosition(outline[i]);
  Vec u_sd = mesh_builder.GetSideDerivative(outline[i]);

  for (size_t k = i + 1; k <= j; ++k) {
    size_t v_idx = outline[k % n];
    Point v = mesh_builder.GetPosition(v_idx);
    Vec v_sd = mesh_builder.GetSideDerivative(v_idx);

    Vec edge = u - v;
    Vec sd = ((u_sd + v_sd) * 0.5f).AsUnitVec();
    // Compute the edge in local coordinates of the mesh, using the side
    // derivative sd to define the frame.
    Vec edge_local = {edge.y * sd.x - edge.x * sd.y,
                      -(edge.x * sd.x + edge.y * sd.y)};

    // Compute the transition costs to the next step.
    std::array<float, 9> next_cost;
    next_cost.fill(kInfinity);

    // Iterate over all labels for (u,v)
    for (int u_label = 0; u_label < 9; ++u_label) {
      for (int v_label = 0; v_label < 9; ++v_label) {
        BoundaryLabel edge_label =
            GetEdgeLabel(static_cast<BoundaryLabel>(v_label),
                         static_cast<BoundaryLabel>(u_label));

        // Ignore bad labels (e.g., when the vertex labels give the edge two
        // labels).
        if (edge_label == kUndefined) continue;

        float path_cost = cost[v_label] + AlignmentCost(edge_label, edge_local);
        if (path_cost < next_cost[u_label]) {
          next_cost[u_label] = path_cost;
          bptr[k - i][u_label] = v_label;
        }
      }
    }
    cost = next_cost;
    u = v;
    u_sd = v_sd;
  }

  // Backtrack and update the computed labels in `labels`.
  int curr_label = labels[j % n];
  for (size_t k = j; k > i + 1; --k) {
    curr_label = bptr[k - i][curr_label];
    labels[(k - 1) % n] = static_cast<BoundaryLabel>(curr_label);
  }

  return cost[labels[j % n]];
}

// A helper function to assign boundary labels for the vertices in `outline`,
// writing the computed labels into `labels`.
void ComputeLabels(absl::Span<const uint32_t> outline,
                   std::vector<BoundaryLabel>& labels,
                   const MeshBuilder& mesh_builder) {
  // We follow the optimization approach described above in the `ComputeLabels`
  // overload: we iterate through choices for the first vertex, compute the
  // for each choice the optimal labeling of the remaining vertices, and choose
  // the one with the minimum cost.
  const size_t n = outline.size();
  float best = kInfinity;
  for (int l = 0; l < 9; ++l) {
    std::vector<BoundaryLabel> candidate_labels = labels;
    candidate_labels[0] = static_cast<BoundaryLabel>(l);
    float cost = ComputeLabels(outline, 0, n, candidate_labels, mesh_builder);
    if (cost < best) {
      best = cost;
      labels = std::move(candidate_labels);
    }
  }
}

// Computes anti-aliasing labels for boundary vertices.
void ComputeAndSetLabels(absl::Span<const std::vector<uint32_t>> outlines,
                         MeshBuilder& mesh_builder) {
  // The boundary of the result mesh typically consists of alternating
  // segments of the original mesh boundary and subtracted shape boundary.
  // During the subtraction computation, the labels of the original mesh
  // vertices are copied (see CopyVertex), while those of the subtracted shape
  // are set to kInterior (see GetTriangleAttributes).
  //
  // To avoid recomputing the labels for the untouched portions of the mesh, we
  // instead traverse the outline to identify maximal segments of unlabeled
  // vertices and compute new labels for each.

  for (absl::Span<const uint32_t> outline : outlines) {
    // Read all the vertex labels from the mesh.
    if (outline.empty()) continue;
    const size_t n = outline.size();
    std::vector<BoundaryLabel> labels;
    labels.reserve(n);
    for (uint32_t vertex_index : outline) {
      labels.push_back(mesh_builder.GetLabel(vertex_index));
    }

    // Check for the unlikely case that the entire outline is unlabeled, and
    // handle it specially.
    if (absl::c_all_of(labels,
                       [](BoundaryLabel l) { return l == kInterior; })) {
      ComputeLabels(outline, labels, mesh_builder);
      for (size_t k = 0; k < n; ++k) {
        mesh_builder.SetLabel(outline[k], labels[k]);
      }
      continue;
    }

    // Iterate through to find maximal unlabeled segments.
    for (size_t i = 0; i < n; ++i) {
      if (labels[i] != kInterior && labels[(i + 1) % n] == kInterior) {
        size_t j = i;
        while (labels[(j + 1) % n] == kInterior) ++j;

        ComputeLabels(outline, i, j + 1, labels, mesh_builder);

        for (size_t k = i + 1; k <= j; ++k) {
          mesh_builder.SetLabel(outline[k % n], labels[k % n]);
        }
      }
    }
  }
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/stroke_vertex.h:margin_encoding,
//     //depot/google3/third_party/ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset,
//     //depot/google3/third_party/ink/rendering/webgpu/StrokeShader.wgsl:calculate_antialiasing_and_position_outset)

// LINT.IfChange(compute_derivatives)
// Computes the anti-aliasing derivatives (`side_derivative` and
// `forward_derivative`) for all the vertices in the mesh.
void ComputeAndSetDerivatives(MeshBuilder& mesh_builder) {
  // This function recomputes the anti-aliasing derivatives for all vertices in
  // the mesh, by iterating over all triangles, accumulating each triangle's
  // contribution onto its vertices, averaging the accumulated derivatives per
  // vertex, and then writing them into the mesh.
  // TODO(b/521448869): Without an efficient way to traverse the mesh's
  // adjacency graph, we recompute the derivatives across all vertices rather
  // than restricting to the neighborhood modified by the subtraction.
  //
  // Our approach to computing derivatives here is a bit different than that
  // used during extrusion (see
  // DerivativeCalculator::AddDerivativesForTriangle), primarily to handle the
  // more varied topologies and boundary label configurations that can arise in
  // subtracted meshes.
  //
  // It works roughly as follows (see also
  // strokes/internal/brush_tip_extruder/derivative_calculator.cc and
  // rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h
  // for further background and details):
  //
  // Recall first that the derivatives are used during rendering to perform
  // anti-aliasing, specifically to 1) outset the vertices on the mesh boundary
  // and 2) compute distances of pixel fragments to the mesh boundary in order
  // to estimate pixel coverage.
  //
  // More precisely, for a triangle (p0, p1, p2), the boundary labels of the
  // vertices define four normalized "distance" fields {f_left, f_right, f_back,
  // f_front} that map a point p in the triangle to its approximate distance (in
  // barycentric coordinates) to the respective boundary. These have the form,
  //    f_s(p) = 1 - \sum_{i=0}^2  \lambda_i(p) * OnSide(s, p_i),
  // where
  //   s is a side in {left, right, front, back},
  //   OnSide(s, p_i) is 1 if vertex p_i is (labeled) on side s and 0 otherwise,
  //   \lambda_i is the barycentric weight function for the vertex i.
  //
  // The derivative vectors for each side are defined as,
  //   derivative_s = \nabla f_s / | \nabla f_s |^2,
  // where \nabla is the standard gradient. These vectors point normal to the
  // boundary, and are scaled so that f_s * |derivative_s| is the distance to
  // the boundary in stroke units.
  //
  // Because the mesh format stores only a single side-derivative (left/right)
  // and a forward-derivative (front/back), we write to each vertex the side
  // and forward derivatives matching its side and forward labels
  // (with interior vertices accumulating both).

  const MutableMesh& mesh = mesh_builder.GetMesh();
  std::vector<AverageDerivative> side_derivative(mesh.VertexCount());
  std::vector<AverageDerivative> forward_derivative(mesh.VertexCount());

  for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
    std::array<uint32_t, 3> indices = mesh.TriangleIndices(tri_idx);
    Triangle triangle = mesh.GetTriangle(tri_idx);
    float area = triangle.SignedArea();
    if (area == 0.0f) continue;

    std::array<BoundaryLabel, 3> labels;
    for (int k = 0; k < 3; ++k) {
      labels[k] = mesh_builder.GetLabel(indices[k]);
    }

    // The gradients of the distance functions can be computed in terms of the
    // gradients of the barycentric weight functions as,
    //    \nabla f_s(p) = \sum_{i=0}^2  -\nabla \lambda_i(p) * OnSide(s, p_i).
    // The gradients of the barycentric functions can be computed by recalling
    // that \lambda_i is linear with \lambda_i(p_i) = 1 and
    // \lambda_i(p_j) = \lambda_i(p_k) = 0, so that \nabla \lambda_i is
    // orthogonal to the opposite edge (p_j, p_k) with length 1 / altitude_i =
    // |p_j p_k| / (2 area).
    //
    // For numerical stability, we'll normalize by the factor of twice area
    // once-for-all at the end, when we compute the derivatives.

    // Gradients of the un-normalized barycentric functions ulambda = 2 area
    // \lambda.
    std::array<Vec, 3> grad_ulambda = {
        (triangle.p2 - triangle.p1).Orthogonal(),
        (triangle.p0 - triangle.p2).Orthogonal(),
        (triangle.p1 - triangle.p0).Orthogonal(),
    };

    // Gradients of the un-normalized distance functions uf_s = 2 area f_s.
    Vec grad_uf_left, grad_uf_right, grad_uf_front, grad_uf_back;
    for (int i = 0; i < 3; ++i) {
      if (IsLeft(labels[i])) grad_uf_left -= grad_ulambda[i];
      if (IsRight(labels[i])) grad_uf_right -= grad_ulambda[i];
      if (IsFront(labels[i])) grad_uf_front -= grad_ulambda[i];
      if (IsBack(labels[i])) grad_uf_back -= grad_ulambda[i];
    }

    float two_area = 2.0f * area;
    auto rescale = [two_area](Vec grad) -> Vec {
      float mag_sq = grad.MagnitudeSquared();
      return (mag_sq > two_area * kFloatTolerance)
                 ? ((two_area / mag_sq) * grad)
                 : Vec{0, 0};
    };
    Vec derivative_left = rescale(grad_uf_left);
    Vec derivative_right = rescale(grad_uf_right);
    Vec derivative_front = rescale(grad_uf_front);
    Vec derivative_back = rescale(grad_uf_back);

    // Add the derivatives onto the vertices.
    for (int i = 0; i < 3; ++i) {
      uint32_t idx = indices[i];
      if (!IsLeft(labels[i])) side_derivative[idx].Add(-derivative_right);
      if (!IsRight(labels[i])) side_derivative[idx].Add(derivative_left);
      if (!IsFront(labels[i])) forward_derivative[idx].Add(-derivative_back);
      if (!IsBack(labels[i])) forward_derivative[idx].Add(derivative_front);
    }
  }

  for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
    mesh_builder.SetDerivatives(i, side_derivative[i].Value(),
                                forward_derivative[i].Value());
  }
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/brush_tip_extruder/derivative_calculator.cc,
//     //depot/google3/third_party/ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset,
//     //depot/google3/third_party/ink/rendering/webgpu/StrokeShader.wgsl:calculate_antialiasing_and_position_outset)

struct SubtractedMesh {
  MutableMesh mesh;
  std::vector<std::vector<uint32_t>> outlines;
};

// Returns a `SubtractedMesh` representing the subtraction of `shape_b` from
// `meshes`.
SubtractedMesh SubtractMeshes(absl::Span<const Mesh> meshes,
                              const MeshFormat& format,
                              const ShapeOutline& shape_b, float epsilon,
                              bool anti_aliasing_enabled) {
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
          .attributes = GetTriangleAttributes(mesh, old_indices)};

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

  std::vector<std::vector<uint32_t>> outlines = ComputeOutlines(sub_mesh);

  if (HasAntiAliasingAttributes(format) && anti_aliasing_enabled) {
    ComputeAndSetLabels(outlines, sub_mesh);
    ComputeAndSetDerivatives(sub_mesh);
  }

  return SubtractedMesh{
      .mesh = std::move(sub_mesh).ExtractMesh(),
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
                                         float epsilon,
                                         bool anti_aliasing_enabled) {
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
    SubtractedMesh subtracted =
        SubtractMeshes(mesh_a.RenderGroupMeshes(group), format, shape_b,
                       epsilon, anti_aliasing_enabled);

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
