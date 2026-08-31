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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/fuzz_domains.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/internal/test_matchers.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_test_helpers.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/vec.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::ink::geometry_internal::IsCyclicPermutationOf;
using ::testing::ElementsAre;
using ::testing::FloatEq;
using ::testing::FloatNear;

using AttributeId = MeshFormat::AttributeId;
using AttributeType = MeshFormat::AttributeType;
using IndexFormat = MeshFormat::IndexFormat;

std::vector<Point> GetOutlinePoints(const PartitionedMesh& partitioned_mesh,
                                    uint32_t group_index,
                                    uint32_t outline_index) {
  uint32_t count =
      partitioned_mesh.OutlineVertexCount(group_index, outline_index);
  std::vector<Point> outline_points(count);
  for (uint32_t i = 0; i < count; ++i) {
    outline_points[i] =
        partitioned_mesh.OutlinePosition(group_index, outline_index, i);
  }
  return outline_points;
}

constexpr float kFloatTolerance = 1e-6f;

constexpr float kInteriorLabel = 0.0f;
constexpr float kLeftLabel = -127.0f;
constexpr float kRightLabel = 127.0f;
constexpr float kFrontLabel = -127.0f;
constexpr float kBackLabel = 127.0f;

uint32_t NumVertices(const PartitionedMesh& partitioned_mesh) {
  uint32_t count = 0;
  for (uint32_t g = 0; g < partitioned_mesh.RenderGroupCount(); ++g) {
    for (const auto& m : partitioned_mesh.RenderGroupMeshes(g)) {
      count += m.VertexCount();
    }
  }
  return count;
}

uint32_t NumTriangles(const PartitionedMesh& partitioned_mesh) {
  uint32_t count = 0;
  for (uint32_t g = 0; g < partitioned_mesh.RenderGroupCount(); ++g) {
    for (const auto& m : partitioned_mesh.RenderGroupMeshes(g)) {
      count += m.TriangleCount();
    }
  }
  return count;
}

// Returns the index of a vertex in `mesh` that is near the point `p` if exists.
std::optional<uint32_t> FindVertexIndex(const Mesh& mesh, Point p) {
  for (uint32_t i = 0; i < mesh.VertexCount(); ++i) {
    if (std::abs(mesh.VertexPosition(i).x - p.x) < 1e-4f &&
        std::abs(mesh.VertexPosition(i).y - p.y) < 1e-4f) {
      return i;
    }
  }
  return std::nullopt;
}

// Asserts that a vertex near `p` exists in `mesh` and has the expected side
// and forward labels (attributes 2 and 4).
void CheckVertexLabels(const Mesh& mesh, Point p, float expected_side,
                       float expected_fwd) {
  std::optional<uint32_t> idx = FindVertexIndex(mesh, p);
  ASSERT_TRUE(idx.has_value());
  EXPECT_FLOAT_EQ(mesh.FloatVertexAttribute(*idx, 2)[0], expected_side);
  EXPECT_FLOAT_EQ(mesh.FloatVertexAttribute(*idx, 4)[0], expected_fwd);
}

// Returns the value of the given attribute at point `p`, by finding the
// triangle in `pm` containing `p` and interpolating its vertex attributes.
// Assumes that there are no overlapping triangles.
std::optional<std::vector<float>> AttributeValue(const PartitionedMesh& pm,
                                                 Point p, uint32_t attr_idx) {
  if (pm.RenderGroupCount() == 0) return std::nullopt;
  for (const Mesh& mesh : pm.RenderGroupMeshes(0)) {
    for (uint32_t i = 0; i < mesh.TriangleCount(); ++i) {
      Triangle tri = mesh.GetTriangle(i);
      if (!tri.Contains(p)) continue;
      if (auto coords =
              ink::geometry_internal::GetBarycentricCoordinates(tri, p)) {
        auto idx = mesh.TriangleIndices(i);
        auto v0 = mesh.FloatVertexAttribute(idx[0], attr_idx);
        auto v1 = mesh.FloatVertexAttribute(idx[1], attr_idx);
        auto v2 = mesh.FloatVertexAttribute(idx[2], attr_idx);
        std::vector<float> result(v0.Size());
        for (size_t c = 0; c < v0.Size(); ++c) {
          result[c] = (*coords)[0] * v0[c] + (*coords)[1] * v1[c] +
                      (*coords)[2] * v2[c];
        }
        return result;
      }
    }
  }
  return std::nullopt;
}

TEST(StrokeSubtractionTest, Disjoint) {
  // C                                F
  // | \                              | \
  // |  \                             |  \
  // |   \                            |   \
  // |    \                           |    \
  // |     \                          |     \
  // |      \                         |      \
  // |       \                        |       \
  // |        \                       |        \
  // | mesh_a  \                      | mesh_b  \
  // A----------B                     D----------E
  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{100, 0}, E{110, 0}, F{100, 10};

  MutableMesh mesh_a(MeshFormat{});
  mesh_a.AppendVertex(A);
  mesh_a.AppendVertex(B);
  mesh_a.AppendVertex(C);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  std::vector<uint32_t> mesh_a_outline = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  MutableMesh mesh_b(MeshFormat{});
  mesh_b.AppendVertex(D);
  mesh_b.AppendVertex(E);
  mesh_b.AppendVertex(F);
  mesh_b.AppendTriangleIndices({0, 1, 2});

  std::vector<uint32_t> mesh_b_outline = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 1);
  EXPECT_EQ(NumVertices(*result), 3);
}

TEST(StrokeSubtractionTest, TriangleMinusTriangle) {
  //  C       F
  // | \      | \
  // |  \     |  \
  // |    \   |   \
  // |     \  |    \
  // |      \ |     \
  // |       \|      \
  // |        \       \
  // |        |\       \
  // | mesh_a | \       \
  // A------------B      \
  //          |  mesh_b   \
  //          D------------E

  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point X1{5, 0};  // Intersection of AB and DF
  Point X2{5, 5};  // Intersection of BC and DF

  // Set up mesh_a
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat1Unpacked, AttributeId::kCustom0},
       {AttributeType::kFloat4Unpacked, AttributeId::kCustom1}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  mesh_a.AppendVertex(A);
  mesh_a.AppendVertex(B);
  mesh_a.AppendVertex(C);
  mesh_a.AppendTriangleIndices({0, 1, 2});

  mesh_a.SetFloatVertexAttribute(0, 1, {0.0f});
  mesh_a.SetFloatVertexAttribute(1, 1, {10.0f});
  mesh_a.SetFloatVertexAttribute(2, 1, {20.0f});

  mesh_a.SetFloatVertexAttribute(0, 2, {1.0f, 0.0f, 0.0f, 1.0f});
  mesh_a.SetFloatVertexAttribute(1, 2, {0.0f, 1.0f, 0.0f, 1.0f});
  mesh_a.SetFloatVertexAttribute(2, 2, {0.0f, 0.0f, 1.0f, 1.0f});

  std::vector<uint32_t> mesh_a_outline = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  mesh_b.AppendVertex({0, 0});
  mesh_b.AppendVertex({20, 0});
  mesh_b.AppendVertex({0, 20});
  mesh_b.AppendTriangleIndices({0, 1, 2});
  AffineTransform mesh_b_transform = AffineTransform::Translate({5.0f, -5.0f});

  std::vector<uint32_t> mesh_b_outline = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               mesh_b_transform, 0.1f);
  ASSERT_THAT(result, IsOk());

  // The result should have 2 triangles and 4 vertices.
  EXPECT_EQ(NumTriangles(*result), 2);
  EXPECT_EQ(NumVertices(*result), 4);

  // Check the outline of the result.
  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points = GetOutlinePoints(*result, 0, 0);
  EXPECT_THAT(outline_points,
              IsCyclicPermutationOf(std::vector<Point>{A, C, X2, X1}));

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  // Check the vertex attributes.

  // Vertex A and C should have been copied from mesh_a.
  std::optional<uint32_t> index_A = FindVertexIndex(result_mesh, A);
  ASSERT_TRUE(index_A.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_A, 1).Values()[0],
                  0.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_A, 2).Values(),
      ElementsAre(FloatEq(1.0f), FloatEq(0.0f), FloatEq(0.0f), FloatEq(1.0f)));

  std::optional<uint32_t> index_C = FindVertexIndex(result_mesh, C);
  ASSERT_TRUE(index_C.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_C, 1).Values()[0],
                  20.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_C, 2).Values(),
      ElementsAre(FloatEq(0.0f), FloatEq(0.0f), FloatEq(1.0f), FloatEq(1.0f)));

  // X1 and X2 are newly formed intersection points. The custom attributes
  // should be linearly interpolated.
  std::optional<uint32_t> index_X1 = FindVertexIndex(result_mesh, X1);
  ASSERT_TRUE(index_X1.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_X1, 1).Values()[0],
                  5.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_X1, 2).Values(),
      ElementsAre(FloatEq(0.5f), FloatEq(0.5f), FloatEq(0.0f), FloatEq(1.0f)));

  std::optional<uint32_t> index_X2 = FindVertexIndex(result_mesh, X2);
  ASSERT_TRUE(index_X2.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_X2, 1).Values()[0],
                  15.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_X2, 2).Values(),
      ElementsAre(FloatEq(0.0f), FloatEq(0.5f), FloatEq(0.5f), FloatEq(1.0f)));
}

TEST(StrokeSubtractionTest, ComputeLabels1) {
  // Note that there is no single canonically "correct" labeling for boundary
  // vertices. This test verifies that the heuristic alignment cost optimization
  // approach implemented by StrokeSubtraction behaves as intended.

  //           H-------------------G
  //           |      mesh_b       |
  // D---------+---------C         |    stroke travel direction
  // |         |       / |         |           ^
  // |         |     /   |         |           |
  // | mesh_a  |   /     |         |           +---> right
  // |         | /       |         |
  // |         |         |         |
  // |       / |         |         |
  // |     /   |         |         |
  // |   /     |         |         |
  // | /       |         |         |
  // A---------+---------B         |
  //           |                   |
  //           E-------------------F
  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};
  Point E{5, -5}, F{15, -5}, G{15, 15}, H{5, 15};
  Point X1{5, 0};   // intersection of AB and EH
  Point X2{5, 5};   // intersection of AC and EH
  Point X3{5, 10};  // intersection of CD and EH

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  // Set up mesh_a
  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  // Set the labels: traveling along the stroke from start to end,
  // AB is front, AD is left, BC is right, CD is back.

  // A is left front
  mesh_a.SetFloatVertexAttribute(0, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  // B is right front
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kFrontLabel});

  // C is right back
  mesh_a.SetFloatVertexAttribute(2, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  // D is left back
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kBackLabel});

  // Set the side and forward derivatives.
  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {10.0f, 0.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {0.0f, 10.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b

  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 3);
  EXPECT_EQ(NumVertices(*result), 5);

  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points = GetOutlinePoints(*result, 0, 0);
  EXPECT_THAT(outline_points,
              IsCyclicPermutationOf(std::vector<Point>{A, D, X3, X2, X1}));

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  CheckVertexLabels(result_mesh, A, kLeftLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, D, kLeftLabel, kBackLabel);
  CheckVertexLabels(result_mesh, X3, kRightLabel, kBackLabel);
  CheckVertexLabels(result_mesh, X2, kRightLabel, kInteriorLabel);
  CheckVertexLabels(result_mesh, X1, kRightLabel, kFrontLabel);
}

TEST(StrokeSubtractionTest, ComputeLabels2) {
  // Note that there is no single canonically "correct" labeling for boundary
  // vertices. This test verifies that the heuristic alignment cost optimization
  // approach implemented by StrokeSubtraction behaves as intended.

  // D-----------------------------C
  // |         mesh_a            / |
  // |                         /   |
  // |                       /     |
  // |                     /       |
  // |                   /         |
  // |                 /           |
  // |               /             |
  // |             /               |          +--> stroke travel direction
  // |           /                 |          |
  // |         /                   |          V
  // |       /    J--------I       |          right
  // |     /       \       |       |
  // |   /           K     H---G   |
  // | /            /          |   |
  // A-------------/-----------|---B
  //              /  mesh_b    |
  //             E-------------F

  Point A{0, 0}, B{10, 0}, C{10, 15}, D{0, 15};
  Point E{5, -2}, F{9, -2}, G{9, 2}, H{8, 2}, I{8, 3}, J{5, 3}, K{7, 2};

  Point X1{6, 0};  // intersection of AB and EK
  Point X2{9, 0};  // intersection of AB and FG

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  // Set up mesh_a
  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  // Set the labels: traveling along the stroke from start to end,
  // AD is front, CD is left, AB is right, BC is back.

  // A is right front
  mesh_a.SetFloatVertexAttribute(0, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  // B is right back
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kBackLabel});
  // C is left back
  mesh_a.SetFloatVertexAttribute(2, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  // D is left front
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kFrontLabel});

  // Set the side and forward derivatives.
  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {0.0f, -15.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {10.0f, 0.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H, I, J, K}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});
  mesh_b.AppendTriangleIndices({0, 3, 4});
  mesh_b.AppendTriangleIndices({0, 4, 6});
  mesh_b.AppendTriangleIndices({6, 4, 5});

  std::vector<uint32_t> mesh_b_outline = {6, 5, 4, 3, 2, 1, 0};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 9);
  EXPECT_EQ(NumVertices(*result), 11);

  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points = GetOutlinePoints(*result, 0, 0);
  EXPECT_THAT(outline_points, IsCyclicPermutationOf(std::vector<Point>{
                                  A, D, C, B, X2, G, H, I, J, K, X1}));

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  // Labeling the vertices is a bit of a puzzle. It may be useful to first note
  // the orientation of the edges (with respect to the orientation of mesh_a):
  // (A,X1) faces right, (X1,K) faces equally back and right, (K,J) faces mostly
  // left and slightly back, (J,I) right, (I,H) front, (H,G) right, (G,X2)
  // front, and (X2,B) right.
  //
  //              J--------I                  +--> stroke travel direction
  //               \       |                  |
  //                 K     H---G              V
  //                /          |             right
  // A-------------X1          X2---B
  //
  // We can roughly reason through as follows:

  CheckVertexLabels(result_mesh, A, kRightLabel, kFrontLabel);
  // edge (A,X1) should definitely be kRight, since (A,B) was kRight.
  CheckVertexLabels(result_mesh, X1, kRightLabel, kBackLabel);
  // edge (X1,K) could reasonably be kRight or kBack, but choosing kRight
  // would demand (K,J) also be kRight (assuming that (J,I) is kRight).
  // So kBack is chosen.
  CheckVertexLabels(result_mesh, K, kInteriorLabel, kBackLabel);
  // (K,J) is geometrically oriented left, but can not be labeled as such
  // (assuming  again that we'll choose (J,I) to be kRight). Given this,
  // kBack is the next best choice.
  CheckVertexLabels(result_mesh, J, kRightLabel, kBackLabel);
  // (J,I) seems pretty clearly kRight.
  CheckVertexLabels(result_mesh, I, kRightLabel, kFrontLabel);
  // (I,H) seems kFront, but could also be kRight. Choosing kRight here would
  // allow (H,G) to be labeled kRight, but then also force (G,X2) to be kRight,
  // effectively mislabeling two edges. So indeed, kFront.
  CheckVertexLabels(result_mesh, H, kInteriorLabel, kFrontLabel);
  // (H,G) would like to be kRight, but that's incompatible with (G,X2) being
  // set to kFront. Choosing (H,G) to be kFront is prioritized here because
  // (G,X2) is larger.
  CheckVertexLabels(result_mesh, G, kInteriorLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, X2, kRightLabel, kFrontLabel);
  // (X2,B) should be kRight, since (A,B) was kRight
  CheckVertexLabels(result_mesh, B, kRightLabel, kBackLabel);
}

TEST(StrokeSubtractionTest, ComputeLabels3) {
  // Tests the case where a frame-shaped `mesh_b` (EFGH - IJKL) erases the
  // entire outer boundary of `mesh_a` (ABCD), verifying that the newly formed
  // outer boundary of the surviving interior piece is assigned proper boundary
  // labels.
  //
  //  H-----------------------------------G
  //  |                                   |
  //  |                                   |
  //  |      D---------------------C      |
  //  |      |                     |      |
  //  |      |                     |      |   stroke travel direction
  //  |      |     L----------K    |      |              ^
  //  |      |     |          |    |      |              |
  //  |      |     |          |    |      |              +---> right
  //  |      |     |          |    |      |
  //  |      |     I----------J    |      |
  //  |      |                     |      |
  //  |      |                     |      |
  //  |      A---------------------B      |
  //  |                                   |
  //  E-----------------------------------F

  Point A{0, 0}, B{30, 0}, C{30, 30}, D{0, 30};
  Point E{-5, -5}, F{35, -5}, G{35, 35}, H{-5, 35}, I{10, 10}, J{20, 10},
      K{20, 20}, L{10, 20};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  mesh_a.SetFloatVertexAttribute(0, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(2, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kBackLabel});

  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {30.0f, 0.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {0.0f, 30.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b as an outer frame erasing mesh_a's perimeter:
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H, I, J, K, L}) {
    mesh_b.AppendVertex(p);
  }
  mesh_b.AppendTriangleIndices({0, 1, 5});  // E, F, J
  mesh_b.AppendTriangleIndices({0, 5, 4});  // E, J, I
  mesh_b.AppendTriangleIndices({1, 2, 6});  // F, G, K
  mesh_b.AppendTriangleIndices({1, 6, 5});  // F, K, J
  mesh_b.AppendTriangleIndices({2, 3, 7});  // G, H, L
  mesh_b.AppendTriangleIndices({2, 7, 6});  // G, L, K
  mesh_b.AppendTriangleIndices({3, 0, 4});  // H, E, I
  mesh_b.AppendTriangleIndices({3, 4, 7});  // H, I, L

  std::vector<uint32_t> mesh_b_frame_outer = {0, 3, 2, 1};  // EHGF
  std::vector<uint32_t> mesh_b_frame_inner = {4, 5, 6, 7};  // IJKL
  absl::StatusOr<PartitionedMesh> mesh_b_pm = PartitionedMesh::FromMutableMesh(
      mesh_b, {{mesh_b_frame_outer, mesh_b_frame_inner}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  CheckVertexLabels(result_mesh, I, kLeftLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, J, kRightLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, K, kRightLabel, kBackLabel);
  CheckVertexLabels(result_mesh, L, kLeftLabel, kBackLabel);
}

TEST(StrokeSubtractionTest, ComputeLabels4) {
  //  D------------------------------------C
  //  |                                    |
  //  |                                    |
  //  |           H----------G             |
  //  |           |          |             |
  //  |           |          |             |
  //  |           |          |             |   stroke travel direction
  //  |           |  mesh_b  |             |              ^
  //  |           E----------F             |              |
  //  |                                    |              +---> right
  //  |    mesh_a                          |
  //  A------------------------------------B

  Point A{0, 0}, B{30, 0}, C{30, 30}, D{0, 30};
  Point E{10, 10}, F{20, 10}, G{20, 20}, H{10, 20};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});  // A, B, C
  mesh_a.AppendTriangleIndices({0, 2, 3});  // A, C, D

  mesh_a.SetFloatVertexAttribute(0, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(2, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kBackLabel});

  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {30.0f, 0.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {0.0f, 30.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});  // E, F, G
  mesh_b.AppendTriangleIndices({0, 2, 3});  // E, G, H

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  // Check outer boundary vertices preserved:
  CheckVertexLabels(result_mesh, A, kLeftLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, B, kRightLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, C, kRightLabel, kBackLabel);
  CheckVertexLabels(result_mesh, D, kLeftLabel, kBackLabel);

  // Check hole boundary vertices:
  CheckVertexLabels(result_mesh, E, kRightLabel, kBackLabel);
  CheckVertexLabels(result_mesh, F, kLeftLabel, kBackLabel);
  CheckVertexLabels(result_mesh, G, kLeftLabel, kFrontLabel);
  CheckVertexLabels(result_mesh, H, kRightLabel, kFrontLabel);
}

TEST(StrokeSubtractionTest, ComputeSideDerivatives) {
  // The derivatives depend on the assigned boundary labels and the resulting
  // triangulation, both of which are not uniquely determined for a subtracted
  // mesh. This test verifies the derivative calculation assuming the boundary
  // labels from `ComputeLabels1` and checks only the derivatives that are
  // independent of the triangulation.
  //
  //           H-------------------G
  //           |      mesh_b       |
  // D---------+---------C         |     stroke travel direction
  // |         |       / |         |           ^
  // |         |     /   |         |           |
  // | mesh_a  |   /     |         |           +---> right
  // |         | /       |         |
  // |         |         |         |
  // |       / |         |         |
  // |     /   |         |         |
  // |   /     |         |         |
  // | /       |         |         |
  // A---------+---------B         |
  //           |                   |
  //           E-------------------F

  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};
  Point E{5, -5}, F{15, -5}, G{15, 15}, H{5, 15};
  Point X1{5, 0};   // intersection of AB and EH
  Point X2{5, 5};   // intersection of AC and EH
  Point X3{5, 10};  // intersection of CD and EH

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  // Set up mesh_a
  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  // Set the labels: traveling along the stroke from start to end,
  // AB is front, AD is left, BC is right, CD is back.
  mesh_a.SetFloatVertexAttribute(0, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(2, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kBackLabel});

  // Set the initial side and forward derivatives (width = 10, length = 10).
  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {10.0f, 0.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {0.0f, 10.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 3);
  EXPECT_EQ(NumVertices(*result), 5);

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  auto check_labels = [&](Point point, float expected_side,
                          float expected_fwd) {
    std::optional<uint32_t> idx = FindVertexIndex(result_mesh, point);
    ASSERT_TRUE(idx.has_value());
    ASSERT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*idx, 2)[0],
                    expected_side);
    ASSERT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*idx, 4)[0], expected_fwd);
  };

  check_labels(A, kLeftLabel, kFrontLabel);
  check_labels(D, kLeftLabel, kBackLabel);
  check_labels(X3, kRightLabel, kBackLabel);
  check_labels(X2, kRightLabel, kInteriorLabel);
  check_labels(X1, kRightLabel, kFrontLabel);

  auto check_side_derivative = [&](Point point, Vec expected_side) {
    std::optional<uint32_t> idx = FindVertexIndex(result_mesh, point);
    ASSERT_TRUE(idx.has_value());
    auto side = result_mesh.FloatVertexAttribute(*idx, 1);
    EXPECT_NEAR(side[0], expected_side.x, 1e-3);
    EXPECT_NEAR(side[1], expected_side.y, 1e-3);
  };

  // The remaining stroke has width = 5 so the recomputed side derivative across
  // all 5 vertices is exactly {5, 0}.
  check_side_derivative(A, {5.0f, 0.0f});
  check_side_derivative(D, {5.0f, 0.0f});
  check_side_derivative(X1, {5.0f, 0.0f});
  check_side_derivative(X2, {5.0f, 0.0f});
  check_side_derivative(X3, {5.0f, 0.0f});
}

TEST(StrokeSubtractionTest, ComputeForwardDerivatives) {
  // The derivatives depend on the assigned boundary labels and the resulting
  // triangulation, both of which are not uniquely determined for a subtracted
  // mesh. This test verifies the derivative calculation assuming the boundary
  // labels and checks only the derivatives that are independent of the
  // triangulation.
  //
  //     H-----------------------------G
  //     |          mesh_b             |
  //     |                             |
  //     |    D-------------------C    |       stroke travel direction
  //     |    |                 / |    |               ^
  //     |    |               /   |    |               |
  //     |    |             /     |    |               +---> right
  //     E----+-----------/-------+----F
  //          |         /         |
  //          |       /           |
  //          |     /             |
  //          |   /               |
  //          | /      mesh_a     |
  //          A-------------------B

  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};
  Point E{-5, 5}, F{15, 5}, G{15, 15}, H{-5, 15};
  Point X1{10, 5};  // intersection of BC and EF
  Point X2{5, 5};   // intersection of AC and EF
  Point X3{0, 5};   // intersection of AD and EF

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  // Set up mesh_a
  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  // Set the labels: traveling along the stroke from start to end,
  // AB is front, AD is left, BC is right, CD is back.
  mesh_a.SetFloatVertexAttribute(0, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(0, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(1, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(1, 4, {kFrontLabel});
  mesh_a.SetFloatVertexAttribute(2, 2, {kRightLabel});
  mesh_a.SetFloatVertexAttribute(2, 4, {kBackLabel});
  mesh_a.SetFloatVertexAttribute(3, 2, {kLeftLabel});
  mesh_a.SetFloatVertexAttribute(3, 4, {kBackLabel});

  for (uint32_t i = 0; i < 4; ++i) {
    mesh_a.SetFloatVertexAttribute(i, 1, {10.0f, 0.0f});
    mesh_a.SetFloatVertexAttribute(i, 3, {0.0f, 10.0f});
  }

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 3);
  EXPECT_EQ(NumVertices(*result), 5);

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  auto check_labels = [&](Point point, float expected_side,
                          float expected_fwd) {
    std::optional<uint32_t> idx = FindVertexIndex(result_mesh, point);
    ASSERT_TRUE(idx.has_value());
    ASSERT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*idx, 2)[0],
                    expected_side);
    ASSERT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*idx, 4)[0], expected_fwd);
  };

  check_labels(A, kLeftLabel, kFrontLabel);
  check_labels(B, kRightLabel, kFrontLabel);
  check_labels(X1, kRightLabel, kBackLabel);
  check_labels(X2, kInteriorLabel, kBackLabel);
  check_labels(X3, kLeftLabel, kBackLabel);

  auto check_forward_derivative = [&](Point point, Vec expected_fwd) {
    std::optional<uint32_t> idx = FindVertexIndex(result_mesh, point);
    ASSERT_TRUE(idx.has_value());
    auto fwd = result_mesh.FloatVertexAttribute(*idx, 3);
    EXPECT_NEAR(fwd[0], expected_fwd.x, 1e-3);
    EXPECT_NEAR(fwd[1], expected_fwd.y, 1e-3);
  };

  // The remaining stroke has length = 5 in y (from y=0 to y=5), so the
  // recomputed forward derivative across all 5 vertices is exactly {0, 5}.
  check_forward_derivative(A, {0.0f, 5.0f});
  check_forward_derivative(B, {0.0f, 5.0f});
  check_forward_derivative(X1, {0.0f, 5.0f});
  check_forward_derivative(X2, {0.0f, 5.0f});
  check_forward_derivative(X3, {0.0f, 5.0f});
}

TEST(StrokeSubtractionTest, AttributeInterpolation) {
  // This test checks that an attribute interpolates correctly in the subtracted
  // mesh. This requires the attribute to be set correctly on new vertices,
  // as well as the triangulation to be a refinement of the original.
  //
  //           H-------------------G
  //           |      mesh_b       |
  // D---------+---------C         |
  // |         |       / |         |
  // |         |     /   |         |
  // | mesh_a  |   /     |         |
  // |         | /       |         |
  // |         |         |         |
  // |       / |         |         |
  // |     /   |         |         |
  // |   /     |         |         |
  // | /       |         |         |
  // A---------+---------B         |
  //           |                   |
  //           E-------------------F
  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};

  // Set up mesh_a
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat1Unpacked, AttributeId::kCustom0}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  std::vector<Point> verts = {A, B, C, D};
  for (uint32_t i = 0; i < 4; i++) {
    mesh_a.AppendVertex(verts[i]);

    // Non-linear values for the vertex attribute
    mesh_a.SetFloatVertexAttribute(i, 1, {verts[i].y * verts[i].y});
  }
  mesh_a.AppendTriangleIndices({2, 0, 1});
  mesh_a.AppendTriangleIndices({2, 3, 0});

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a);
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  mesh_b.AppendVertex({5, -5});
  mesh_b.AppendVertex({15, -5});
  mesh_b.AppendVertex({15, 15});
  mesh_b.AppendVertex({5, 15});
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  constexpr uint32_t mesh_b_outline[] = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 3);
  EXPECT_EQ(NumVertices(*result), 5);
  // Check for equality of interpolated attribute value at a few points
  // within the subtracted mesh.
  std::vector<Point> test_points = {
      {1.0f, 1.0f}, {2.0f, 3.0f}, {4.0f, 4.0f},
      {0.5f, 9.5f}, {4.5f, 5.0f}, {2.5f, 7.5f},
  };

  for (Point p : test_points) {
    auto original_attr = AttributeValue(*mesh_a_pm, p, 1);
    ASSERT_TRUE(original_attr.has_value());

    auto subtracted_attr = AttributeValue(*result, p, 1);
    ASSERT_TRUE(subtracted_attr.has_value());

    EXPECT_THAT((*subtracted_attr)[0], FloatEq((*original_attr)[0]));
  }
}

TEST(StrokeSubtractionTest, HslColorShiftInterpolation) {
  //
  // A-----------B
  //  \  mesh_a /
  //   \       /
  //    \  D  /
  //     \/ \/
  //     /\ /\
  //    /  C  \
  //   /       \
  //  / mesh_b  \
  // E-----------F

  Point A{0, 10}, B{10, 10}, C{5, 2};
  Point D{5, 8}, E{0, 0}, F{10, 0};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat3Unpacked, AttributeId::kColorShiftHsl}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  mesh_a.AppendVertex(A);
  mesh_a.AppendVertex(B);
  mesh_a.AppendVertex(C);

  mesh_a.SetFloatVertexAttribute(0, 1, {0.9f, 0.5f, 0.4f});
  mesh_a.SetFloatVertexAttribute(1, 1, {0.8f, 0.5f, 0.8f});
  mesh_a.SetFloatVertexAttribute(2, 1, {0.1f, 0.4f, 0.2f});

  mesh_a.AppendTriangleIndices({0, 2, 1});

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a);
  ASSERT_THAT(mesh_a_pm, IsOk());

  MutableMesh mesh_b(MeshFormat{});
  mesh_b.AppendVertex(E);
  mesh_b.AppendVertex(D);
  mesh_b.AppendVertex(F);
  mesh_b.AppendTriangleIndices({0, 2, 1});

  constexpr uint32_t mesh_b_outline[] = {0, 1, 2};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];

  std::optional<uint32_t> index_d = FindVertexIndex(result_mesh, D);
  ASSERT_TRUE(index_d.has_value());
  ink::SmallArray<float, 4> hsl = result_mesh.FloatVertexAttribute(*index_d, 1);

  // Note that D = 3/8 A + 3/8 B + 2/8 C.
  // Recall also that the HSL values are:
  //  A: (0.9f, 0.5f, 0.4f)
  //  B: (0.8f, 0.5f, 0.8f)
  //  C: (0.1f, 0.4f, 0.2f)
  EXPECT_THAT(hsl.Values(),
              ElementsAre(
                  // Hue and saturation are linearly interpolated after mapping
                  // from polar coordinates to cartesian coordinates.
                  FloatNear(-0.099683f, kFloatTolerance),
                  FloatNear(0.125730f, kFloatTolerance),
                  // Lightness interpolates linearly.
                  FloatNear(0.5f, kFloatTolerance)));
}

TEST(StrokeSubtractionTest, MultipleCoats) {
  //                      H---------------------------G
  //                      |          mesh_b           |
  //  D-------------------+--------------C            |
  //  | mesh_a coat 0     |              |            |
  //  |       L-----------+-------K      |            |
  //  |       |           |       |      |            |
  //  |       | coat 1    |       |      |            |
  //  |       I-----------+-------J      |            |
  //  |                   |              |            |
  //  A-------------------+--------------B            |
  //                      |                           |
  //                      E---------------------------F
  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};
  Point I{2, 2}, J{8, 2}, K{8, 8}, L{2, 8};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a_coat_0(*format);
  for (const Point& p : {A, B, C, D}) mesh_a_coat_0.AppendVertex(p);
  mesh_a_coat_0.AppendTriangleIndices({0, 1, 2});
  mesh_a_coat_0.AppendTriangleIndices({0, 2, 3});

  MutableMesh mesh_a_coat_1(*format);
  for (const Point& p : {I, J, K, L}) mesh_a_coat_1.AppendVertex(p);
  mesh_a_coat_1.AppendTriangleIndices({0, 1, 2});
  mesh_a_coat_1.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_a_coat_0_outline = {0, 3, 2, 1};
  std::vector<uint32_t> mesh_a_coat_1_outline = {0, 3, 2, 1};

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMeshGroups({
          PartitionedMesh::MutableMeshGroup{
              .mesh = &mesh_a_coat_0, .outlines = {{mesh_a_coat_0_outline}}},
          PartitionedMesh::MutableMeshGroup{
              .mesh = &mesh_a_coat_1, .outlines = {{mesh_a_coat_1_outline}}},
      });
  ASSERT_THAT(mesh_a_pm, IsOk());

  Point E{5, -5}, F{15, -5}, G{15, 25}, H{5, 25};
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  // Result should have 2 coat.
  EXPECT_EQ(result->RenderGroupCount(), 2);

  // Coat 0 starts of with triangles ABC and ACD. After the subtraction, ACD
  // becomes a quad, and is re-triangulated into two triangles.
  EXPECT_EQ(result->RenderGroupMeshes(0).size(), 1);
  EXPECT_EQ(result->RenderGroupMeshes(0)[0].TriangleCount(), 3);
  EXPECT_EQ(result->RenderGroupMeshes(0)[0].VertexCount(), 5);

  EXPECT_EQ(result->RenderGroupMeshes(1).size(), 1);
  EXPECT_EQ(result->RenderGroupMeshes(1)[0].TriangleCount(), 3);
  EXPECT_EQ(result->RenderGroupMeshes(1)[0].VertexCount(), 5);

  Point X0{5, 0};   // Intersection of AB and mesh_b
  Point X1{5, 10};  // Intersection of CD and mesh_b
  Point X2{5, 5};   // Intersection of diagonal AC and mesh_b
  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points_0 = GetOutlinePoints(*result, 0, 0);
  EXPECT_THAT(outline_points_0,
              IsCyclicPermutationOf(std::vector<Point>{A, D, X1, X2, X0}));

  Point X3{5, 2};  // Intersection of IJ and mesh_b
  Point X4{5, 8};  // Intersection of KL and mesh_b
  Point X5{5, 5};  // Intersection of diagonal IK and mesh_b
  ASSERT_EQ(result->OutlineCount(1), 1);
  std::vector<Point> outline_points_1 = GetOutlinePoints(*result, 1, 0);
  EXPECT_THAT(outline_points_1,
              IsCyclicPermutationOf(std::vector<Point>{I, L, X4, X5, X3}));
}

TEST(StrokeSubtractionTest, FullDeleted) {
  //  H-----------------------------------G
  //  |             mesh_b                |
  //  |      D---------------------C      |
  //  |      |       mesh_a        |      |
  //  |      |                     |      |
  //  |      |                     |      |
  //  |      |                     |      |
  //  |      A---------------------B      |
  //  |                                   |
  //  E-----------------------------------F
  Point A{0, 0}, B{10, 0}, C{10, 10}, D{0, 10};
  Point E{-5, -5}, F{15, -5}, G{15, 15}, H{-5, 15};

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_a_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  // mesh_b completely encloses mesh_a
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  std::vector<uint32_t> mesh_b_outline = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), 0.1f);
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 0);
  EXPECT_EQ(NumVertices(*result), 0);
  EXPECT_EQ(result->OutlineCount(0), 0);
}

TEST(StrokeSubtractionTest, EpsilonDeduplication) {
  // This test checks that in the subtraction ABC - DEFGHI,  the vertices {D, E,
  // F}, which are spaced less than `epsilon` apart, are deduplicated in
  // subtraction result.
  //
  //                      C
  //                     / |
  //                    /  |
  //                   /   |
  //                  /    |
  //                 /     |
  //                /      |
  //               /       |
  //              /        |
  //             /         |
  //    G-------/--F E     |
  //    |      /     D     |
  //    |     A------|-----B
  //    |            |
  //    H------------I

  Point A{0, 0}, B{10, 0}, C{10, 10};
  Point E{3, 2}, I{3, -5}, H{-5, -5}, G{-5, 2};
  Point F{2.999f, 2.0f}, D{3.0f, 1.999f};  // Points close to E
  const float epsilon = .01f;

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});

  std::vector<uint32_t> mesh_a_outline = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a, {{mesh_a_outline}});
  ASSERT_THAT(mesh_a_pm, IsOk());

  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {D, E, F, G, H, I}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({4, 3, 2});  // H, G, F
  mesh_b.AppendTriangleIndices({4, 0, 5});  // H, D, I

  std::vector<uint32_t> mesh_b_outline = {5, 4, 3, 2, 1, 0};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity(), epsilon);
  ASSERT_THAT(result, IsOk());

  // The result should have 5 vertices, not 7, since D and F should have been
  // deduplicated.
  EXPECT_EQ(NumVertices(*result), 5);

  Point X0{3, 0};  // Intersection of AB and mesh_b
  Point X1{2, 2};  // Intersection of diagonal AC and mesh_b
  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points = GetOutlinePoints(*result, 0, 0);

  EXPECT_THAT(
      outline_points,
      IsCyclicPermutationOf(std::vector<Point>{B, X0, E, X1, C}, epsilon));
}

void StrokeSubtractionDoesNotCrash(
    std::pair<Triangle, Triangle> outer_and_inner) {
  const auto& [outer, inner] = outer_and_inner;
  PartitionedMesh mesh_a = MakeTrianglePartitionedMesh(outer, 3);
  PartitionedMesh mesh_b = MakeTrianglePartitionedMesh(inner, 3);
  auto result = Subtract(mesh_a, AffineTransform::Identity(), mesh_b,
                         AffineTransform::Identity(), 1e-4f);
  EXPECT_THAT(result, IsOk());
}
FUZZ_TEST(StrokeSubtractionTest, StrokeSubtractionDoesNotCrash)
    .WithDomains(NestedTriangles(Rect::FromCenterAndDimensions({0, 0}, 1e18f,
                                                               1e18f)));

}  // namespace
}  // namespace ink::strokes_internal
