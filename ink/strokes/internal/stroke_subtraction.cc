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
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/numbers.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::ComputeSubtraction;
using ::ink::geometry_internal::ComputeTriangulation;
using ::ink::geometry_internal::Intersects;
using ::ink::geometry_internal::ShapeOutline;
using ::ink::numbers::kPi;

using TriangleAttributes =
    absl::InlinedVector<std::array<SmallArray<float, 4>, 3>, 4>;

constexpr float kInfinity = std::numeric_limits<float>::infinity();

float DistanceSquared(Point a, Point b) { return (a - b).MagnitudeSquared(); }

// A representation of a triangulation, consisting of a list of `vertices` and
// `triangles` represented by triplets of indices of vertices.
struct Triangulation {
  std::vector<Point> vertices;
  std::vector<std::array<uint32_t, 3>> triangles;
};

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
    if (id == MeshFormat::AttributeId::kColorShiftHsl) {
      attributes[i] = {
          HslShiftToLinearSpace(mesh.FloatVertexAttribute(indices[0], i)),
          HslShiftToLinearSpace(mesh.FloatVertexAttribute(indices[1], i)),
          HslShiftToLinearSpace(mesh.FloatVertexAttribute(indices[2], i))};
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
bool HasBoundaryLabels(const MeshFormat& format) {
  StrokeVertex::FormatAttributeIndices attr_indices =
      StrokeVertex::FindAttributeIndices(format);
  return attr_indices.side_label != -1 && attr_indices.forward_label != -1 &&
         attr_indices.side_derivative != -1 &&
         attr_indices.forward_derivative != -1;
}

// Properties of an input mesh triangle, including the `indices` of the points
// in the mesh, the `transform` from the mesh's coordinate space to the
// triangle's barycentric coordinates, the geometric `heights` (altitudes) of
// the triangle, and mesh vertex attributes `attributes` of its vertices.
struct TriangleData {
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

  // Copies the vertex and its attributes from the given `mesh` at
  // `vertex_index` and appends it to this mesh, and returns
  // the index of the newly added vertex in this mesh.
  uint32_t CopyVertex(const Mesh& mesh, uint32_t vertex_index) {
    uint32_t new_index = mutable_mesh_.VertexCount();
    mutable_mesh_.AppendVertex(mesh.VertexPosition(vertex_index));
    const MeshFormat& format = mutable_mesh_.Format();
    for (uint32_t i = 0; i < format.Attributes().size(); ++i) {
      if (i == format.PositionAttributeIndex()) continue;
      mutable_mesh_.SetFloatVertexAttribute(
          new_index, i, mesh.FloatVertexAttribute(vertex_index, i));
    }
    return new_index;
  }

  // Finds an existing vertex or adds one for a point `p` contained in the
  // given `triangle` (whose vertices are assumed to have already been added
  // to the result mesh), and returns its index. The `epsilon` parameter
  // specifies the tolerance for snapping to vertices/edges.
  uint32_t GetOrAddVertex(Point p, const TriangleData& triangle,
                          float epsilon) {
    std::array<double, 3> weights =
        ComputeBarycentricCoordinates(p, triangle.transform);
    std::array<bool, 3> near_zero = {
        weights[0] * triangle.heights[0] < epsilon,
        weights[1] * triangle.heights[1] < epsilon,
        weights[2] * triangle.heights[2] < epsilon};

    if (near_zero[1] && near_zero[2]) return triangle.indices[0];
    if (near_zero[2] && near_zero[0]) return triangle.indices[1];
    if (near_zero[0] && near_zero[1]) return triangle.indices[2];

    for (int side = 0; side < 3; ++side) {
      if (near_zero[side]) {
        return GetOrAddEdgeVertex(p, side, triangle, weights, epsilon);
      }
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

  const MutableMesh& GetMesh() const { return mutable_mesh_; }

  // Extracts the underlying MutableMesh by moving it, consuming the
  // MeshBuilder.
  MutableMesh ExtractMesh() && { return std::move(mutable_mesh_); }

  // Adds a triangle to the subtraction result mesh.
  void AddTriangle(const std::array<uint32_t, 3>& triangle) {
    mutable_mesh_.AppendTriangleIndices(triangle);
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

      // Don't forget to map the HSL shift back to proper coordinates.
      if (id == MeshFormat::AttributeId::kColorShiftHsl) {
        interp_val = LinearSpaceToHslShift(interp_val);
      }
      mutable_mesh_.SetFloatVertexAttribute(new_index, attr, interp_val);
    }
    return new_index;
  }

  // Finds an existing vertex or adds one for a point `p` lying along the side
  // `side` of `triangle`, and returns its index.
  uint32_t GetOrAddEdgeVertex(Point p, int side, const TriangleData& triangle,
                              const std::array<double, 3>& weights,
                              float epsilon) {
    // Get vertices of the endpoints of `side`.
    uint32_t v1 = triangle.indices[(side == 2) ? 0 : side + 1];
    uint32_t v2 = triangle.indices[(side == 0) ? 2 : side - 1];

    if (v1 > v2) std::swap(v1, v2);

    auto& edge_points = edge_vertex_map_[{v1, v2}];
    for (const auto& [existing_p, existing_index] : edge_points) {
      if (DistanceSquared(p, existing_p) <= epsilon * epsilon)
        return existing_index;
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
std::vector<std::vector<uint32_t>> ComputeOutlines(const MutableMesh& mesh) {
  // To compute the outline, we first compute the "simplicial boundary"
  // (the set of directed boundary edges of triangles that aren't
  // "cancelled" out by an adjacent triangle) of the mesh. Then we trace the
  // boundary edges to extract the outline.
  // TODO(b/523326691): Consider initializing `boundary` with the existing mesh
  // outlines to avoid recomputing the outline for untouched parts of the mesh.

  // We store the simplicial boundary in a vertex adjacency list `boundary`,
  // representing directed edges of the mesh. As we add triangle boundaries, we
  // make sure to remove cancelled edges (i.e. if (v, u) is already present in
  // the adjacency list, adding (u, v) removes it). The capacity of 4 is chosen
  // as 4 is the typical degree of a vertex in a triangle strip.
  std::vector<absl::InlinedVector<uint32_t, 4>> boundary(mesh.VertexCount());
  for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
    std::array<uint32_t, 3> triangle = mesh.TriangleIndices(tri_idx);
    uint32_t u = triangle[2];
    for (uint32_t v : triangle) {
      if (absl::erase_if(boundary[v], [u](uint32_t x) { return x == u; }) ==
          0) {
        boundary[u].push_back(v);
      }
      u = v;
    }
  }

  // `boundary` now is an adjaceny map of the boundary outline of the graph: for
  // any boundary vertex u, `boundary[u]` is the singleton list consisting of
  // the successor vertex of `u` in a counterclockwise walk of the boundary. We
  // now traverse the `boundary` to extract the outline as a sequence of
  // vertices.
  std::vector<std::vector<uint32_t>> outlines;
  for (uint32_t start = 0; start < boundary.size(); ++start) {
    while (!boundary[start].empty()) {
      std::vector<uint32_t> loop = {start};
      uint32_t curr = start;
      while (!boundary[curr].empty()) {
        uint32_t next = boundary[curr].back();
        boundary[curr].pop_back();
        if (next == start) break;
        loop.push_back(next);
        curr = next;
      }

      if (loop.size() > 2) {
        std::reverse(loop.begin(), loop.end());
        outlines.push_back(std::move(loop));
      }
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
  if (label == kFront) return -edge.y;
  if (label == kBack) return edge.y;
  if (label == kInterior) return 0.0f;
  return kInfinity;
}

// A helper function to assign boundary labels for the vertices (strictly)
// between indices `i` and `j % n` in `outline`, writing the computed
// labels into `labels`.
void ComputeLabels(absl::Span<const uint32_t> outline, size_t i, size_t j,
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
    const size_t n = outline.size();
    std::vector<BoundaryLabel> labels;
    labels.reserve(n);
    for (uint32_t vertex_index : outline) {
      labels.push_back(mesh_builder.GetLabel(vertex_index));
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

    // TODO(b/521449017): handle case when entire boundary is erased.
  }
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/stroke_vertex.h:margin_encoding,
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

  // Copy over all of the vertices. Vertices not belonging to any triangle
  // will be removed by PartitionedMesh::FromMutableMeshGroups.
  for (const Mesh& mesh : meshes) {
    for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
      sub_mesh.CopyVertex(mesh, i);
    }
  }

  // Process the triangles.
  uint32_t vertex_offset = 0;
  for (const Mesh& mesh : meshes) {
    for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
      std::array<uint32_t, 3> old_indices = mesh.TriangleIndices(tri_idx);

      // Indices of the copied over vertices of the triangle in the result mesh.
      std::array<uint32_t, 3> indices = {old_indices[0] + vertex_offset,
                                         old_indices[1] + vertex_offset,
                                         old_indices[2] + vertex_offset};

      Triangle tri = mesh.GetTriangle(tri_idx);

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
      for (const std::array<uint32_t, 3>& fragment : fragments.triangles) {
        std::array<uint32_t, 3> fragment_triangle = {
            mapped_indices[fragment[0]], mapped_indices[fragment[1]],
            mapped_indices[fragment[2]]};
        if (fragment_triangle[0] == fragment_triangle[1] ||
            fragment_triangle[1] == fragment_triangle[2] ||
            fragment_triangle[2] == fragment_triangle[0]) {
          continue;
        }
        sub_mesh.AddTriangle(fragment_triangle);
      }
    }

    vertex_offset += mesh.VertexCount();
  }

  std::vector<std::vector<uint32_t>> outlines =
      ComputeOutlines(sub_mesh.GetMesh());

  if (HasBoundaryLabels(format)) ComputeAndSetLabels(outlines, sub_mesh);

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
