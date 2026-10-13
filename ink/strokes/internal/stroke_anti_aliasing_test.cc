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
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/internal/test_matchers.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::IsCyclicPermutationOf;
using ::testing::ElementsAre;
using ::testing::FloatNear;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

using AttributeId = MeshFormat::AttributeId;
using AttributeType = MeshFormat::AttributeType;
using IndexFormat = MeshFormat::IndexFormat;

constexpr float kInteriorLabel = 0.0f;
constexpr float kLeftLabel = -127.0f;
constexpr float kRightLabel = 127.0f;
constexpr float kFrontLabel = -127.0f;
constexpr float kBackLabel = 127.0f;

constexpr uint32_t kSideDerivativeAttr = 1;
constexpr uint32_t kSideLabelAttr = 2;
constexpr uint32_t kForwardDerivativeAttr = 3;
constexpr uint32_t kForwardLabelAttr = 4;

MeshFormat AntiAliasingMeshFormat() {
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ABSL_CHECK_OK(format);
  return *format;
}

void SetVertexLabels(MutableMesh& mesh, uint32_t v, float side_label,
                     float fwd_label) {
  mesh.SetFloatVertexAttribute(v, kSideLabelAttr, {side_label});
  mesh.SetFloatVertexAttribute(v, kForwardLabelAttr, {fwd_label});
}

void SetVertexDerivatives(MutableMesh& mesh, uint32_t v, Vec side_derivative,
                          Vec fwd_derivative) {
  mesh.SetFloatVertexAttribute(v, kSideDerivativeAttr,
                               {side_derivative.x, side_derivative.y});
  mesh.SetFloatVertexAttribute(v, kForwardDerivativeAttr,
                               {fwd_derivative.x, fwd_derivative.y});
}

// Returns the index of a vertex in `mesh` that is near the point `p` if exists.
std::optional<uint32_t> FindVertexIndex(const MutableMesh& mesh, Point p) {
  for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
    if (std::abs(mesh.VertexPosition(i).x - p.x) < 1e-4f &&
        std::abs(mesh.VertexPosition(i).y - p.y) < 1e-4f) {
      return i;
    }
  }
  return std::nullopt;
}

// Asserts that a vertex near `p` exists in `mesh` and has the expected side
// and forward labels.
void CheckVertexLabels(const MutableMesh& mesh, Point p, float expected_side,
                       float expected_fwd) {
  std::optional<uint32_t> idx = FindVertexIndex(mesh, p);
  ASSERT_TRUE(idx.has_value());
  EXPECT_EQ(
      StrokeVertex::Label{mesh.FloatVertexAttribute(*idx, kSideLabelAttr)[0]}
          .DecodeSideCategory(),
      StrokeVertex::Label{expected_side}.DecodeSideCategory());
  EXPECT_EQ(
      StrokeVertex::Label{mesh.FloatVertexAttribute(*idx, kForwardLabelAttr)[0]}
          .DecodeForwardCategory(),
      StrokeVertex::Label{expected_fwd}.DecodeForwardCategory());
}

// Asserts that a vertex near `p` exists in `mesh` and has the expected side
// and forward derivatives.
void CheckVertexDerivatives(const MutableMesh& mesh, Point p, Vec expected_side,
                            Vec expected_fwd) {
  std::optional<uint32_t> idx = FindVertexIndex(mesh, p);
  ASSERT_TRUE(idx.has_value());
  auto side = mesh.FloatVertexAttribute(*idx, kSideDerivativeAttr);
  EXPECT_NEAR(side[0], expected_side.x, 1e-3);
  EXPECT_NEAR(side[1], expected_side.y, 1e-3);
  auto fwd = mesh.FloatVertexAttribute(*idx, kForwardDerivativeAttr);
  EXPECT_NEAR(fwd[0], expected_fwd.x, 1e-3);
  EXPECT_NEAR(fwd[1], expected_fwd.y, 1e-3);
}

// Asserts that a vertex near `p` exists in `mesh` and has the expected decoded
// side and forward margins.
void CheckVertexMargins(const MutableMesh& mesh, Point p,
                        float expected_side_margin, float expected_fwd_margin) {
  std::optional<uint32_t> idx = FindVertexIndex(mesh, p);
  ASSERT_TRUE(idx.has_value());
  float side_label_val = mesh.FloatVertexAttribute(*idx, kSideLabelAttr)[0];
  EXPECT_THAT(StrokeVertex::Label{side_label_val}.DecodeMargin(),
              FloatNear(expected_side_margin, 1e-3f));
  float fwd_label_val = mesh.FloatVertexAttribute(*idx, kForwardLabelAttr)[0];
  EXPECT_THAT(StrokeVertex::Label{fwd_label_val}.DecodeMargin(),
              FloatNear(expected_fwd_margin, 1e-3f));
}

// Verifies that no interior edge (shared by multiple triangles) in `mesh` is
// mislabeled with a boundary label.
void ExpectNoMislabeledInteriorEdges(const MutableMesh& mesh) {
  absl::flat_hash_map<std::pair<uint32_t, uint32_t>, int> edge_tri_count;
  for (uint32_t t = 0; t < mesh.TriangleCount(); ++t) {
    std::array<uint32_t, 3> tri = mesh.TriangleIndices(t);
    for (int i = 0; i < 3; ++i) {
      edge_tri_count[std::minmax(tri[i], tri[(i + 1) % 3])]++;
    }
  }

  for (const auto& [edge, count] : edge_tri_count) {
    if (count < 2) continue;  // Boundary edge; skip.

    auto [u, v] = edge;
    float side_u = mesh.FloatVertexAttribute(u, kSideLabelAttr)[0];
    float side_v = mesh.FloatVertexAttribute(v, kSideLabelAttr)[0];
    float fwd_u = mesh.FloatVertexAttribute(u, kForwardLabelAttr)[0];
    float fwd_v = mesh.FloatVertexAttribute(v, kForwardLabelAttr)[0];

    bool same_side = (side_u > 0 && side_v > 0) || (side_u < 0 && side_v < 0);
    bool same_fwd = (fwd_u > 0 && fwd_v > 0) || (fwd_u < 0 && fwd_v < 0);

    EXPECT_FALSE(same_side || same_fwd);
  }
}

TEST(StrokeAntiAliasingTest, EmptyMesh) {
  Point A{0, 0}, B{1, 0}, C{0, 1};
  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C}) mesh.AppendVertex(p);
  EXPECT_THAT(ComputeOutlinesAndLabels(mesh), IsEmpty());
}

TEST(StrokeAntiAliasingTest, WithoutAntiAliasingAttributes) {
  // C
  // | \
  // |  \
  // |   \
  // |    \
  // |     \
  // |      \
  // |       \
  // |        \
  // |         \
  // A----------B
  Point A{0, 0}, B{10, 0}, C{0, 10};
  MutableMesh mesh(MeshFormat{});
  for (const Point& p : {A, B, C}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({0, 1, 2});

  EXPECT_THAT(
      ComputeOutlinesAndLabels(mesh),
      ElementsAre(IsCyclicPermutationOf(std::vector<uint32_t>{0, 2, 1})));
  ComputeDerivativesAndMargins(mesh);
  EXPECT_EQ(mesh.VertexCount(), 3);
}

TEST(StrokeAntiAliasingTest, PartiallyComputeLabels1) {
  // This test checks partial label computation, imagining that the input mesh
  // arises from partially subtracting a stroke. Vertices A, E are carried over
  // from the original mesh (preserving their labels and derivatives), while
  // vertices B, C, D are newly created by the subtraction (with interpolated
  // derivatives).
  //
  // Note that there is no single canonically "correct" labeling for boundary
  // vertices. This test verifies that the heuristic alignment cost optimization
  // approach behaves as intended.
  //
  // E---------D         stroke travel direction
  // | \       |                   ^
  // |   \     |                   |
  // |     \   |                   +---> right
  // |       \ |
  // |         C
  // |       / |
  // |     /   |
  // |   /     |
  // | /       |
  // A---------B
  Point A{0, 0}, B{5, 0}, C{5, 5}, D{5, 10}, E{0, 10};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D, E}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({0, 2, 4});
  mesh.AppendTriangleIndices({4, 2, 3});

  // A is left front, E is left back.
  SetVertexLabels(mesh, 0, kLeftLabel, kFrontLabel);
  SetVertexLabels(mesh, 4, kLeftLabel, kBackLabel);

  // Set derivatives to define the stroke's local coordinate frame.
  for (uint32_t i = 0; i < 5; ++i) {
    SetVertexDerivatives(mesh, i, {10.0f, 0.0f}, {0.0f, 10.0f});
  }

  EXPECT_THAT(
      ComputeOutlinesAndLabels(mesh, {false, true, true, true, false}),
      ElementsAre(IsCyclicPermutationOf(std::vector<uint32_t>{0, 4, 3, 2, 1})));

  CheckVertexLabels(mesh, A, kLeftLabel, kFrontLabel);
  CheckVertexLabels(mesh, E, kLeftLabel, kBackLabel);
  CheckVertexLabels(mesh, D, kRightLabel, kBackLabel);
  CheckVertexLabels(mesh, C, kRightLabel, kInteriorLabel);
  CheckVertexLabels(mesh, B, kRightLabel, kFrontLabel);

  ExpectNoMislabeledInteriorEdges(mesh);
}

TEST(StrokeAntiAliasingTest, PartiallyComputeLabels2) {
  // This test checks partial label computation, imagining that the input mesh
  // arises from partially subtracting a stroke. Vertices A, B, C, D are carried
  // over from the original mesh (preserving their labels), while vertices E
  // through K are newly created by the subtraction (with interpolated
  // derivatives).
  //
  // Note that there is no single canonically "correct" labeling for boundary
  // vertices. This test verifies that the heuristic alignment cost optimization
  // approach behaves as intended.
  //
  // D-----------------------------C
  // |                           / |
  // |                         /   |
  // |                       /     |
  // |                     /       |
  // |                   /         |
  // |                 /           |
  // |               /             |          +--> stroke travel direction
  // |             /               |          |
  // |           /                 |          V
  // |         /                   |          right
  // |       /    G--------H       |
  // |     /       \       |       |
  // |   /           F     I---J   |
  // | /            /          |   |
  // A-------------E           K---B
  Point A{0, 0}, B{10, 0}, C{10, 15}, D{0, 15};
  Point E{6, 0}, F{7, 2}, G{5, 3}, H{8, 3}, I{8, 2}, J{9, 2}, K{9, 0};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D, E, F, G, H, I, J, K}) {
    mesh.AppendVertex(p);
  }
  mesh.AppendTriangleIndices({0, 2, 3});   // ACD
  mesh.AppendTriangleIndices({0, 4, 5});   // AEF
  mesh.AppendTriangleIndices({0, 5, 6});   // AFG
  mesh.AppendTriangleIndices({0, 6, 2});   // AGC
  mesh.AppendTriangleIndices({2, 6, 7});   // CGH
  mesh.AppendTriangleIndices({2, 7, 8});   // CHI
  mesh.AppendTriangleIndices({2, 8, 9});   // CIJ
  mesh.AppendTriangleIndices({2, 9, 1});   // CJB
  mesh.AppendTriangleIndices({9, 10, 1});  // JKB

  // Set the labels: traveling along the stroke from start to end,
  // AD is front, CD is left, AB is right, BC is back.
  SetVertexLabels(mesh, 0, kRightLabel, kFrontLabel);
  SetVertexLabels(mesh, 1, kRightLabel, kBackLabel);
  SetVertexLabels(mesh, 2, kLeftLabel, kBackLabel);
  SetVertexLabels(mesh, 3, kLeftLabel, kFrontLabel);

  // Set derivatives to define the stroke's local coordinate frame.
  for (uint32_t i = 0; i < 11; ++i) {
    SetVertexDerivatives(mesh, i, {0.0f, -15.0f}, {10.0f, 0.0f});
  }

  EXPECT_THAT(
      ComputeOutlinesAndLabels(mesh, {false, false, false, false, true, true,
                                      true, true, true, true, true}),
      ElementsAre(IsCyclicPermutationOf(
          std::vector<uint32_t>{0, 3, 2, 1, 10, 9, 8, 7, 6, 5, 4})));

  // Subdivision is required to ensure that chord edges are not labeled as
  // boundaries.
  EXPECT_GT(mesh.TriangleCount(), 9);
  EXPECT_GT(mesh.VertexCount(), 11);

  // Labeling the vertices is a bit of a puzzle. It may be useful to first note
  // the orientation of the edges (with respect to the orientation of the mesh):
  // (A,E) faces right, (E,F) faces equally back and right, (F,G) faces mostly
  // left and slightly back, (G,H) right, (H,I) front, (I,J) right, (J,K)
  // front, and (K,B) right.
  //
  //              G--------H                  +--> stroke travel direction
  //               \       |                  |
  //                 F     I---J              V
  //                /          |             right
  // A-------------E           K---B
  //
  // We can roughly reason through as follows:

  CheckVertexLabels(mesh, A, kRightLabel, kFrontLabel);
  // edge (A,E) should definitely be kRight, since (A,B) was kRight.
  CheckVertexLabels(mesh, E, kRightLabel, kBackLabel);
  // edge (E,F) could reasonably be kRight or kBack, but choosing kRight
  // would demand (F,G) also be kRight (assuming that (G,H) is kRight).
  // So kBack is chosen.
  CheckVertexLabels(mesh, F, kInteriorLabel, kBackLabel);
  // (F,G) is geometrically oriented left, but can not be labeled as such
  // (assuming again that we'll choose (G,H) to be kRight). Given this,
  // kBack is the next best choice.
  CheckVertexLabels(mesh, G, kRightLabel, kBackLabel);
  // (G,H) seems pretty clearly kRight.
  CheckVertexLabels(mesh, H, kRightLabel, kFrontLabel);
  // (H,I) seems kFront, but could also be kRight. Choosing kRight here would
  // allow (I,J) to be labeled kRight, but then also force (J,K) to be kRight,
  // effectively mislabeling two edges. So indeed, kFront.
  CheckVertexLabels(mesh, I, kInteriorLabel, kFrontLabel);
  // (I,J) would like to be kRight, but that's incompatible with (J,K) being
  // set to kFront. Choosing (I,J) to be kFront is prioritized here because
  // (J,K) is larger.
  CheckVertexLabels(mesh, J, kInteriorLabel, kFrontLabel);
  CheckVertexLabels(mesh, K, kRightLabel, kFrontLabel);
  // (K,B) should be kRight, since (A,B) was kRight.
  CheckVertexLabels(mesh, B, kRightLabel, kBackLabel);

  // Check that subdivision worked.
  ExpectNoMislabeledInteriorEdges(mesh);
}

TEST(StrokeAntiAliasingTest, PartiallyComputeLabels3) {
  // This test checks partial label computation when an interior hole is punched
  // in a stroke. Outer vertices A, B, C, D are carried over from the original
  // mesh (preserving their labels), while hole vertices E, F, G, H are newly
  // created by the subtraction (with interpolated derivatives).
  //
  //  D------------------------------------C
  //  |                                    |
  //  |                                    |
  //  |           H----------G             |
  //  |           |          |             |
  //  |           |          |             |
  //  |           |          |             |   stroke travel direction
  //  |           |          |             |              ^
  //  |           E----------F             |              |
  //  |                                    |              +---> right
  //  |                                    |
  //  A------------------------------------B
  Point A{0, 0}, B{30, 0}, C{30, 30}, D{0, 30};
  Point E{10, 10}, F{20, 10}, G{20, 20}, H{10, 20};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D, E, F, G, H}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({0, 1, 5});  // ABF
  mesh.AppendTriangleIndices({0, 5, 4});  // AFE
  mesh.AppendTriangleIndices({1, 2, 6});  // BCG
  mesh.AppendTriangleIndices({1, 6, 5});  // BGF
  mesh.AppendTriangleIndices({2, 3, 7});  // CDH
  mesh.AppendTriangleIndices({2, 7, 6});  // CHG
  mesh.AppendTriangleIndices({3, 0, 4});  // DAE
  mesh.AppendTriangleIndices({3, 4, 7});  // DEH

  SetVertexLabels(mesh, 0, kLeftLabel, kFrontLabel);
  SetVertexLabels(mesh, 1, kRightLabel, kFrontLabel);
  SetVertexLabels(mesh, 2, kRightLabel, kBackLabel);
  SetVertexLabels(mesh, 3, kLeftLabel, kBackLabel);

  // Set derivatives to define the stroke's local coordinate frame.
  for (uint32_t i = 0; i < 8; ++i) {
    SetVertexDerivatives(mesh, i, {30.0f, 0.0f}, {0.0f, 30.0f});
  }

  EXPECT_THAT(ComputeOutlinesAndLabels(
                  mesh, {false, false, false, false, true, true, true, true}),
              UnorderedElementsAre(
                  IsCyclicPermutationOf(std::vector<uint32_t>{0, 3, 2, 1}),
                  IsCyclicPermutationOf(std::vector<uint32_t>{4, 5, 6, 7})));

  // Check outer boundary vertices preserved:
  CheckVertexLabels(mesh, A, kLeftLabel, kFrontLabel);
  CheckVertexLabels(mesh, B, kRightLabel, kFrontLabel);
  CheckVertexLabels(mesh, C, kRightLabel, kBackLabel);
  CheckVertexLabels(mesh, D, kLeftLabel, kBackLabel);

  // Check hole boundary vertices:
  CheckVertexLabels(mesh, E, kRightLabel, kBackLabel);
  CheckVertexLabels(mesh, F, kLeftLabel, kBackLabel);
  CheckVertexLabels(mesh, G, kLeftLabel, kFrontLabel);
  CheckVertexLabels(mesh, H, kRightLabel, kFrontLabel);

  ExpectNoMislabeledInteriorEdges(mesh);
}

TEST(StrokeAntiAliasingTest, ComputeLabels) {
  // This test checks computing all labels from scratch when no pre-existing
  // labels or derivatives are set on the mesh - in which case, the labeling
  // algorithm defaults to a global stroke direction of up.
  //
  //  D---------------------C
  //  |                   / |
  //  |                 /   |
  //  |               /     |   default travel direction
  //  |             /       |              ^
  //  |           /         |              |
  //  |         /           |              +---> right
  //  |       /             |
  //  |     /               |
  //  |   /                 |
  //  | /                   |
  //  A---------------------B
  Point A{10, 10}, B{20, 10}, C{20, 20}, D{10, 20};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({0, 2, 3});

  EXPECT_THAT(
      ComputeOutlinesAndLabels(mesh),
      ElementsAre(IsCyclicPermutationOf(std::vector<uint32_t>{0, 3, 2, 1})));

  CheckVertexLabels(mesh, A, kLeftLabel, kFrontLabel);
  CheckVertexLabels(mesh, B, kRightLabel, kFrontLabel);
  CheckVertexLabels(mesh, C, kRightLabel, kBackLabel);
  CheckVertexLabels(mesh, D, kLeftLabel, kBackLabel);

  ExpectNoMislabeledInteriorEdges(mesh);
}

TEST(StrokeAntiAliasingTest, PartiallyComputeDerivatives) {
  // This test checks partial derivative computation, imagining that triangle
  // ABC was modified by a subtraction (marking vertices A, B, C for
  // recomputation) while triangle ACD was untouched (leaving vertex D
  // unmasked). Vertices A, B, C have their derivatives recomputed while vertex
  // D preserves its original derivatives.
  //
  //  D---------------------C
  //  |                   / |
  //  |                 /   |
  //  |               /     |   stroke travel direction
  //  |             /       |              ^
  //  |           /         |              |
  //  |         /           |              +---> right
  //  |       /             |
  //  |     /               |
  //  |   /                 |
  //  | /                   |
  //  A---------------------B
  Point A{0, 0}, B{5, 0}, C{5, 10}, D{0, 10};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({0, 1, 2});  // ABC
  mesh.AppendTriangleIndices({0, 2, 3});  // ACD

  SetVertexLabels(mesh, 0, kLeftLabel, kFrontLabel);
  SetVertexLabels(mesh, 1, kRightLabel, kFrontLabel);
  SetVertexLabels(mesh, 2, kRightLabel, kBackLabel);
  SetVertexLabels(mesh, 3, kLeftLabel, kBackLabel);

  // Initialize the mesh with nonsense derivatives.
  for (uint32_t i = 0; i < 4; ++i) {
    SetVertexDerivatives(mesh, i, {99.0f, 99.0f}, {99.0f, 99.0f});
  }

  ComputeDerivativesAndMargins(mesh, {true, true, true, false});

  // A, B, and C are updated to match the stroke width (5) and length (10);
  // D is masked out and keeps its original derivatives.
  CheckVertexDerivatives(mesh, A, {5.0f, 0.0f}, {0.0f, 10.0f});
  CheckVertexDerivatives(mesh, B, {5.0f, 0.0f}, {0.0f, 10.0f});
  CheckVertexDerivatives(mesh, C, {5.0f, 0.0f}, {0.0f, 10.0f});
  CheckVertexDerivatives(mesh, D, {99.0f, 99.0f}, {99.0f, 99.0f});
}

TEST(StrokeAntiAliasingTest, ComputeMargins) {
  //    D---------------C
  //    |             //
  //    |          /  /
  //    |       /    /
  //    |    /      /
  //    | /        /                  stroke travel direction
  //    E         F                                ^
  //    | \        \                               |
  //    |    \      \                              +---> right
  //    |       \    \
  //    |          \  \
  //    |             \\
  //    A---------------B
  Point A{0, -2}, B{10, -2}, C{10, 2}, D{0, 2}, E{0, 0}, F{2, 0};

  MutableMesh mesh(AntiAliasingMeshFormat());
  for (const Point& p : {A, B, C, D, E, F}) mesh.AppendVertex(p);
  mesh.AppendTriangleIndices({4, 0, 1});  // EAB
  mesh.AppendTriangleIndices({4, 1, 5});  // EBF
  mesh.AppendTriangleIndices({4, 5, 2});  // EFC
  mesh.AppendTriangleIndices({4, 2, 3});  // ECD

  SetVertexLabels(mesh, 0, kLeftLabel, kFrontLabel);
  SetVertexLabels(mesh, 1, kRightLabel, kFrontLabel);
  SetVertexLabels(mesh, 2, kRightLabel, kBackLabel);
  SetVertexLabels(mesh, 3, kLeftLabel, kBackLabel);
  SetVertexLabels(mesh, 4, kLeftLabel, kInteriorLabel);
  SetVertexLabels(mesh, 5, kRightLabel, kInteriorLabel);

  ComputeDerivativesAndMargins(mesh);

  // Given the triangulation and the labeling, the derivatives can, with a
  // little effort, be computed.
  CheckVertexDerivatives(mesh, B, {4.132f, 3.226f}, {0.0f, 2.0f});
  CheckVertexDerivatives(mesh, C, {4.132f, -3.226f}, {0.0f, 2.0f});
  CheckVertexDerivatives(mesh, F, {0.485f, 0.0f}, {0.0f, 2.0f});

  // The margins can also be computed, although this time requiring considerable
  // effort and close attention to the details of
  // `ComputeTriangleMarginUpperBounds`.
  // As a check, since the derivatives of B and C point into the concavity, we
  // expect them to be constrained.
  CheckVertexMargins(mesh, B, 0.349f, StrokeVertex::kMaximumMargin);
  CheckVertexMargins(mesh, C, 0.349f, StrokeVertex::kMaximumMargin);

  // On the other hand F, at the center of the concavity, is unconstrained.
  CheckVertexMargins(mesh, F, StrokeVertex::kMaximumMargin, 0.0f);
}

}  // namespace
}  // namespace ink::strokes_internal
