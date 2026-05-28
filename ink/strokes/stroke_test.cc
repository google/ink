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

#include "ink/strokes/stroke.h"

#include <optional>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/brush/type_matchers.h"
#include "ink/color/color.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/mesh_test_helpers.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/fuzz_domains.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/input/type_matchers.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;

constexpr absl::string_view kTestTextureId = "test-texture";

Brush CreateBrush() {
  auto family = BrushFamily::Create(
      {.scale = {0.5, 0.7}, .corner_rounding = 0, .rotation = kFullTurn / 8},
      {.texture_layers = {BrushPaint::StampingTexture{
           .client_texture_id = std::string(kTestTextureId),
           .blend_mode = BrushPaint::BlendMode::kSrcAtop}}},
      BrushFamily::DefaultInputModel(),
      {.client_brush_family_id =
           "//test/brush-family:awesome-rectangular-brush"});
  ABSL_CHECK_OK(family);
  Color color;
  float brush_size = 10;
  float brush_epsilon = 0.01;
  auto brush = Brush::Create(*family, color, brush_size, brush_epsilon);
  ABSL_CHECK_OK(brush);
  return *brush;
}

StrokeInputBatch CreateFilledInputs() {
  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.tool_type = StrokeInput::ToolType::kStylus,
                                 .position = {10, 3},
                                 .elapsed_time = Duration32::Seconds(0),
                                 .pressure = StrokeInput::kNoPressure,
                                 .tilt = StrokeInput::kNoTilt,
                                 .orientation = StrokeInput::kNoOrientation},
                                {.tool_type = StrokeInput::ToolType::kStylus,
                                 .position = {11, 5},
                                 .elapsed_time = Duration32::Seconds(1),
                                 .pressure = StrokeInput::kNoPressure,
                                 .tilt = StrokeInput::kNoTilt,
                                 .orientation = StrokeInput::kNoOrientation},
                                {.tool_type = StrokeInput::ToolType::kStylus,
                                 .position = {12, 2},
                                 .elapsed_time = Duration32::Seconds(2),
                                 .pressure = StrokeInput::kNoPressure,
                                 .tilt = StrokeInput::kNoTilt,
                                 .orientation = StrokeInput::kNoOrientation}});
  ABSL_CHECK_OK(inputs);
  return *inputs;
}

StrokeInputBatch CreateEmptyInputs() { return StrokeInputBatch(); }

PartitionedMesh CreateFilledShape() {
  auto shape = PartitionedMesh::FromMutableMesh(
      MakeStraightLineMutableMesh(18, MakeSinglePackedPositionFormat()),
      {{1, 5, 4, 0}, {5, 9, 4}});
  ABSL_CHECK_OK(shape);
  return *shape;
}

TEST(StrokeTest, ConstructFromBrush) {
  Brush brush = CreateBrush();
  Stroke stroke(brush);

  EXPECT_EQ(brush.CoatCount(), 1u);
  ASSERT_EQ(stroke.GetShape().RenderGroupCount(), 1u);
  EXPECT_THAT(stroke.GetShape().RenderGroupMeshes(0), IsEmpty());
  EXPECT_TRUE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, ConstructFromBrushAndInputs) {
  Brush brush = CreateBrush();
  StrokeInputBatch inputs = CreateFilledInputs();
  Stroke stroke(brush, inputs);

  EXPECT_EQ(stroke.GetShape().Meshes().size(), 1u);
  EXPECT_THAT(
      stroke.GetShape().Bounds(),
      EnvelopeNear(Rect::FromTwoPoints({5.757, -2.243}, {16.243, 9.230}),
                   0.001));

  // Modifying the original brush and inputs should not modify the Stroke since
  // it has its own copies.
  ASSERT_THAT(brush.SetSize(27), IsOk());
  ASSERT_THAT(inputs.Append({.tool_type = StrokeInput::ToolType::kStylus,
                             .position = {4, 5},
                             .elapsed_time = Duration32::Seconds(3)}),
              IsOk());

  EXPECT_EQ(stroke.GetBrush().GetSize(), 10u);
  EXPECT_EQ(stroke.GetInputs().Size(), 3u);
}

TEST(StrokeTest, ConstructFromBrushAndInputsAndShape) {
  Brush brush = CreateBrush();
  StrokeInputBatch inputs = CreateFilledInputs();
  PartitionedMesh shape = CreateFilledShape();
  Stroke stroke(brush, inputs, shape);

  // The same `Mesh` instances are shared rather than copied.
  EXPECT_THAT(stroke.GetShape(), PartitionedMeshShallowEq(shape));

  // Modifying the original brush and inputs should not modify the Stroke since
  // it has its own copies.
  ASSERT_THAT(brush.SetSize(27), IsOk());
  ASSERT_THAT(inputs.Append({.tool_type = StrokeInput::ToolType::kStylus,
                             .position = {4, 5},
                             .elapsed_time = Duration32::Seconds(3)}),
              IsOk());

  EXPECT_EQ(stroke.GetBrush().GetSize(), 10u);
  EXPECT_EQ(stroke.GetInputs().Size(), 3u);
  EXPECT_THAT(stroke.GetShape(), PartitionedMeshDeepEq(CreateFilledShape()));
}

TEST(StrokeTest, GetBounds) {
  Brush brush = CreateBrush();
  StrokeInputBatch inputs = CreateFilledInputs();
  PartitionedMesh shape = CreateFilledShape();
  Stroke stroke(brush, inputs, shape);

  EXPECT_THAT(stroke.GetShape().Bounds(),
              EnvelopeEq(Rect::FromTwoPoints({0, -1}, {19, 0})));
}

TEST(StrokeTest, GetBoundsWithEmptyMesh) {
  Brush brush = CreateBrush();
  StrokeInputBatch empty_inputs = CreateEmptyInputs();
  Stroke stroke(brush, empty_inputs);

  EXPECT_EQ(stroke.GetShape().Meshes().size(), 0u);
  EXPECT_TRUE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, GetBrush) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateEmptyInputs());
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
}

TEST(StrokeTest, SetBrushColor) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateEmptyInputs());
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));

  stroke.SetBrushColor(Color::Blue());
  EXPECT_EQ(stroke.GetBrush().GetColor(), Color::Blue());
}

TEST(StrokeTest, SetValidBrushSize) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  auto mbr_before = stroke.GetShape().Bounds().AsRect();
  ASSERT_TRUE(mbr_before.has_value());
  EXPECT_EQ(stroke.GetBrush().GetSize(), 12u);
  EXPECT_THAT(stroke.SetBrushSize(80), IsOk());
  EXPECT_EQ(stroke.GetBrush().GetSize(), 80u);
  auto mbr_after = stroke.GetShape().Bounds().AsRect();
  EXPECT_TRUE(mbr_after.has_value());
  EXPECT_THAT(*mbr_before, Not(RectEq(*mbr_after)));
}

TEST(StrokeTest, SetInvalidBrushSize) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_EQ(stroke.GetBrush().GetSize(), 12);
  EXPECT_THAT(stroke.SetBrushSize(-2),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_EQ(stroke.GetBrush().GetSize(), 12);
}

TEST(StrokeTest, SetBrushSizeToSameValueDoesNotRegenerateShape) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  PartitionedMesh shape = stroke.GetShape();

  // Setting to the same size should not regenerate the shape.
  EXPECT_THAT(stroke.SetBrushSize(12), IsOk());
  EXPECT_THAT(stroke.GetShape(), PartitionedMeshShallowEq(shape));
}

TEST(StrokeTest, SetBrushEpsilonChangesEpsilonAndShape) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1.25);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_EQ(stroke.GetBrush().GetEpsilon(), 1.25);
  PartitionedMesh shape = stroke.GetShape();

  EXPECT_THAT(stroke.SetBrushEpsilon(0.5), IsOk());
  EXPECT_EQ(stroke.GetBrush().GetEpsilon(), 0.5);
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshShallowEq(shape)));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshDeepEq(shape)));
}

TEST(StrokeTest, SetBrushEpsilonToInvalidValueReturnsError) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1.25);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_EQ(stroke.GetBrush().GetEpsilon(), 1.25);

  EXPECT_THAT(stroke.SetBrushEpsilon(-2),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_EQ(stroke.GetBrush().GetEpsilon(), 1.25);
}

TEST(StrokeTest, SetBrushEpsilonToSameValueDoesNotRegenerateShape) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1.25);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  PartitionedMesh shape = stroke.GetShape();

  // Setting to the same epsilon should not regenerate the shape.
  EXPECT_THAT(stroke.SetBrushEpsilon(1.25), IsOk());
  EXPECT_THAT(stroke.GetShape(), PartitionedMeshShallowEq(shape));
}

TEST(StrokeTest, ConstructorBrushOnly) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
  EXPECT_EQ(stroke.GetInputs().Size(), 0u);
  EXPECT_TRUE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, ConstructorBrushStrokeInputBatch) {
  auto brush = Brush::Create({}, Color::Red(), 12, 1);
  auto inputs = CreateFilledInputs();
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, inputs);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
  EXPECT_EQ(stroke.GetInputs().Size(), inputs.Size());
  ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
  EXPECT_EQ(stroke.GetShape().Meshes()[0].VertexCount(), 11u);
  EXPECT_FALSE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, ConstructorBrushStrokeInputBatchSize1) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.tool_type = StrokeInput::ToolType::kStylus,
                                 .position = {-1.0f, -1.0f},
                                 .elapsed_time = Duration32::Seconds(5),
                                 .pressure = -1}});
  ASSERT_THAT(inputs, IsOk());
  Stroke stroke(*brush, *inputs);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
  EXPECT_EQ(stroke.GetInputs().Size(), inputs->Size());
  ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
  EXPECT_EQ(stroke.GetShape().Meshes()[0].VertexCount(), 6u);
  EXPECT_FALSE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, ConstructorBrushEmptyStrokeInputBatch) {
  absl::StatusOr<Brush> brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, StrokeInputBatch());
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
  EXPECT_EQ(stroke.GetInputs().Size(), 0u);
  EXPECT_EQ(stroke.GetShape().Meshes().size(), 0u);
  EXPECT_TRUE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, ConstructorBrushStrokeInputBatchShape) {
  PartitionedMesh shape = CreateFilledShape();
  StrokeInputBatch inputs = CreateFilledInputs();
  Brush brush = CreateBrush();

  Stroke stroke(brush, inputs, shape);

  PartitionedMesh shape_out = stroke.GetShape();
  StrokeInputBatch inputs_out = stroke.GetInputs();
  EXPECT_EQ(shape_out.Meshes().size(), 1u);
  ASSERT_EQ(shape_out.RenderGroupCount(), 1u);
  EXPECT_EQ(shape_out.OutlineCount(0), 2u);
  EXPECT_EQ(inputs_out.Size(), inputs.Size());
  EXPECT_THAT(inputs_out.First().position, PointEq(inputs.First().position));
  EXPECT_THAT(stroke.GetBrush(), BrushEq(brush));
}

TEST(StrokeTest, ConstructorBrushEmptyStrokeInputBatchShape) {
  Brush brush = CreateBrush();

  Stroke stroke(
      brush, StrokeInputBatch(),
      *PartitionedMesh::FromMeshGroups({PartitionedMesh::MeshGroup{}}));

  PartitionedMesh shape_out = stroke.GetShape();
  StrokeInputBatch inputs_out = stroke.GetInputs();
  ASSERT_EQ(shape_out.RenderGroupCount(), 1u);
  EXPECT_THAT(shape_out.RenderGroupMeshes(0), IsEmpty());
  EXPECT_EQ(inputs_out.Size(), 0u);
  EXPECT_TRUE(shape_out.Bounds().IsEmpty());
  EXPECT_THAT(stroke.GetBrush(), BrushEq(brush));
}

TEST(StrokeTest, CopyAndMove) {
  {
    Stroke stroke(CreateBrush(), CreateFilledInputs(), CreateFilledShape());

    Stroke copied_stroke = stroke;
    EXPECT_THAT(copied_stroke.GetBrush(), BrushEq(stroke.GetBrush()));
    EXPECT_THAT(copied_stroke.GetInputs(),
                StrokeInputBatchEq(stroke.GetInputs()));
  }
  {
    auto brush = Brush::Create({}, Color::Red(), 12, 1);
    Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());

    Stroke copied_stroke(CreateBrush());
    copied_stroke = stroke;
    EXPECT_THAT(copied_stroke.GetBrush(), BrushEq(stroke.GetBrush()));
    EXPECT_THAT(copied_stroke.GetInputs(),
                StrokeInputBatchEq(stroke.GetInputs()));
  }
  {
    Stroke stroke(CreateBrush(), CreateFilledInputs(), CreateFilledShape());

    Stroke copied_stroke = stroke;
    Stroke moved_stroke = std::move(stroke);
    EXPECT_THAT(moved_stroke.GetBrush(), BrushEq(copied_stroke.GetBrush()));
    EXPECT_THAT(moved_stroke.GetInputs(),
                StrokeInputBatchEq(copied_stroke.GetInputs()));
  }
  {
    auto brush = Brush::Create({}, Color::Red(), 12, 1);
    Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());

    Stroke copied_stroke = stroke;
    Stroke moved_stroke(CreateBrush());
    moved_stroke = std::move(stroke);
    EXPECT_THAT(moved_stroke.GetBrush(), BrushEq(copied_stroke.GetBrush()));
    EXPECT_THAT(moved_stroke.GetInputs(),
                StrokeInputBatchEq(copied_stroke.GetInputs()));
  }
}

TEST(StrokeTest, SetBrushAndInputs) {
  Brush brush;
  Stroke stroke(brush);

  ASSERT_THAT(stroke.GetBrush(), BrushEq(brush));
  ASSERT_EQ(stroke.GetInputs().Size(), 0u);
  EXPECT_EQ(stroke.GetShape().Meshes().size(), 0u);
  ASSERT_TRUE(stroke.GetShape().Bounds().IsEmpty());

  auto set_brush = Brush::Create({}, Color::Red(), 12, 1);
  StrokeInputBatch inputs = CreateFilledInputs();

  stroke.SetBrushAndInputs(*set_brush, inputs);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*set_brush));
  EXPECT_EQ(stroke.GetInputs().Size(), inputs.Size());
  ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
  EXPECT_EQ(stroke.GetShape().Meshes()[0].VertexCount(), 11u);
  ASSERT_FALSE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, SetInputGetInputs) {
  Stroke stroke(CreateBrush());
  {
    ASSERT_EQ(stroke.GetInputs().Size(), 0u);
    StrokeInputBatch inputs = CreateFilledInputs();
    stroke.SetInputs(inputs);
    ASSERT_EQ(stroke.GetInputs().Size(), inputs.Size());
    ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
    EXPECT_EQ(stroke.GetShape().Meshes()[0].VertexCount(), 16u);
    ASSERT_FALSE(stroke.GetShape().Bounds().IsEmpty());
  }
  {
    ASSERT_GT(stroke.GetInputs().Size(), 0u);
    stroke.SetInputs(StrokeInputBatch());
    EXPECT_EQ(stroke.GetInputs().Size(), 0u);
    EXPECT_EQ(stroke.GetShape().Meshes().size(), 0u);
    ASSERT_TRUE(stroke.GetShape().Bounds().IsEmpty());
  }
}

TEST(StrokeTest, SetBrushDifferentColor) {
  auto brush = Brush::Create({}, Color::Black(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  ASSERT_THAT(stroke.GetBrush(), BrushEq(*brush));
  PartitionedMesh shape = stroke.GetShape();

  // Color change should change the brush but not regenerate the shape.
  auto new_brush = Brush::Create({}, Color::Red(), 12, 1);
  ASSERT_THAT(new_brush, IsOk());
  stroke.SetBrush(*new_brush);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*new_brush));
  EXPECT_THAT(stroke.GetShape(), PartitionedMeshShallowEq(shape));
}

TEST(StrokeTest, SetBrushDifferentSize) {
  auto brush = Brush::Create({}, Color::Black(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  ASSERT_THAT(stroke.GetBrush(), BrushEq(*brush));
  PartitionedMesh shape = stroke.GetShape();
  ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
  ASSERT_GT(shape.Meshes()[0].VertexCount(), 0u);

  // Changing brush size should change the brush and regenerate the shape.
  auto new_brush = Brush::Create({}, Color::Red(), 5, 1);
  ASSERT_THAT(new_brush, IsOk());
  stroke.SetBrush(*new_brush);
  ASSERT_THAT(stroke.GetBrush(), BrushEq(*new_brush));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshShallowEq(shape)));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshDeepEq(shape)));

  ASSERT_THAT(stroke.GetShape().Meshes(), SizeIs(Ge(1)));
  EXPECT_GT(stroke.GetShape().Meshes()[0].VertexCount(), 0u);
  ASSERT_EQ(stroke.GetShape().RenderGroupCount(), 1u);
  ASSERT_GT(stroke.GetShape().OutlineCount(0), 0u);
  EXPECT_GT(stroke.GetShape().Outline(0, 0).size(), 0u);
  EXPECT_FALSE(stroke.GetShape().Bounds().IsEmpty());
}

TEST(StrokeTest, SetBrushDifferentEpsilon) {
  auto brush = Brush::Create({}, Color::Black(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  ASSERT_THAT(stroke.GetBrush(), BrushEq(*brush));
  PartitionedMesh shape = stroke.GetShape();

  // Changing brush epsilon should change the brush and regenerate the shape.
  auto new_brush = Brush::Create({}, Color::Red(), 12, 0.6);
  ASSERT_THAT(new_brush, IsOk());
  stroke.SetBrush(*new_brush);
  ASSERT_THAT(stroke.GetBrush(), BrushEq(*new_brush));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshShallowEq(shape)));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshDeepEq(shape)));
}

TEST(StrokeTest, SetBrushWithDifferentTipRegeneratesShape) {
  absl::StatusOr<BrushFamily> brush_family =
      BrushFamily::Create({.scale = {1, 0.5}}, {});
  ASSERT_THAT(brush_family, IsOk());
  absl::StatusOr<Brush> brush =
      Brush::Create(*brush_family, Color::Black(), 12, 0.6);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*brush));
  PartitionedMesh shape = stroke.GetShape();

  // Changing brush tip should change the brush and regenerate the shape.
  absl::StatusOr<BrushFamily> new_family =
      BrushFamily::Create({.scale = {0.3, 1}}, {});
  ASSERT_THAT(new_family, IsOk());
  absl::StatusOr<Brush> new_brush =
      Brush::Create(*new_family, Color::Black(), 12, 0.6);
  ASSERT_THAT(new_brush, IsOk());
  stroke.SetBrush(*new_brush);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(*new_brush));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshShallowEq(shape)));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshDeepEq(shape)));
}

TEST(StrokeTest, GetBrushFamily) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{}, BrushPaint{}, BrushFamily::DefaultInputModel(),
      {.client_brush_family_id = "ink://ink/brush-family:highlighter:1"});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());

  EXPECT_THAT(stroke.GetBrushFamily(), BrushFamilyEq(*family));
}

TEST(StrokeTest, SetBrushFamily) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{}, BrushPaint{}, BrushFamily::DefaultInputModel(),
      {.client_brush_family_id = "ink://ink/brush-family:highlighter:1"});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_THAT(stroke.GetBrushFamily(), BrushFamilyEq(*family));

  absl::StatusOr<BrushFamily> new_family = BrushFamily::Create(
      BrushTip{}, BrushPaint{}, BrushFamily::DefaultInputModel(),
      {.client_brush_family_id = "ink://ink/brush-family:marker"});
  ASSERT_THAT(new_family, IsOk());
  stroke.SetBrushFamily(*new_family);
  EXPECT_THAT(stroke.GetBrushFamily(), Not(BrushFamilyEq(*family)));
  EXPECT_THAT(stroke.GetBrushFamily(), BrushFamilyEq(*new_family));
}

TEST(StrokeTest, SetBrushFamilyWithDifferentTipRegeneratesShape) {
  absl::StatusOr<BrushFamily> family =
      BrushFamily::Create({.scale = {1, 0.5}}, {});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Red(), 12, 1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs(), CreateFilledShape());
  EXPECT_THAT(stroke.GetBrushFamily(), BrushFamilyEq(*family));
  PartitionedMesh shape = stroke.GetShape();

  // Changing brush tip should change the brush and regenerate the shape.
  absl::StatusOr<BrushFamily> new_family =
      BrushFamily::Create({.scale = {0.3, 1}}, {});
  ASSERT_THAT(new_family, IsOk());
  stroke.SetBrushFamily(*new_family);
  EXPECT_THAT(stroke.GetBrushFamily(), BrushFamilyEq(*new_family));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshShallowEq(shape)));
  EXPECT_THAT(stroke.GetShape(), Not(PartitionedMeshDeepEq(shape)));
}

TEST(StrokeTest, GetInputDurationEmptyStroke) {
  Brush brush = CreateBrush();
  Stroke stroke(brush);

  EXPECT_EQ(stroke.GetInputDuration(), Duration32::Zero());
}

TEST(StrokeTest, GetInputDuration) {
  StrokeInputBatch batch = CreateFilledInputs();
  Stroke stroke(CreateBrush(), batch);

  EXPECT_EQ(stroke.GetInputDuration(), Duration32::Seconds(2));
}

TEST(StrokeDeathTest, ConstructFromMismatchedShapeAndBrush) {
  BrushCoat coat = BrushCoat{.tip = BrushTip()};
  absl::StatusOr<BrushFamily> family = BrushFamily::Create({coat, coat});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::White(), 10, 0.1);
  ASSERT_THAT(brush, IsOk());
  StrokeInputBatch inputs = CreateFilledInputs();
  PartitionedMesh shape = CreateFilledShape();

  EXPECT_EQ(brush->CoatCount(), 2);
  EXPECT_EQ(shape.RenderGroupCount(), 1);
  EXPECT_DEATH_IF_SUPPORTED(Stroke(*brush, inputs, shape),
                            "one render group per brush coat");
}

void CanConstructStrokeFromAnyInputBatch(const Brush& brush,
                                         const StrokeInputBatch& batch) {
  Stroke stroke(brush, batch);
  EXPECT_THAT(stroke.GetBrush(), BrushEq(brush));
  EXPECT_THAT(stroke.GetInputs(), StrokeInputBatchEq(batch));
}
// TODO(b/299275580): Add fuzz tests for stroke mesh generation. This currently
// fails, being unable to create a PartitionedMesh in Stroke::RegenerateShape.
FUZZ_TEST(DISABLED_StrokeTest, CanConstructStrokeFromAnyInputBatch)
    .WithDomains(ValidBrush(), ArbitraryStrokeInputBatch());

TEST(StrokeTest, PartialEraseWithEmptyEraserShapeReturnsStroke) {
  Stroke stroke(CreateBrush(), CreateFilledInputs());
  PartitionedMesh empty_eraser_shape;
  AffineTransform identity = AffineTransform::Identity();

  std::vector<Stroke> result =
      stroke.PartialErase(empty_eraser_shape, identity, identity);

  ASSERT_THAT(result, SizeIs(1));
  EXPECT_THAT(result[0].GetBrush(), BrushEq(stroke.GetBrush()));
  EXPECT_THAT(result[0].GetInputs(), StrokeInputBatchEq(stroke.GetInputs()));
}

TEST(StrokeTest, PartialEraseWithDegenerateTransformReturnsStroke) {
  Stroke stroke(CreateBrush(), CreateFilledInputs());
  PartitionedMesh eraser_shape = CreateFilledShape();
  AffineTransform identity = AffineTransform::Identity();
  AffineTransform degenerate = AffineTransform::Scale(1, 0);

  std::vector<Stroke> result =
      stroke.PartialErase(eraser_shape, identity, degenerate);

  ASSERT_THAT(result, SizeIs(1));
  EXPECT_THAT(result[0].GetBrush(), BrushEq(stroke.GetBrush()));
  EXPECT_THAT(result[0].GetInputs(), StrokeInputBatchEq(stroke.GetInputs()));
  EXPECT_THAT(result[0].GetShape(), PartitionedMeshDeepEq(stroke.GetShape()));
}

TEST(StrokeTest, ParticleBrushWithOneDimensionZero) {
  // Brush which will create a tip geometry with a width that is 0 and is a
  // particle brush, so it will hit the case in
  // ComputeParticleSurfaceUVTransform where we must guard against division by
  // zero.
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.particle_gap_distance_scale = 0.2f,
               .behaviors = {BrushBehavior{
                   {BrushBehavior::SourceNode(
                        BrushBehavior::Source::
                            kDistanceTraveledInMultiplesOfBrushSize,
                        BrushBehavior::OutOfRange::kMirror, {0, 1}),
                    BrushBehavior::TargetNode(
                        BrushBehavior::Target::kWidthMultiplier, {0, 2})}}}},
      {});
  ASSERT_THAT(family, IsOk());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Black(), 1, 0.1);
  ASSERT_THAT(brush, IsOk());
  Stroke stroke(*brush, CreateFilledInputs());
  // If we did divide by zero, RegenerateShape would produce a PartitionedMesh
  // with no meshes.
  EXPECT_THAT(stroke.GetShape().Meshes(), SizeIs(1));
}

}  // namespace
}  // namespace ink
