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

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"

namespace ink::strokes_internal {
namespace {

using ::absl_testing::IsOk;

TEST(StrokeSubtractionTest, SubtractRemovesIntersectingTriangles) {
  MutableMesh mesh(MeshFormat{});
  mesh.AppendVertex(Point{0, 0});
  mesh.AppendVertex(Point{10, 0});
  mesh.AppendVertex(Point{0, 10});
  mesh.AppendVertex(Point{20, 20});
  mesh.AppendVertex(Point{30, 20});
  mesh.AppendVertex(Point{20, 30});

  mesh.AppendTriangleIndices({0, 1, 2});
  mesh.AppendTriangleIndices({3, 4, 5});

  absl::StatusOr<PartitionedMesh> pm = PartitionedMesh::FromMutableMesh(mesh);
  ASSERT_THAT(pm, IsOk());

  MutableMesh eraser(MeshFormat{});
  eraser.AppendVertex(Point{-5, -5});
  eraser.AppendVertex(Point{15, -5});
  eraser.AppendVertex(Point{-5, 15});
  eraser.AppendTriangleIndices({0, 1, 2});

  absl::StatusOr<PartitionedMesh> eraser_pm =
      PartitionedMesh::FromMutableMesh(eraser);
  ASSERT_THAT(eraser_pm, IsOk());

  absl::StatusOr<PartitionedMesh> result =
      Subtract(*pm, AffineTransform::Identity(), *eraser_pm,
               AffineTransform::Identity());

  ASSERT_THAT(result, IsOk());
  EXPECT_EQ(result->RenderGroupCount(), 1);

  uint32_t total_triangles = 0;
  uint32_t total_vertices = 0;
  for (const auto& m : result->RenderGroupMeshes(0)) {
    total_triangles += m.TriangleCount();
    total_vertices += m.VertexCount();
  }
  EXPECT_EQ(total_triangles, 1);
  EXPECT_EQ(total_vertices, 3);
}

}  // namespace
}  // namespace ink::strokes_internal
