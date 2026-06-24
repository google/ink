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

#include "ink/strokes/internal/stroke_segmentation.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/type_matchers.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;
using ::testing::ElementsAre;
using ::testing::SizeIs;

TEST(StrokeSegmentationTest, SegmentSpatially) {
  //  C                     F
  //  |\                    |\
  //  | \                   | \
  //  |__\                  |__\
  //  A  B                  D  E
  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{20, 0}, E{30, 0}, F{20, 10};

  // Note that the distance between the triangles is |BD| = 10.0f.

  MutableMesh mesh(MeshFormat{});
  for (const Point& p : {A, B, C, D, E, F}) mesh.AppendVertex(p);

  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({3, 4, 5});

  absl::StatusOr<PartitionedMesh> pm = PartitionedMesh::FromMutableMesh(mesh);
  ASSERT_THAT(pm, IsOk());

  // With small tolerance.
  {
    absl::StatusOr<std::vector<PartitionedMesh>> components =
        SegmentSpatially(*pm, AffineTransform::Identity(), /*tolerance=*/1.0f);
    ASSERT_THAT(components, IsOk());

    MutableMesh expected_mesh1(MeshFormat{});
    for (const Point& p : {A, B, C}) expected_mesh1.AppendVertex(p);
    expected_mesh1.AppendTriangleIndices({0, 1, 2});
    absl::StatusOr<PartitionedMesh> expected_pm1 =
        PartitionedMesh::FromMutableMesh(expected_mesh1);
    ASSERT_THAT(expected_pm1, IsOk());

    MutableMesh expected_mesh2(MeshFormat{});
    for (const Point& p : {D, E, F}) expected_mesh2.AppendVertex(p);
    expected_mesh2.AppendTriangleIndices({0, 1, 2});
    absl::StatusOr<PartitionedMesh> expected_pm2 =
        PartitionedMesh::FromMutableMesh(expected_mesh2);
    ASSERT_THAT(expected_pm2, IsOk());

    EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*expected_pm1),
                                         PartitionedMeshDeepEq(*expected_pm2)));
  }

  // Large tolerance
  {
    absl::StatusOr<std::vector<PartitionedMesh>> components =
        SegmentSpatially(*pm, AffineTransform::Identity(), /*tolerance=*/25.0f);
    ASSERT_THAT(components, IsOk());

    EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*pm)));
  }
}

TEST(StrokeSegmentationTest, SegmentSpatiallyWithTransform) {
  //  C                     F
  //  |\                    |\
  //  | \                   | \
  //  |__\                  |__\
  //  A  B                  D  E
  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{20, 0}, E{30, 0}, F{20, 10};

  MutableMesh mesh(MeshFormat{});
  for (const Point& p : {A, B, C, D, E, F}) mesh.AppendVertex(p);

  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({3, 4, 5});

  absl::StatusOr<PartitionedMesh> pm = PartitionedMesh::FromMutableMesh(mesh);
  ASSERT_THAT(pm, IsOk());

  {
    absl::StatusOr<std::vector<PartitionedMesh>> components =
        SegmentSpatially(*pm, AffineTransform::Scale(.05f), /*tolerance=*/1.0f);
    ASSERT_THAT(components, IsOk());
    EXPECT_THAT(*components, SizeIs(1));
  }

  {
    absl::StatusOr<std::vector<PartitionedMesh>> components =
        SegmentSpatially(*pm, AffineTransform::Scale(0.1f, 10.0f),
                         /*tolerance=*/1.0f);
    ASSERT_THAT(components, IsOk());
    EXPECT_THAT(*components, SizeIs(1));
    EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*pm)));
  }

  {
    absl::StatusOr<std::vector<PartitionedMesh>> components =
        SegmentSpatially(*pm, AffineTransform::Scale(10.0f, 1.0f),
                         /*tolerance=*/25.0f);
    ASSERT_THAT(components, IsOk());

    // The tolerance is large but the scale is also large.
    EXPECT_THAT(*components, SizeIs(2));
  }
}

TEST(StrokeSegmentationTest, SegmentSpatiallyMultipleCoats) {
  //  C                     F
  //  |\                    |\
  //  | \                   | \
  //  |  \                  |  \
  //  |   \                 |   \
  //  |Coat\                |Coat\
  //  |  0  \               |  1  \
  //  |______\              |______\
  //  A       B             D       E
  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{20, 0}, E{30, 0}, F{20, 10};

  MutableMesh mesh0(MeshFormat{});
  for (const Point& p : {A, B, C}) mesh0.AppendVertex(p);
  mesh0.AppendTriangleIndices({0, 1, 2});

  MutableMesh mesh1(MeshFormat{});
  for (const Point& p : {D, E, F}) mesh1.AppendVertex(p);
  mesh1.AppendTriangleIndices({0, 1, 2});

  std::vector<PartitionedMesh::MutableMeshGroup> groups = {
      {.mesh = &mesh0},
      {.mesh = &mesh1},
  };
  absl::StatusOr<PartitionedMesh> pm =
      PartitionedMesh::FromMutableMeshGroups(groups);
  ASSERT_THAT(pm, IsOk());

  absl::StatusOr<std::vector<PartitionedMesh>> components =
      SegmentSpatially(*pm, AffineTransform::Identity(), /*tolerance=*/1.0f);
  ASSERT_THAT(components, IsOk());

  EXPECT_THAT(*components, SizeIs(2));

  MutableMesh empty_mesh(MeshFormat{});

  std::vector<PartitionedMesh::MutableMeshGroup> expected_groups0 = {
      {.mesh = &mesh0},
      {.mesh = &empty_mesh},
  };
  absl::StatusOr<PartitionedMesh> expected_pm0 =
      PartitionedMesh::FromMutableMeshGroups(expected_groups0);
  ASSERT_THAT(expected_pm0, IsOk());

  std::vector<PartitionedMesh::MutableMeshGroup> expected_groups1 = {
      {.mesh = &empty_mesh},
      {.mesh = &mesh1},
  };
  absl::StatusOr<PartitionedMesh> expected_pm1 =
      PartitionedMesh::FromMutableMeshGroups(expected_groups1);
  ASSERT_THAT(expected_pm1, IsOk());

  EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*expected_pm0),
                                       PartitionedMeshDeepEq(*expected_pm1)));
}

TEST(StrokeSegmentationTest, SegmentSpatiallyMultipleCoats2) {
  //
  //   C                  I                  F
  //   |\                 |\                 |\
  //   | \                | \                | \
  //   |  \               |  \               |  \
  //   |   \              |   \              |   \
  //   |Coat\             |Coat\             |Coat\
  //   |  0  \            |  1  \            |  0  \
  //   |______\           |______\           |______\
  //   A       B          G       H          D       E
  //           |--- 10 ---|       |--- 10 ---|
  //
  //
  // This tests that SegmentSpatially considers connectivity across coats: while
  // the two triangles of coat 0 are further than `tolerance` apart, the middle
  // triangle of coat 1 is within `tolerance` of both of them, and therefore
  // holds the entire mesh together.

  const float tolerance = 15.0f;

  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{40, 0}, E{50, 0}, F{40, 10};
  Point G{20, 0}, H{30, 0}, I{20, 10};

  MutableMesh mesh0(MeshFormat{});
  for (const Point& p : {A, B, C, D, E, F}) mesh0.AppendVertex(p);
  mesh0.AppendTriangleIndices({0, 1, 2});
  mesh0.AppendTriangleIndices({3, 4, 5});

  MutableMesh mesh1(MeshFormat{});
  for (const Point& p : {G, H, I}) mesh1.AppendVertex(p);
  mesh1.AppendTriangleIndices({0, 1, 2});

  std::vector<PartitionedMesh::MutableMeshGroup> groups = {
      {.mesh = &mesh0},
      {.mesh = &mesh1},
  };
  absl::StatusOr<PartitionedMesh> pm =
      PartitionedMesh::FromMutableMeshGroups(groups);
  ASSERT_THAT(pm, IsOk());

  absl::StatusOr<std::vector<PartitionedMesh>> components =
      SegmentSpatially(*pm, AffineTransform::Identity(), tolerance);
  ASSERT_THAT(components, IsOk());

  // Although the two triangles of coat 0 are far apart, coat 1 ties them
  // together.
  EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*pm)));
}

TEST(StrokeSegmentationTest, SegmentSpatiallyWithAttributes) {
  //  C                     F
  //  |\                    |\
  //  | \                   | \
  //  |__\                  |__\
  //  A  B                  D  E
  Point A{0, 0}, B{10, 0}, C{0, 10};
  Point D{20, 20}, E{30, 20}, F{20, 30};

  absl::StatusOr<MeshFormat> format =
      MeshFormat::Create({{MeshFormat::AttributeType::kFloat2Unpacked,
                           MeshFormat::AttributeId::kPosition},
                          {MeshFormat::AttributeType::kFloat1Unpacked,
                           MeshFormat::AttributeId::kColorShiftHsl}},
                         MeshFormat::IndexFormat::k32BitUnpacked16BitPacked);
  ASSERT_THAT(format, IsOk());

  MutableMesh mesh(*format);
  for (const Point& p : {A, B, C, D, E, F}) mesh.AppendVertex(p);
  mesh.SetFloatVertexAttribute(0, 1, {0.1f});
  mesh.SetFloatVertexAttribute(1, 1, {0.2f});
  mesh.SetFloatVertexAttribute(2, 1, {0.3f});
  mesh.SetFloatVertexAttribute(3, 1, {0.4f});
  mesh.SetFloatVertexAttribute(4, 1, {0.5f});
  mesh.SetFloatVertexAttribute(5, 1, {0.6f});

  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({3, 4, 5});

  absl::StatusOr<PartitionedMesh> pm = PartitionedMesh::FromMutableMesh(mesh);
  ASSERT_THAT(pm, IsOk());

  absl::StatusOr<std::vector<PartitionedMesh>> components =
      SegmentSpatially(*pm, AffineTransform::Identity(), /*tolerance=*/1.0f);
  ASSERT_THAT(components, IsOk());

  MutableMesh expected_mesh1(*format);
  for (const Point& p : {A, B, C}) expected_mesh1.AppendVertex(p);
  expected_mesh1.SetFloatVertexAttribute(0, 1, {0.1f});
  expected_mesh1.SetFloatVertexAttribute(1, 1, {0.2f});
  expected_mesh1.SetFloatVertexAttribute(2, 1, {0.3f});
  expected_mesh1.AppendTriangleIndices({0, 1, 2});
  absl::StatusOr<PartitionedMesh> expected_pm1 =
      PartitionedMesh::FromMutableMesh(expected_mesh1);
  ASSERT_THAT(expected_pm1, IsOk());

  MutableMesh expected_mesh2(*format);
  for (const Point& p : {D, E, F}) expected_mesh2.AppendVertex(p);
  expected_mesh2.SetFloatVertexAttribute(0, 1, {0.4f});
  expected_mesh2.SetFloatVertexAttribute(1, 1, {0.5f});
  expected_mesh2.SetFloatVertexAttribute(2, 1, {0.6f});
  expected_mesh2.AppendTriangleIndices({0, 1, 2});
  absl::StatusOr<PartitionedMesh> expected_pm2 =
      PartitionedMesh::FromMutableMesh(expected_mesh2);
  ASSERT_THAT(expected_pm2, IsOk());

  EXPECT_THAT(*components, ElementsAre(PartitionedMeshDeepEq(*expected_pm1),
                                       PartitionedMeshDeepEq(*expected_pm2)));
}

}  // namespace
}  // namespace ink::strokes_internal
