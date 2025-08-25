// Copyright 2025 Google LLC
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

#include "ink/strokes/internal/mutable_multi_mesh.h"

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/triangle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::ElementsAre;
using ::testing::FloatEq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(MutableMultiMeshTest, Empty) {
  MutableMultiMesh mesh = MutableMultiMesh(MeshFormat());
  EXPECT_EQ(mesh.VertexCount(), 0);
  EXPECT_EQ(mesh.TriangleCount(), 0);
  EXPECT_THAT(mesh.GetMeshes(), IsEmpty());
}

TEST(MutableMultiMeshTest, PartitionAfterTooManyVertices) {
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 25; ++i) mesh.AppendVertex({0, 0});
  EXPECT_EQ(mesh.VertexCount(), 25);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(3));
  EXPECT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_EQ(mesh.GetMeshes()[1].VertexCount(), 10);
  EXPECT_EQ(mesh.GetMeshes()[2].VertexCount(), 5);
}

TEST(MutableMultiMeshTest, UsesSameMeshFormatForAllPartitions) {
  // Create a new `MutableMultiMesh` with some non-trivial mesh format.
  absl::StatusOr<MeshFormat> format =
      MeshFormat::Create({{MeshFormat::AttributeType::kFloat4PackedInOneFloat,
                           MeshFormat::AttributeId::kColorShiftHsl},
                          {MeshFormat::AttributeType::kFloat2PackedInOneFloat,
                           MeshFormat::AttributeId::kPosition},
                          {MeshFormat::AttributeType::kFloat3PackedInTwoFloats,
                           MeshFormat::AttributeId::kCustom0}},
                         MeshFormat::IndexFormat::k16BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMultiMesh mesh = MutableMultiMesh(*format, /* partition_after= */ 10);
  EXPECT_THAT(mesh.Format(), MeshFormatEq(*format));
  // Append a bunch of vertices to force the mesh to contain several partitions.
  for (int i = 0; i < 25; ++i) mesh.AppendVertex({0, 0});
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(3));
  // All of the partition meshes should be using the specified format.
  for (const MutableMesh& partition : mesh.GetMeshes()) {
    EXPECT_THAT(partition.Format(), MeshFormatEq(*format));
  }
}

TEST(MutableMultiMeshTest, PartitionedTriangles) {
  // Create a mesh with 8 triangles across 3 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 9);
  for (int i = 0; i < 9; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  for (uint32_t i = 0; i < 3; ++i) {
    mesh.AppendTriangleIndices({i * 3, i * 3 + 1, i * 3 + 2});
  }
  for (int i = 9; i < 18; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  for (uint32_t i = 3; i < 6; ++i) {
    mesh.AppendTriangleIndices({i * 3, i * 3 + 1, i * 3 + 2});
  }
  for (int i = 18; i < 24; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  for (uint32_t i = 6; i < 8; ++i) {
    mesh.AppendTriangleIndices({i * 3, i * 3 + 1, i * 3 + 2});
  }
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(3));
  ASSERT_EQ(mesh.TriangleCount(), 8);
  // The triangles should be properly distributed among the partitions.
  ASSERT_EQ(mesh.GetMeshes()[0].TriangleCount(), 3);
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(0)));
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(1),
              TriangleEq(mesh.GetTriangle(1)));
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(2),
              TriangleEq(mesh.GetTriangle(2)));
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 3);
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(3)));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(1),
              TriangleEq(mesh.GetTriangle(4)));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(2),
              TriangleEq(mesh.GetTriangle(5)));
  ASSERT_EQ(mesh.GetMeshes()[2].TriangleCount(), 2);
  EXPECT_THAT(mesh.GetMeshes()[2].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(6)));
  EXPECT_THAT(mesh.GetMeshes()[2].GetTriangle(1),
              TriangleEq(mesh.GetTriangle(7)));
}

TEST(MutableMultiMeshTest, AppendTriangleIndicesCopiesVerticesIntoMesh) {
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  // Append 11 vertices (10 of which end up in the first mesh), then make a
  // triangle out of the last three.
  for (int i = 0; i < 11; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({8, 9, 10});
  ASSERT_EQ(mesh.TriangleCount(), 1);
  EXPECT_THAT(mesh.TriangleIndices(0), ElementsAre(8, 9, 10));
  EXPECT_THAT(mesh.GetTriangle(0),
              TriangleEq(Triangle{{8, 0}, {9, 0}, {10, 0}}));
  // Vertices 8 and 9 should get copied into the second mesh, but the multi-mesh
  // as a whole should still only have 11 vertices.
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  ASSERT_EQ(mesh.GetMeshes()[1].VertexCount(), 3);
  EXPECT_EQ(mesh.VertexCount(), 11);
  // The second mesh should contain vertices 8, 9, and 10 (in the order they
  // were added to it), but its triangle should refer to those vertices by their
  // local indices.
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(0), PointEq({10, 0}));
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(1), PointEq({8, 0}));
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(2), PointEq({9, 0}));
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  EXPECT_THAT(mesh.GetMeshes()[1].TriangleIndices(0), ElementsAre(1, 2, 0));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(0)));
}

TEST(MutableMultiMeshTest, SetTriangleIndicesCopiesVerticesIntoMesh) {
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  // Append 13 vertices (10 of which end up in the first mesh), then make a
  // triangle out of the last three.
  for (int i = 0; i < 13; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  EXPECT_EQ(mesh.VertexCount(), 13);
  ASSERT_EQ(mesh.TriangleCount(), 1);
  EXPECT_THAT(mesh.TriangleIndices(0), ElementsAre(10, 11, 12));
  // The triangle should be in the second partition.
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 0);
  EXPECT_EQ(mesh.GetMeshes()[1].VertexCount(), 3);
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  EXPECT_THAT(mesh.GetMeshes()[1].TriangleIndices(0), ElementsAre(0, 1, 2));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(0)));
  // Now change the triangle's indices from {10, 11, 12} to {9, 10, 11}.
  mesh.SetTriangleIndices(0, {9, 10, 11});
  EXPECT_EQ(mesh.VertexCount(), 13);
  ASSERT_EQ(mesh.TriangleCount(), 1);
  EXPECT_THAT(mesh.TriangleIndices(0), ElementsAre(9, 10, 11));
  // Vertex 9 should have been copied from the first partition into the second,
  // so that the second partition can store the updated triangle.
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_THAT(mesh.GetMeshes()[0].VertexPosition(9), PointEq({9, 0}));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 0);
  ASSERT_EQ(mesh.GetMeshes()[1].VertexCount(), 4);
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(3), PointEq({9, 0}));
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  EXPECT_THAT(mesh.GetMeshes()[1].TriangleIndices(0), ElementsAre(3, 0, 1));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(0)));
}

TEST(MutableMultiMeshTest, CopiedVerticesIncludeNonPositionAttributes) {
  absl::StatusOr<MeshFormat> format =
      MeshFormat::Create({{MeshFormat::AttributeType::kFloat2Unpacked,
                           MeshFormat::AttributeId::kPosition},
                          {MeshFormat::AttributeType::kFloat1Unpacked,
                           MeshFormat::AttributeId::kOpacityShift}},
                         MeshFormat::IndexFormat::k16BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMultiMesh mesh = MutableMultiMesh(*format, /* partition_after= */ 10);
  // Append 12 vertices (10 of which end up in the first mesh), and set a
  // non-position attribute on each one.
  for (int i = 0; i < 12; ++i) {
    mesh.AppendVertex({static_cast<float>(i), 0});
    mesh.SetFloatVertexAttribute(i, 1, SmallArray<float, 4>{i * 0.01f});
  }
  // Make a triangle out of the last three vertices.  Vertex 9 should get copied
  // into the second partition.
  mesh.AppendTriangleIndices({9, 10, 11});
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_THAT(mesh.GetMeshes()[0].VertexPosition(9), PointEq({9, 0}));
  ASSERT_EQ(mesh.GetMeshes()[1].VertexCount(), 3);
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(2), PointEq({9, 0}));
  // The copied vertex should also include the non-position attribute.
  EXPECT_THAT(mesh.GetMeshes()[0].FloatVertexAttribute(9, 1).Values(),
              ElementsAre(FloatEq(0.09)));
  EXPECT_THAT(mesh.GetMeshes()[1].FloatVertexAttribute(2, 1).Values(),
              ElementsAre(FloatEq(0.09)));
}

TEST(MutableMultiMeshTest, SetVertexPositionUpdatesAllCopies) {
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  // Append 12 vertices (10 of which end up in the first mesh), then make a
  // triangle out of the last three, which copies vertex 9 into the second mesh.
  for (int i = 0; i < 12; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({9, 10, 11});
  // Verify that vertex 9 appears not only in the multi-mesh, but in both
  // individual meshes.
  ASSERT_EQ(mesh.VertexCount(), 12);
  EXPECT_THAT(mesh.VertexPosition(9), PointEq({9, 0}));
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  ASSERT_EQ(mesh.GetMeshes()[1].VertexCount(), 3);
  EXPECT_THAT(mesh.GetMeshes()[0].VertexPosition(9), PointEq({9, 0}));
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(2), PointEq({9, 0}));
  // Update the position of vertex 9.  Its position should get updated not only
  // in the multi-mesh, but in both individual meshes.
  mesh.SetVertexPosition(9, {9, 99});
  EXPECT_THAT(mesh.VertexPosition(9), PointEq({9, 99}));
  EXPECT_THAT(mesh.GetMeshes()[0].VertexPosition(9), PointEq({9, 99}));
  EXPECT_THAT(mesh.GetMeshes()[1].VertexPosition(2), PointEq({9, 99}));
}

TEST(MutableMultiMeshTest, SetFloatVertexAttributeUpdatesAllCopies) {
  absl::StatusOr<MeshFormat> format =
      MeshFormat::Create({{MeshFormat::AttributeType::kFloat2Unpacked,
                           MeshFormat::AttributeId::kPosition},
                          {MeshFormat::AttributeType::kFloat1Unpacked,
                           MeshFormat::AttributeId::kOpacityShift}},
                         MeshFormat::IndexFormat::k16BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());
  MutableMultiMesh mesh = MutableMultiMesh(*format, /* partition_after= */ 10);
  // Append 12 vertices (10 of which end up in the first mesh), then make a
  // triangle out of the last three, which copies vertex 9 into the second mesh.
  for (int i = 0; i < 12; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({9, 10, 11});
  // Update the opacity shift of vertex 9.  Its opacity shift should get updated
  // in both individual meshes.
  ASSERT_EQ(mesh.VertexCount(), 12);
  mesh.SetFloatVertexAttribute(9, 1, SmallArray<float, 4>{0.625});
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_THAT(mesh.GetMeshes()[0].FloatVertexAttribute(9, 1).Values(),
              ElementsAre(0.625));
  ASSERT_EQ(mesh.GetMeshes()[1].VertexCount(), 3);
  EXPECT_THAT(mesh.GetMeshes()[1].FloatVertexAttribute(2, 1).Values(),
              ElementsAre(0.625));
}

TEST(MutableMultiMeshTest, InsertTriangleIndicesInMiddle) {
  // Create a mesh with 3 triangles across 2 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 10; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({6, 7, 8});
  for (int i = 10; i < 20; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  EXPECT_EQ(mesh.TriangleCount(), 3);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 2);
  EXPECT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  // Insert a new triangle into the first partition.
  mesh.InsertTriangleIndices(1, {3, 4, 5});
  EXPECT_EQ(mesh.TriangleCount(), 4);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].TriangleCount(), 3);
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  // The triangles should be correctly distributed between the two partitions.
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(0)));
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(1),
              TriangleEq(mesh.GetTriangle(1)));
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(2),
              TriangleEq(mesh.GetTriangle(2)));
  EXPECT_THAT(mesh.GetMeshes()[1].GetTriangle(0),
              TriangleEq(mesh.GetTriangle(3)));
}

TEST(MutableMultiMeshTest, InsertTriangleIndicesAtEnd) {
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 14; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  EXPECT_EQ(mesh.TriangleCount(), 1);
  mesh.InsertTriangleIndices(1, {11, 12, 13});
  ASSERT_EQ(mesh.TriangleCount(), 2);
  EXPECT_THAT(mesh.TriangleIndices(0), ElementsAre(10, 11, 12));
  EXPECT_THAT(mesh.TriangleIndices(1), ElementsAre(11, 12, 13));
}

TEST(MutableMultiMeshTest, InsertTriangleIndicesCopiesVerticesIntoMesh) {
  // Create a mesh with 3 triangles across 2 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 10; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({6, 7, 8});
  for (int i = 10; i < 20; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  EXPECT_EQ(mesh.VertexCount(), 20);
  EXPECT_EQ(mesh.TriangleCount(), 3);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 2);
  EXPECT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  // Insert a new triangle into the first partition, using a vertex from the
  // second partition.
  mesh.InsertTriangleIndices(1, {3, 4, 15});
  EXPECT_EQ(mesh.TriangleCount(), 4);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  ASSERT_EQ(mesh.GetMeshes()[0].TriangleCount(), 3);
  ASSERT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
  // Vertex 15 should have been copied from the second partition into the first,
  // so that the first partition can store the new triangle.
  EXPECT_EQ(mesh.VertexCount(), 20);
  ASSERT_EQ(mesh.GetMeshes()[0].VertexCount(), 11);
  EXPECT_EQ(mesh.GetMeshes()[1].VertexCount(), 10);
  EXPECT_THAT(mesh.GetMeshes()[0].VertexPosition(10), PointEq({15, 0}));
  EXPECT_THAT(mesh.GetMeshes()[0].TriangleIndices(1), ElementsAre(3, 4, 10));
  EXPECT_THAT(mesh.GetMeshes()[0].GetTriangle(1),
              TriangleEq(mesh.GetTriangle(1)));
}

TEST(MutableMultiMeshTest, TruncateTrianglesAndVertices) {
  // Create a mesh with 25 vertices and 5 triangles across 3 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 10; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({6, 7, 8});
  for (int i = 10; i < 20; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  mesh.AppendTriangleIndices({16, 17, 18});
  for (int i = 20; i < 25; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({20, 21, 22});
  EXPECT_EQ(mesh.VertexCount(), 25);
  EXPECT_EQ(mesh.TriangleCount(), 5);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(3));
  EXPECT_EQ(mesh.GetMeshes()[0].VertexCount(), 10);
  EXPECT_EQ(mesh.GetMeshes()[1].VertexCount(), 10);
  EXPECT_EQ(mesh.GetMeshes()[2].VertexCount(), 5);

  // Truncating to a larger number of triangles should have no effect.
  mesh.TruncateTriangles(10);
  EXPECT_EQ(mesh.VertexCount(), 25);
  EXPECT_EQ(mesh.TriangleCount(), 5);
  EXPECT_THAT(mesh.GetMeshes(), SizeIs(3));

  // Truncating to a larger number of vertices should have no effect.
  mesh.TruncateVertices(30);
  EXPECT_EQ(mesh.VertexCount(), 25);
  EXPECT_EQ(mesh.TriangleCount(), 5);
  EXPECT_THAT(mesh.GetMeshes(), SizeIs(3));

  // Truncate down to 2 triangles.
  mesh.TruncateTriangles(2);
  EXPECT_EQ(mesh.VertexCount(), 25);
  EXPECT_EQ(mesh.TriangleCount(), 2);
  EXPECT_THAT(mesh.GetMeshes(), SizeIs(3));

  // Truncate down to 9 vertices.  Any now-empty partitions should be removed.
  mesh.TruncateVertices(9);
  EXPECT_EQ(mesh.VertexCount(), 9);
  EXPECT_EQ(mesh.TriangleCount(), 2);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(1));
  EXPECT_EQ(mesh.GetMeshes()[0].VertexCount(), 9);

  // Truncate down to empty.
  mesh.TruncateTriangles(0);
  mesh.TruncateVertices(0);
  EXPECT_EQ(mesh.VertexCount(), 0);
  EXPECT_EQ(mesh.TriangleCount(), 0);
  EXPECT_THAT(mesh.GetMeshes(), IsEmpty());
}

TEST(MutableMultiMeshTest, Clear) {
  // Create a mesh with 15 vertices and 3 triangles across 2 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 10; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({6, 7, 8});
  for (int i = 10; i < 15; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  EXPECT_EQ(mesh.VertexCount(), 15);
  EXPECT_EQ(mesh.TriangleCount(), 3);
  EXPECT_THAT(mesh.GetMeshes(), SizeIs(2));

  mesh.Clear();
  EXPECT_EQ(mesh.VertexCount(), 0);
  EXPECT_EQ(mesh.TriangleCount(), 0);
  EXPECT_THAT(mesh.GetMeshes(), SizeIs(0));
}

TEST(MutableMultiMeshTest, TruncatingTrianglesMaintainsIndexMappings) {
  // Create a mesh with 5 triangles across 2 partitions.
  MutableMultiMesh mesh =
      MutableMultiMesh(MeshFormat(), /* partition_after= */ 10);
  for (int i = 0; i < 10; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({3, 4, 5});
  mesh.AppendTriangleIndices({6, 7, 8});
  for (int i = 10; i < 20; ++i) mesh.AppendVertex({static_cast<float>(i), 0});
  mesh.AppendTriangleIndices({10, 11, 12});
  mesh.AppendTriangleIndices({13, 14, 15});
  EXPECT_EQ(mesh.TriangleCount(), 5);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 3);
  EXPECT_EQ(mesh.GetMeshes()[1].TriangleCount(), 2);

  // Truncate down to 2 triangles.
  mesh.TruncateTriangles(2);
  EXPECT_EQ(mesh.TriangleCount(), 2);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 2);
  EXPECT_EQ(mesh.GetMeshes()[1].TriangleCount(), 0);

  // Now append a new triangle, and verify that the triangle counts (both for
  // the whole multi-mesh, and for each partition) are still correct.
  mesh.AppendTriangleIndices({16, 17, 18});
  EXPECT_EQ(mesh.TriangleCount(), 3);
  ASSERT_THAT(mesh.GetMeshes(), SizeIs(2));
  EXPECT_EQ(mesh.GetMeshes()[0].TriangleCount(), 2);
  EXPECT_EQ(mesh.GetMeshes()[1].TriangleCount(), 1);
}

}  // namespace
}  // namespace ink::strokes_internal
