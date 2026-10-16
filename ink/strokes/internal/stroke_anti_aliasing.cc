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

#include "ink/strokes/internal/stroke_anti_aliasing.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/brush_tip_extruder/derivative_calculator.h"
#include "ink/strokes/internal/mesh_attribute_interpolation.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::strokes_internal {
namespace {

using ::ink::brush_tip_extruder_internal::DerivativeCalculator;
using AttributeIndices = StrokeVertex::FormatAttributeIndices;
using AverageDerivative = DerivativeCalculator::AverageDerivative;
using ForwardCategory = StrokeVertex::ForwardCategory;
using Label = StrokeVertex::Label;
using SideCategory = StrokeVertex::SideCategory;
using StrokeVertex::ForwardCategory::kExteriorBack;
using StrokeVertex::ForwardCategory::kExteriorFront;
using StrokeVertex::SideCategory::kExteriorLeft;
using StrokeVertex::SideCategory::kExteriorRight;

// Relative error margin for 32-bit floating point operations.
constexpr float kFloatTolerance = 1e-6f;
constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr float kMaximumMargin = StrokeVertex::kMaximumMargin;

// A map of undirected edges to their incident triangles. Each edge {u, v} is
// canonically represented as a pair (u, v) with u < v. Each incident triangle
// is represented by a pair consisting of its triangle index and a ±1
// orientation indicating whether (u, v) aligns with the triangle's
// counterclockwise order.
using EdgeTriangleIncidenceMap =
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>,
                        absl::InlinedVector<std::pair<uint32_t, int>, 2>>;

// Associates the triangle `tri_index` with its directed edge (u,v) in
// `edge_tri_map`, (assuming (u, v) is an edge of counter clockwise oriented
// triangle).
void AddEdgeToIncidenceMap(uint32_t u, uint32_t v, uint32_t tri_index,
                           EdgeTriangleIncidenceMap& edge_tri_map) {
  if (u < v) {
    edge_tri_map[{u, v}].push_back({tri_index, 1});
  } else {
    edge_tri_map[{v, u}].push_back({tri_index, -1});
  }
}

// Returns the edge-triangle incidence map of `mesh`.
EdgeTriangleIncidenceMap ComputeEdgeTriangleMap(const MutableMesh& mesh) {
  EdgeTriangleIncidenceMap edge_tri_map;
  for (uint32_t t = 0; t < mesh.TriangleCount(); ++t) {
    std::array<uint32_t, 3> tri = mesh.TriangleIndices(t);
    AddEdgeToIncidenceMap(tri[0], tri[1], t, edge_tri_map);
    AddEdgeToIncidenceMap(tri[1], tri[2], t, edge_tri_map);
    AddEdgeToIncidenceMap(tri[2], tri[0], t, edge_tri_map);
  }
  return edge_tri_map;
}

// Returns the vertex in `tri` opposite to the edge (`u`, `v`).
uint32_t OppositeVertex(const std::array<uint32_t, 3>& tri, uint32_t u,
                        uint32_t v) {
  if (tri[0] != u && tri[0] != v) return tri[0];
  if (tri[1] != u && tri[1] != v) return tri[1];
  ABSL_DCHECK(tri[2] != u && tri[2] != v);
  return tri[2];
}

// Returns the (clockwise oriented) outlines of a mesh with the given
// `edge_tri_map`.
std::vector<std::vector<uint32_t>> ComputeOutlines(
    const EdgeTriangleIncidenceMap& edge_tri_map) {
  // To compute the outline, we first iterate through all boundary edges (those
  // with only one incident triangle) and build an adjacency map `boundary` that
  // maps each boundary vertex to its next vertex in a counterclockwise walk of
  // the boundary.
  std::vector<int> boundary;

  // TODO(b/523326691): Consider initializing `boundary` with the existing mesh
  // outlines to avoid recomputing the outline for untouched parts of the mesh.
  // TODO(b/521449017): Handle pinch points where multiple boundary loops meet
  // at a common vertex.

  for (const auto& [edge, tris] : edge_tri_map) {
    if (tris.size() != 1) continue;
    auto [u, v] = edge;
    if (v >= boundary.size()) boundary.resize(v + 1, -1);
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

// Converts `StrokeVertex::Label` side and forward labels to a `BoundaryLabel`.
BoundaryLabel DecodeBoundaryLabel(Label side, Label fwd) {
  auto index = [](Label label) {
    float sign = label.DerivativeOutsetSign();
    return sign < 0 ? 1 : (sign > 0 ? 2 : 0);
  };
  return static_cast<BoundaryLabel>(3 * index(fwd) + index(side));
}

// Converts a `BoundaryLabel` to `StrokeVertex::Label` side and forward labels.
std::pair<Label, Label> EncodeBoundaryLabel(BoundaryLabel label) {
  constexpr Label kSideLabels[] = {
      StrokeVertex::kInteriorLabel,
      StrokeVertex::kExteriorLeftLabel,
      StrokeVertex::kExteriorRightLabel,
  };
  constexpr Label kForwardLabels[] = {
      StrokeVertex::kInteriorLabel,
      StrokeVertex::kExteriorFrontLabel,
      StrokeVertex::kExteriorBackLabel,
  };
  return {kSideLabels[label % 3], kForwardLabels[label / 3]};
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

// Returns true if `attrs` has the attributes required for anti-aliasing.
bool HasAntiAliasingAttributes(const AttributeIndices& attrs) {
  return attrs.side_label != -1 && attrs.forward_label != -1 &&
         attrs.side_derivative != -1 && attrs.forward_derivative != -1;
}

Label GetSideLabel(const MutableMesh& mesh, const AttributeIndices& attrs,
                   uint32_t v) {
  return {mesh.FloatVertexAttribute(v, attrs.side_label)[0]};
}

Label GetFwdLabel(const MutableMesh& mesh, const AttributeIndices& attrs,
                  uint32_t v) {
  return {mesh.FloatVertexAttribute(v, attrs.forward_label)[0]};
}

void SetLabel(MutableMesh& mesh, const AttributeIndices& attrs, uint32_t v,
              BoundaryLabel label) {
  auto [side, forward] = EncodeBoundaryLabel(label);
  mesh.SetFloatVertexAttribute(v, attrs.side_label, {side.encoded_value});
  mesh.SetFloatVertexAttribute(v, attrs.forward_label, {forward.encoded_value});
}

BoundaryLabel GetLabel(const MutableMesh& mesh, const AttributeIndices& attrs,
                       uint32_t v) {
  return DecodeBoundaryLabel(GetSideLabel(mesh, attrs, v),
                             GetFwdLabel(mesh, attrs, v));
}

// Returns the {side, forward} margins of vertex `v`.
std::pair<float, float> GetMargins(const MutableMesh& mesh,
                                   const AttributeIndices& attrs, uint32_t v) {
  return {GetSideLabel(mesh, attrs, v).DecodeMargin(),
          GetFwdLabel(mesh, attrs, v).DecodeMargin()};
}

void SetMargins(MutableMesh& mesh, const AttributeIndices& attrs, uint32_t v,
                float side_margin, float forward_margin) {
  mesh.SetFloatVertexAttribute(
      v, attrs.side_label,
      {GetSideLabel(mesh, attrs, v).WithMargin(side_margin).encoded_value});
  mesh.SetFloatVertexAttribute(
      v, attrs.forward_label,
      {GetFwdLabel(mesh, attrs, v).WithMargin(forward_margin).encoded_value});
}

Vec GetSideDerivative(const MutableMesh& mesh, const AttributeIndices& attrs,
                      uint32_t v) {
  auto sd = mesh.FloatVertexAttribute(v, attrs.side_derivative);
  return {sd[0], sd[1]};
}

Vec GetForwardDerivative(const MutableMesh& mesh, const AttributeIndices& attrs,
                         uint32_t v) {
  auto fd = mesh.FloatVertexAttribute(v, attrs.forward_derivative);
  return {fd[0], fd[1]};
}

void SetDerivatives(MutableMesh& mesh, const AttributeIndices& attrs,
                    uint32_t v, Vec side, Vec forward) {
  mesh.SetFloatVertexAttribute(v, attrs.side_derivative, {side.x, side.y});
  mesh.SetFloatVertexAttribute(v, attrs.forward_derivative,
                               {forward.x, forward.y});
}

// Subdivides the interior edge between `u` and `v` by inserting a midpoint
// vertex (labeled interior), splitting its incident triangles, and updating
// `edge_tri_map`.
void SubdivideEdge(uint32_t u, uint32_t v, MutableMesh& mesh,
                   const AttributeIndices& attrs,
                   EdgeTriangleIncidenceMap& edge_tri_map) {
  // To subdivide the edge, we first add a vertex at the midpoint of the edge,
  // then break each incident triangle up into two pieces, and then update the
  // edge-triangle incidence map.
  uint32_t m = mesh.VertexCount();
  mesh.AppendVertex(
      Segment{mesh.VertexPosition(u), mesh.VertexPosition(v)}.Midpoint());
  SetInterpolatedAttributes(mesh, m,
                            std::array{GetInterpolationAttributes(mesh, u),
                                       GetInterpolationAttributes(mesh, v)},
                            /*weights=*/std::array{0.5f, 0.5f});
  SetLabel(mesh, attrs, m, kInterior);

  // Get the incident triangles.
  auto it = edge_tri_map.find(std::minmax(u, v));
  if (it == edge_tri_map.end()) return;
  auto incident_triangles = it->second;

  // Remove the old edge from the incidence map.
  edge_tri_map.erase(it);

  for (const auto& [tri_idx, orientation] : incident_triangles) {
    // Orient the triangle vertices as (a, b, c) in counterclockwise order,
    // with (a, b) as the directed subdivided edge.
    std::array<uint32_t, 3> tri = mesh.TriangleIndices(tri_idx);
    uint32_t a = std::min(u, v), b = std::max(u, v);
    if (orientation < 0) std::swap(a, b);
    uint32_t c = OppositeVertex(tri, u, v);
    //          c
    //         /|\
    //        / | \
    //       /  |  \
    //      / T1|T2 \
    //     /    |    \
    //    a-----m-----b

    // With the edge (a,b) subdivided, the triangle is now a quad
    // (a, m, b, c), which we break into two new triangles.

    // Reuse the index of the old triangle, for triangle T1
    uint32_t t1_idx = tri_idx;
    mesh.SetTriangleIndices(t1_idx, {c, a, m});

    // Add a triangle for triangle T2.
    uint32_t t2_idx = mesh.TriangleCount();
    mesh.AppendTriangleIndices({c, m, b});

    // Now some bookkeeping to update the edge-triangle incidence map:

    // Since T1 uses the same index as the old triangle, no need to update
    // (a,c). On the other hand, edge (b, c) is now incident to T2
    for (auto& [t_idx, orient] : edge_tri_map[std::minmax(b, c)]) {
      if (t_idx == t1_idx) {
        t_idx = t2_idx;
        break;
      }
    }
    // Then add all the new edges.
    AddEdgeToIncidenceMap(a, m, t1_idx, edge_tri_map);
    AddEdgeToIncidenceMap(m, c, t1_idx, edge_tri_map);
    AddEdgeToIncidenceMap(m, b, t2_idx, edge_tri_map);
    AddEdgeToIncidenceMap(c, m, t2_idx, edge_tri_map);
  }
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
                    std::vector<BoundaryLabel>& labels, const MutableMesh& mesh,
                    const AttributeIndices& attrs) {
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

  Point u = mesh.VertexPosition(outline[i]);
  Vec u_sd = GetSideDerivative(mesh, attrs, outline[i]);

  for (size_t k = i + 1; k <= j; ++k) {
    size_t v_idx = outline[k % n];
    Point v = mesh.VertexPosition(v_idx);
    Vec v_sd = GetSideDerivative(mesh, attrs, v_idx);

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
                   std::vector<BoundaryLabel>& labels, const MutableMesh& mesh,
                   const AttributeIndices& attrs) {
  // We follow the optimization approach described above in the `ComputeLabels`
  // overload: we iterate through choices for the first vertex, compute
  // for each choice the optimal labeling of the remaining vertices, and choose
  // the one with the minimum cost.
  const size_t n = outline.size();
  float best = kInfinity;
  for (int l = 0; l < 9; ++l) {
    std::vector<BoundaryLabel> candidate_labels = labels;
    candidate_labels[0] = static_cast<BoundaryLabel>(l);
    float cost = ComputeLabels(outline, 0, n, candidate_labels, mesh, attrs);
    if (cost < best) {
      best = cost;
      labels = std::move(candidate_labels);
    }
  }
}

}  // namespace

std::vector<std::vector<uint32_t>> ComputeOutlinesAndLabels(
    MutableMesh& mesh, const std::vector<bool>& vertex_mask) {
  ABSL_DCHECK(vertex_mask.empty() || vertex_mask.size() == mesh.VertexCount());

  EdgeTriangleIncidenceMap edge_tri_map = ComputeEdgeTriangleMap(mesh);
  std::vector<std::vector<uint32_t>> outlines = ComputeOutlines(edge_tri_map);

  AttributeIndices attrs = StrokeVertex::FindAttributeIndices(mesh.Format());
  if (!HasAntiAliasingAttributes(attrs)) return outlines;

  // Typically, `vertex_mask` selects contiguous segments along the mesh
  // boundary (for example, for example in a subtracted mesh, the boundary of
  // the the subtracted shape consists of alternating segments of the original
  // mesh and the subtracted shape). We traverse each outline to identify
  // maximal segments of masked vertices and compute new labels for each.
  //
  // To simplify computing labels, we first assign vertex labels based purely on
  // boundary edges. This however can cause interior edges to mistakenly be
  // labeled with a boundary label. To rectify this, in a subsequent
  // post-processing step, we subdivide any such mislabeled interior edges.
  for (const std::vector<uint32_t>& outline : outlines) {
    const size_t n = outline.size();

    std::vector<BoundaryLabel> labels;
    labels.reserve(n);
    for (uint32_t v : outline) {
      labels.push_back((vertex_mask.empty() || vertex_mask[v])
                           ? kUndefined
                           : GetLabel(mesh, attrs, v));
    }

    // Check for the case that the entire outline is unlabeled, and handle it
    // specially.
    if (absl::c_all_of(labels,
                       [](BoundaryLabel l) { return l == kUndefined; })) {
      ComputeLabels(outline, labels, mesh, attrs);
      for (size_t k = 0; k < n; ++k)
        SetLabel(mesh, attrs, outline[k], labels[k]);
      continue;
    }

    // Iterate through to find maximal unlabeled segments.
    for (size_t i = 0; i < n; ++i) {
      if (labels[i] != kUndefined && labels[(i + 1) % n] == kUndefined) {
        size_t j = i;
        while (labels[(j + 1) % n] == kUndefined) ++j;

        ComputeLabels(outline, i, j + 1, labels, mesh, attrs);

        for (size_t k = i + 1; k <= j; ++k) {
          SetLabel(mesh, attrs, outline[k % n], labels[k % n]);
        }
      }
    }
  }

  // Find all mislabeled interior edges and subdivide them.
  // TODO(b/523326691): Try edge-flipping where feasible instead of subdividing.
  std::vector<std::pair<uint32_t, uint32_t>> mislabeled;
  for (const auto& [edge, tris] : edge_tri_map) {
    if (tris.size() != 2) continue;
    const auto [u, v] = edge;
    if (GetEdgeLabel(GetLabel(mesh, attrs, u), GetLabel(mesh, attrs, v)) !=
        kInterior) {
      mislabeled.push_back(edge);
    }
  }

  for (const auto& [u, v] : mislabeled) {
    SubdivideEdge(u, v, mesh, attrs, edge_tri_map);
  }
  return outlines;
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/stroke_vertex.h:margin_encoding,
//     //depot/google3/third_party/ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset,
//     //depot/google3/third_party/ink/rendering/webgpu/StrokeShader.wgsl:calculate_antialiasing_and_position_outset)

// LINT.IfChange(compute_derivatives)
void ComputeDerivativesAndMargins(MutableMesh& mesh,
                                  const std::vector<bool>& vertex_mask) {
  ABSL_DCHECK(vertex_mask.empty() || vertex_mask.size() == mesh.VertexCount());
  AttributeIndices attrs = StrokeVertex::FindAttributeIndices(mesh.Format());
  if (!HasAntiAliasingAttributes(attrs)) return;

  // Computing derivatives.
  // We recompute the derivatives by iterating over all triangles, accumulating
  // each triangle's contribution onto its vertices, averaging the accumulated
  // derivatives per vertex, and then writing them into the mesh. Only vertices
  // with `vertex_mask` set are updated; triangles with no such vertex are
  // skipped.
  // TODO(b/523326691): Process only the triangles in a neighborhood of the
  // recomputed vertices rather than scanning all triangles in the mesh.
  //
  // Our approach to computing derivatives here is a bit different than that
  // used during extrusion (see
  // DerivativeCalculator::AddDerivativesForTriangle), primarily to handle the
  // more varied topologies and boundary label configurations that can arise in
  // e.g. subtracted meshes.
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
  std::vector<AverageDerivative> side_derivative(mesh.VertexCount());
  std::vector<AverageDerivative> forward_derivative(mesh.VertexCount());

  for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
    std::array<uint32_t, 3> indices = mesh.TriangleIndices(tri_idx);
    // Skip the triangle if none of its vertices need recomputation.
    if (!vertex_mask.empty() && !vertex_mask[indices[0]] &&
        !vertex_mask[indices[1]] && !vertex_mask[indices[2]]) {
      continue;
    }
    Triangle triangle = mesh.GetTriangle(tri_idx);
    float area = triangle.SignedArea();
    if (area == 0.0f) continue;

    std::array<SideCategory, 3> side_cat;
    std::array<ForwardCategory, 3> fwd_cat;
    for (int i = 0; i < 3; ++i) {
      side_cat[i] = GetSideLabel(mesh, attrs, indices[i]).DecodeSideCategory();
      fwd_cat[i] = GetFwdLabel(mesh, attrs, indices[i]).DecodeForwardCategory();
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
      if (side_cat[i] == kExteriorLeft) grad_uf_left -= grad_ulambda[i];
      if (side_cat[i] == kExteriorRight) grad_uf_right -= grad_ulambda[i];
      if (fwd_cat[i] == kExteriorFront) grad_uf_front -= grad_ulambda[i];
      if (fwd_cat[i] == kExteriorBack) grad_uf_back -= grad_ulambda[i];
    }

    float two_area = 2.0f * area;
    auto rescale = [two_area](Vec grad) -> Vec {
      float mag_sq = grad.MagnitudeSquared();
      return (mag_sq > two_area * kFloatTolerance)
                 ? ((two_area / mag_sq) * grad)
                 : Vec{0, 0};
    };
    Vec deriv_left = rescale(grad_uf_left);
    Vec deriv_right = rescale(grad_uf_right);
    Vec deriv_front = rescale(grad_uf_front);
    Vec deriv_back = rescale(grad_uf_back);

    // Add the derivatives onto the vertices.
    for (int i = 0; i < 3; ++i) {
      uint32_t v = indices[i];
      if (side_cat[i] != kExteriorLeft) side_derivative[v].Add(-deriv_right);
      if (side_cat[i] != kExteriorRight) side_derivative[v].Add(deriv_left);
      if (fwd_cat[i] != kExteriorFront) forward_derivative[v].Add(-deriv_back);
      if (fwd_cat[i] != kExteriorBack) forward_derivative[v].Add(deriv_front);
    }
  }

  for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
    if (!vertex_mask.empty() && !vertex_mask[i]) continue;
    SetDerivatives(mesh, attrs, i, side_derivative[i].Value(),
                   forward_derivative[i].Value());
  }

  // Computing margins.
  // Recall that margins are used during rendering to constrain anti-aliasing
  // vertex outsets and prevent self-overlap and triangle inversion. See the
  // comments in `StrokeVertex::Label` and the implementation of
  // `DerivativeCalculator::ComputeTriangleMarginUpperBounds` for details.
  //
  // We iterate through all triangles in the mesh; each triangle imposes an
  // upper bound margin on its vertices. We compute these per-triangle
  // constraints, and assign each vertex the tightest (minimum) bound across its
  // incident triangles, and then write the margins to the mesh.
  //
  // Only vertices with `vertex_mask` set have margins recomputed; triangles
  // with no such vertex are skipped. To handle other vertices that are adjacent
  // to recomputed vertices (that may have had their derivatives changed), we
  // take a simplified (conservative) approach of only lowering the margin.
  // TODO(b/521448869): Consider computing the margin exactly for vertices
  // adjacent to recomputed vertices.
  std::vector<float> side_margins(mesh.VertexCount(), kMaximumMargin);
  std::vector<float> forward_margins(mesh.VertexCount(), kMaximumMargin);

  for (uint32_t tri_idx = 0; tri_idx < mesh.TriangleCount(); ++tri_idx) {
    std::array<uint32_t, 3> indices = mesh.TriangleIndices(tri_idx);
    // Skip the triangle if none of its vertices need recomputation.
    if (!vertex_mask.empty() && !vertex_mask[indices[0]] &&
        !vertex_mask[indices[1]] && !vertex_mask[indices[2]]) {
      continue;
    }
    Triangle triangle = mesh.GetTriangle(tri_idx);

    std::array<Vec, 3> side_outsets, forward_outsets;
    for (int i = 0; i < 3; ++i) {
      uint32_t v = indices[i];
      side_outsets[i] = GetSideLabel(mesh, attrs, v).DerivativeOutsetSign() *
                        GetSideDerivative(mesh, attrs, v);
      forward_outsets[i] = GetFwdLabel(mesh, attrs, v).DerivativeOutsetSign() *
                           GetForwardDerivative(mesh, attrs, v);
    }

    std::array<float, 3> side_bounds =
        DerivativeCalculator::ComputeTriangleMarginUpperBounds(triangle,
                                                               side_outsets);
    std::array<float, 3> forward_bounds =
        DerivativeCalculator::ComputeTriangleMarginUpperBounds(triangle,
                                                               forward_outsets);

    for (int i = 0; i < 3; ++i) {
      side_margins[indices[i]] =
          std::min(side_margins[indices[i]], side_bounds[i]);
      forward_margins[indices[i]] =
          std::min(forward_margins[indices[i]], forward_bounds[i]);
    }
  }

  for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
    if (vertex_mask.empty() || vertex_mask[i]) {
      SetMargins(mesh, attrs, i, side_margins[i], forward_margins[i]);
      continue;
    }
    // For other vertices, we only decrease the margin.
    auto [side, forward] = GetMargins(mesh, attrs, i);
    if (side_margins[i] < side || forward_margins[i] < forward) {
      SetMargins(mesh, attrs, i, std::min(side, side_margins[i]),
                 std::min(forward, forward_margins[i]));
    }
  }
}
// LINT.ThenChange(
//     //depot/google3/third_party/ink/strokes/internal/brush_tip_extruder/derivative_calculator.cc,
//     //depot/google3/third_party/ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:calculate_antialiasing_and_position_outset,
//     //depot/google3/third_party/ink/rendering/webgpu/StrokeShader.wgsl:calculate_antialiasing_and_position_outset)

}  // namespace ink::strokes_internal
