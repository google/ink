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

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::ink::geometry_internal::IsCyclicPermutationOf;
using ::testing::ElementsAre;
using ::testing::FloatEq;
using ::testing::Not;

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

std::pair<uint32_t, uint32_t> NumVerticesAndTriangles(
    const PartitionedMesh& partitioned_mesh) {
  uint32_t vertices = 0;
  uint32_t triangles = 0;
  for (uint32_t g = 0; g < partitioned_mesh.RenderGroupCount(); ++g) {
    for (const auto& m : partitioned_mesh.RenderGroupMeshes(g)) {
      vertices += m.VertexCount();
      triangles += m.TriangleCount();
    }
  }
  return {vertices, triangles};
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

  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(3, 1));
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

  // The result should have 4 vertices and 2 triangles.
  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(4, 2));

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

TEST(StrokeSubtractionTest, VertexWelding) {
  // We should make sure that there are no duplicate vertices or seams in the
  // subtraction result, especially when a subtraction cuts through an edge
  // shared by two triangles.
  //
  // H-------------------G
  // |   mesh_b          |
  // |         A         |
  // |        /|\        |
  // E-------/-|-\-------F
  //        /  |  \
  //       /   |   \
  //      /    |    \
  //     /     |     \
  //    /      |      \
  //   /       |       \
  //  /        | mesh_a \
  // B---------C---------D

  Point A{0, 0}, B{-5, -1e6f}, C{0, -1e6f}, D{5, -1e6f};
  Point E{-3, -2}, F{3, -2}, G{3, 2}, H{-3, 2};

  // Set up mesh_a
  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition}},
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

  // The result should have 6 vertices and 4 triangles.
  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(6, 4));
}

TEST(StrokeSubtractionTest, DegenerateTriangle) {
  // Degenerate triangles arise frequently in input meshes and need to be
  // handled with care to ensure that they do not leave seams in the mesh or
  // zig-zags in the outline. `Subtract` chooses to filter out the degenerate
  // triangles by welding along the duplicate edge.

  //           H---------------------G
  //           |             mesh_b  |
  // D---------+--------C,C'         |
  // |         |       / |           |
  // |         |     /   |           |
  // | mesh_a  |   /     |           |
  // |         | /       |           |
  // |         |         |           |
  // |       / |         |           |
  // |     /   |         |           |
  // |   /     |         |           |
  // | /       |         |           |
  // A---------+---------B           |
  //           |                     |
  //           E---------------------F
  Point A{0, 0}, B{10, 0}, C{10, 10}, C_prime{10, 10}, D{0, 10};
  Point E{5, -5}, F{15, -5}, G{15, 15}, H{5, 15};
  Point X1{5, 0};   // Intersection of AB and EH
  Point X2{5, 5};   // Intersection of AC and EH
  Point X3{5, 10};  // Intersection of CD and EH

  // Set up mesh_a with a degenerate triangle {C, C', A} connecting two
  // triangles where C and C' have coincident positions.
  MutableMesh mesh_a(MeshFormat{});
  for (const Point& p : {A, B, C, D, C_prime}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({2, 0, 1});  // {C, A, B}
  mesh_a.AppendTriangleIndices({4, 3, 0});  // {C', D, A}
  mesh_a.AppendTriangleIndices({2, 4, 0});  // {C, C', A} (degenerate)

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a);
  ASSERT_THAT(mesh_a_pm, IsOk());

  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);

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

  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(5, 3));

  ASSERT_EQ(result->OutlineCount(0), 1);
  std::vector<Point> outline_points = GetOutlinePoints(*result, 0, 0);
  EXPECT_THAT(outline_points,
              IsCyclicPermutationOf(std::vector<Point>{A, D, X3, X2, X1}));
}

TEST(StrokeSubtractionTest, UpdatesAntiAliasingAttributes) {
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
  Point X2{5, 5};  // intersection of AC and EH

  absl::StatusOr<MeshFormat> format = MeshFormat::Create(
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kSideLabel},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative},
       {AttributeType::kFloat1Unpacked, AttributeId::kForwardLabel}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  constexpr float kLeftLabel = -127.0f;
  constexpr float kRightLabel = 127.0f;
  constexpr float kFrontLabel = -127.0f;
  constexpr float kBackLabel = 127.0f;

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

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

  const Mesh& result_mesh = result->RenderGroupMeshes(0)[0];
  std::optional<uint32_t> index_X2 = FindVertexIndex(result_mesh, X2);
  ASSERT_TRUE(index_X2.has_value());
  EXPECT_NE(result_mesh.FloatVertexAttribute(*index_X2, 2)[0], 0.0f);
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_X2, 1).Values(),
              Not(ElementsAre(0.0f, 0.0f)));
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

  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(5, 3));
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

  EXPECT_EQ(NumVerticesAndTriangles(*result), std::pair(0, 0));
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
  EXPECT_EQ(NumVerticesAndTriangles(*result).first, 5);

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
