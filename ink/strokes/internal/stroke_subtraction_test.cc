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
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::ElementsAre;
using ::testing::FloatEq;

using AttributeId = MeshFormat::AttributeId;
using AttributeType = MeshFormat::AttributeType;
using IndexFormat = MeshFormat::IndexFormat;

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
       {AttributeType::kFloat4Unpacked, AttributeId::kCustom1},
       {AttributeType::kFloat2Unpacked, AttributeId::kSideDerivative},
       {AttributeType::kFloat2Unpacked, AttributeId::kForwardDerivative}},
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

  mesh_a.SetFloatVertexAttribute(0, 3, {1.0f, 1.0f});
  mesh_a.SetFloatVertexAttribute(1, 3, {1.0f, 1.0f});
  mesh_a.SetFloatVertexAttribute(2, 3, {1.0f, 1.0f});

  mesh_a.SetFloatVertexAttribute(0, 4, {2.0f, 2.0f});
  mesh_a.SetFloatVertexAttribute(1, 4, {2.0f, 2.0f});
  mesh_a.SetFloatVertexAttribute(2, 4, {2.0f, 2.0f});

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a);
  ASSERT_THAT(mesh_a_pm, IsOk());

  // Set up mesh_b
  MutableMesh mesh_b(MeshFormat{});
  mesh_b.AppendVertex({0, 0});
  mesh_b.AppendVertex({20, 0});
  mesh_b.AppendVertex({0, 20});
  mesh_b.AppendTriangleIndices({0, 1, 2});
  AffineTransform mesh_b_transform = AffineTransform::Translate({5.0f, -5.0f});

  constexpr uint32_t mesh_b_outline[] = {0, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result = Subtract(
      *mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm, mesh_b_transform);
  ASSERT_THAT(result, IsOk());

  // The result should have 2 triangles and 4 vertices.
  EXPECT_EQ(NumTriangles(*result), 2);
  EXPECT_EQ(NumVertices(*result), 4);

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
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_A, 3).Values(),
              ElementsAre(FloatEq(1.0f), FloatEq(1.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_A, 4).Values(),
              ElementsAre(FloatEq(2.0f), FloatEq(2.0f)));

  std::optional<uint32_t> index_C = FindVertexIndex(result_mesh, C);
  ASSERT_TRUE(index_C.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_C, 1).Values()[0],
                  20.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_C, 2).Values(),
      ElementsAre(FloatEq(0.0f), FloatEq(0.0f), FloatEq(1.0f), FloatEq(1.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_C, 3).Values(),
              ElementsAre(FloatEq(1.0f), FloatEq(1.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_C, 4).Values(),
              ElementsAre(FloatEq(2.0f), FloatEq(2.0f)));

  // X1 and X2 are newly formed intersection points. The two custom attributes
  // should be interpolated, while the SideDerivative and ForwardDerivative
  // attributes should just be set to zero.
  std::optional<uint32_t> index_X1 = FindVertexIndex(result_mesh, X1);
  ASSERT_TRUE(index_X1.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_X1, 1).Values()[0],
                  5.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_X1, 2).Values(),
      ElementsAre(FloatEq(0.5f), FloatEq(0.5f), FloatEq(0.0f), FloatEq(1.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_X1, 3).Values(),
              ElementsAre(FloatEq(0.0f), FloatEq(0.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_X1, 4).Values(),
              ElementsAre(FloatEq(0.0f), FloatEq(0.0f)));

  std::optional<uint32_t> index_X2 = FindVertexIndex(result_mesh, X2);
  ASSERT_TRUE(index_X2.has_value());
  EXPECT_FLOAT_EQ(result_mesh.FloatVertexAttribute(*index_X2, 1).Values()[0],
                  15.0f);
  EXPECT_THAT(
      result_mesh.FloatVertexAttribute(*index_X2, 2).Values(),
      ElementsAre(FloatEq(0.0f), FloatEq(0.5f), FloatEq(0.5f), FloatEq(1.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_X2, 3).Values(),
              ElementsAre(FloatEq(0.0f), FloatEq(0.0f)));
  EXPECT_THAT(result_mesh.FloatVertexAttribute(*index_X2, 4).Values(),
              ElementsAre(FloatEq(0.0f), FloatEq(0.0f)));
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
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

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

  constexpr uint32_t mesh_b_outline[] = {0, 1, 2, 3};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  // Subtract
  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity());
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 2);
  EXPECT_EQ(NumVertices(*result), 4);

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

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMeshGroups({
          PartitionedMesh::MutableMeshGroup{.mesh = &mesh_a_coat_0},
          PartitionedMesh::MutableMeshGroup{.mesh = &mesh_a_coat_1},
      });
  ASSERT_THAT(mesh_a_pm, IsOk());

  Point E{5, -5}, F{15, -5}, G{15, 25}, H{5, 25};
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  constexpr uint32_t mesh_b_outline[] = {0, 1, 2, 3};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity());
  ASSERT_THAT(result, IsOk());

  // Result should have 2 coat.
  EXPECT_EQ(result->RenderGroupCount(), 2);

  // Each coat should have 2 triangles and 4 vertices after subtraction
  // of the right half.
  EXPECT_EQ(result->RenderGroupMeshes(0).size(), 1);
  EXPECT_EQ(result->RenderGroupMeshes(0)[0].TriangleCount(), 2);
  EXPECT_EQ(result->RenderGroupMeshes(0)[0].VertexCount(), 4);

  EXPECT_EQ(result->RenderGroupMeshes(1).size(), 1);
  EXPECT_EQ(result->RenderGroupMeshes(1)[0].TriangleCount(), 2);
  EXPECT_EQ(result->RenderGroupMeshes(1)[0].VertexCount(), 4);
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
      {{AttributeType::kFloat2Unpacked, AttributeId::kPosition}},
      IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh_a(*format);
  for (const Point& p : {A, B, C, D}) mesh_a.AppendVertex(p);
  mesh_a.AppendTriangleIndices({0, 1, 2});
  mesh_a.AppendTriangleIndices({0, 2, 3});

  absl::StatusOr<PartitionedMesh> mesh_a_pm =
      PartitionedMesh::FromMutableMesh(mesh_a);
  ASSERT_THAT(mesh_a_pm, IsOk());

  // mesh_b completely encloses mesh_a
  MutableMesh mesh_b(MeshFormat{});
  for (const Point& p : {E, F, G, H}) mesh_b.AppendVertex(p);
  mesh_b.AppendTriangleIndices({0, 1, 2});
  mesh_b.AppendTriangleIndices({0, 2, 3});

  constexpr uint32_t mesh_b_outline[] = {0, 3, 2, 1};
  absl::StatusOr<PartitionedMesh> mesh_b_pm =
      PartitionedMesh::FromMutableMesh(mesh_b, {{mesh_b_outline}});
  ASSERT_THAT(mesh_b_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*mesh_a_pm, AffineTransform::Identity(), *mesh_b_pm,
               AffineTransform::Identity());
  ASSERT_THAT(result, IsOk());

  EXPECT_EQ(NumTriangles(*result), 0);
  EXPECT_EQ(NumVertices(*result), 0);
}

}  // namespace
}  // namespace ink::strokes_internal
