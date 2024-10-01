// Copyright 2024 Google LLC
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

#include "ink/strokes/internal/stroke_shape_builder.h"

#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {
namespace {

using ::ink::geometry_internal::CalculateEnvelope;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;

TEST(StrokeShapeBuilderTest, DefaultConstructedIsEmpty) {
  StrokeShapeBuilder builder;
  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());
  EXPECT_TRUE(builder.GetIndexOutlines().empty());
}

TEST(StrokeShapeBuilderTest, FirstStartStrokeHasEmptyMeshAndOutline) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 10, 0.1);

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());
  EXPECT_THAT(builder.GetIndexOutlines(), ElementsAre(IsEmpty()));
}

TEST(StrokeShapeBuilderTest, EmptyExtendHasEmptyUpdateMeshAndOutline) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 10, 0.1);

  StrokeShapeUpdate update = builder.ExtendStroke({}, {}, Duration32::Zero());

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());

  EXPECT_TRUE(update.region.IsEmpty());
  EXPECT_THAT(update.first_index_offset, Eq(std::nullopt));
  EXPECT_THAT(update.first_vertex_offset, Eq(std::nullopt));
  EXPECT_THAT(builder.GetIndexOutlines(), ElementsAre(IsEmpty()));
}

TEST(StrokeShapeBuilderTest, NonEmptyExtend) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 10, 0.1);

  absl::StatusOr<StrokeInputBatch> real_inputs = StrokeInputBatch::Create({
      {.position = {5, 7}, .elapsed_time = Duration32::Zero()},
      {.position = {6, 8}, .elapsed_time = Duration32::Seconds(1. / 60)},
      {.position = {7, 9}, .elapsed_time = Duration32::Seconds(2. / 60)},
  });
  ASSERT_EQ(real_inputs.status(), absl::OkStatus());

  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create({
      {.position = {8, 10}, .elapsed_time = Duration32::Seconds(3. / 60)},
  });
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());

  StrokeShapeUpdate update =
      builder.ExtendStroke(*real_inputs, *predicted_inputs, Duration32::Zero());

  EXPECT_NE(builder.GetMesh().VertexCount(), 0);
  EXPECT_NE(builder.GetMesh().TriangleCount(), 0);
  EXPECT_THAT(builder.GetMeshBounds().AsRect(),
              Optional(RectNear(*CalculateEnvelope(builder.GetMesh()).AsRect(),
                                0.0001)));
  EXPECT_FALSE(update.region.IsEmpty());
  // The first update should include the entire mesh:
  EXPECT_THAT(update.first_index_offset, Optional(Eq(0)));
  EXPECT_THAT(update.first_vertex_offset, Optional(Eq(0)));
  EXPECT_THAT(builder.GetIndexOutlines(), ElementsAre(Not(IsEmpty())));

  real_inputs = StrokeInputBatch::Create({
      {.position = {7, 8}, .elapsed_time = Duration32::Seconds(3. / 60)},
  });
  ASSERT_EQ(real_inputs.status(), absl::OkStatus());
  update = builder.ExtendStroke(*real_inputs, {}, Duration32::Zero());

  EXPECT_NE(builder.GetMesh().VertexCount(), 0);
  EXPECT_NE(builder.GetMesh().TriangleCount(), 0);
  EXPECT_THAT(builder.GetMeshBounds().AsRect(),
              Optional(RectNear(*CalculateEnvelope(builder.GetMesh()).AsRect(),
                                0.0001)));
  EXPECT_FALSE(update.region.IsEmpty());
  // The second update should only include a part of the mesh, since some of the
  // mesh should be fixed:
  EXPECT_THAT(update.first_index_offset, Optional(Gt(0)));
  EXPECT_THAT(update.first_vertex_offset, Optional(Gt(0)));
  EXPECT_THAT(builder.GetIndexOutlines(), ElementsAre(Not(IsEmpty())));
}

TEST(StrokeShapeBuilderTest, StartAfterExtendEmptiesMeshAndOutline) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 10, 0.1);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  builder.ExtendStroke(*inputs, {}, Duration32::Zero());
  ASSERT_NE(builder.GetMesh().VertexCount(), 0);
  ASSERT_NE(builder.GetMesh().TriangleCount(), 0);
  ASSERT_THAT(builder.GetMeshBounds().AsRect(),
              Optional(RectNear(*CalculateEnvelope(builder.GetMesh()).AsRect(),
                                0.0001)));
  ASSERT_THAT(builder.GetIndexOutlines(), ElementsAre(Not(IsEmpty())));

  builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 10, 0.1);

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());
  EXPECT_THAT(builder.GetIndexOutlines(), ElementsAre(IsEmpty()));
}

TEST(StrokeShapeBuilderDeathTest, StartWithEmptyBrushTips) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> brush_tips = {};
  EXPECT_DEATH_IF_SUPPORTED(
      builder.StartStroke(BrushFamily::DefaultInputModel(), brush_tips, 1, 0.1),
      "");
}

TEST(StrokeShapeBuilderDeathTest, StartWithMultipleBrushTips) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> brush_tips = {BrushTip(), BrushTip()};
  EXPECT_DEATH_IF_SUPPORTED(
      builder.StartStroke(BrushFamily::DefaultInputModel(), brush_tips, 1, 0.1),
      "");
}

TEST(StrokeShapeBuilderDeathTest, StartWithZeroBrushSize) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  EXPECT_DEATH_IF_SUPPORTED(
      builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 0,
                          0.1),
      "");
}

TEST(StrokeShapeBuilderDeathTest, StartWithZeroBrushEpsilon) {
  StrokeShapeBuilder builder;
  std::vector<BrushTip> default_tips = {BrushTip()};
  EXPECT_DEATH_IF_SUPPORTED(
      builder.StartStroke(BrushFamily::DefaultInputModel(), default_tips, 1, 0),
      "");
}

TEST(StrokeShapeBuilderDeathTest, ExtendWithoutStart) {
  StrokeShapeBuilder builder;
  EXPECT_DEATH_IF_SUPPORTED(builder.ExtendStroke({}, {}, Duration32::Zero()),
                            "");
}

TEST(StrokeShapeBuilderDeathTest, ExtendInputsAfterInputsFinished) {
  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      {.position = {5, 7}, .elapsed_time = Duration32::Zero()},
  });
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  StrokeShapeBuilder builder;
  std::vector<BrushTip> brush_tips = {BrushTip()};
  builder.StartStroke(BrushFamily::DefaultInputModel(), brush_tips, 1, 0.1);
  builder.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.FinishStrokeInputs();

  EXPECT_DEATH_IF_SUPPORTED(
      builder.ExtendStroke(*inputs, {}, Duration32::Zero()),
      "Can't add more inputs");
}

}  // namespace
}  // namespace ink::strokes_internal
