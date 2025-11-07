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

#include "ink/strokes/in_progress_stroke.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/type_matchers.h"
#include "ink/color/color.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/input/type_matchers.h"
#include "ink/strokes/stroke.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::ink::geometry_internal::CalculateEnvelope;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;
using ::testing::Pointee;
using ::testing::Property;

constexpr absl::string_view kTestTextureId = "test-texture";

Brush CreateRectangularTestBrush() {
  auto family = BrushFamily::Create(
      {
          .scale = {0.5, 0.7},
          .corner_rounding = 0,
          .rotation = kFullTurn / 8,
      },
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {3, 5},
                           .size_jitter = {0.1, 2},
                           .keyframes = {{.progress = 0.1,
                                          .rotation = kFullTurn / 8}},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn}}},
      "//test/brush-family:awesome-rectangular-brush",
      BrushFamily::SpringModel{});
  ABSL_CHECK_OK(family);
  Color color;
  float brush_size = 10;
  float brush_epsilon = 0.01;
  auto brush = Brush::Create(*family, color, brush_size, brush_epsilon);
  ABSL_CHECK_OK(brush);
  return *brush;
}

Brush CreateCircularTestBrush() {
  auto family = BrushFamily::Create(
      {
          .scale = {0.75, 0.75},
          .corner_rounding = 1,
      },
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {3, 5},
                           .keyframes = {{.progress = 0.1,
                                          .rotation = kFullTurn / 8}},
                           .blend_mode = BrushPaint::BlendMode::kSrcAtop}}},
      "//test/brush-family:awesome-circular-brush", BrushFamily::SpringModel{});
  ABSL_CHECK_OK(family);
  Color color;
  float brush_size = 5;
  float brush_epsilon = 0.01;
  auto brush = Brush::Create(*family, color, brush_size, brush_epsilon);
  ABSL_CHECK_OK(brush);
  return *brush;
}

std::vector<MeshFormat::AttributeId> GetAttributeIds(const MeshFormat& format) {
  std::vector<MeshFormat::AttributeId> ids;
  ids.reserve(format.Attributes().size());
  for (MeshFormat::Attribute attribute : format.Attributes()) {
    ids.push_back(attribute.id);
  }
  return ids;
}

MATCHER_P(IsFailedPreconditionErrorThat, message_matcher, "") {
  return ExplainMatchResult(
      AllOf(Property("code", &absl::Status::code,
                     Eq(absl::StatusCode::kFailedPrecondition)),
            Property("message", &absl::Status::message, message_matcher)),
      arg, result_listener);
}

MATCHER_P(IsInvalidArgumentErrorThat, message_matcher, "") {
  return ExplainMatchResult(
      AllOf(Property("code", &absl::Status::code,
                     Eq(absl::StatusCode::kInvalidArgument)),
            Property("message", &absl::Status::message, message_matcher)),
      arg, result_listener);
}

MATCHER_P3(UpdateShapeFailsAndDoesNotModifyStroke, current_elapsed_time,
           expected_status_code, error_message_matcher, "") {
  const Brush* brush_before_update = arg->GetBrush();
  StrokeInputBatch inputs_before_update = arg->GetInputs();
  uint32_t coat_count_before_update = arg->BrushCoatCount();
  std::vector<MutableMesh> meshes_before_update;
  meshes_before_update.reserve(coat_count_before_update);
  std::vector<Envelope> bounds_before_update;
  bounds_before_update.reserve(coat_count_before_update);
  std::vector<std::vector<std::vector<uint32_t>>> index_outlines_before_update;
  index_outlines_before_update.reserve(coat_count_before_update);
  for (uint32_t i = 0; i < coat_count_before_update; ++i) {
    meshes_before_update.push_back(arg->GetMesh(i).Clone());
    bounds_before_update.push_back(arg->GetMeshBounds(i));
    std::vector<std::vector<uint32_t>> outlines;
    for (absl::Span<const uint32_t> outline : arg->GetCoatOutlines(i)) {
      outlines.push_back(std::vector<uint32_t>(outline.begin(), outline.end()));
    }
    index_outlines_before_update.push_back(std::move(outlines));
  }

  absl::Status status = arg->UpdateShape(current_elapsed_time);

  Envelope updated_region_before_update = arg->GetUpdatedRegion();
  if (!ExplainMatchResult(Eq(expected_status_code), status.code(),
                          result_listener) ||
      !ExplainMatchResult(error_message_matcher, status.message(),
                          result_listener)) {
    return false;
  }

  if (!ExplainMatchResult(Eq(coat_count_before_update), arg->BrushCoatCount(),
                          result_listener)) {
    return false;
  }
  for (uint32_t i = 0; i < coat_count_before_update; ++i) {
    if (!ExplainMatchResult(
            AllOf(Property(&MutableMesh::RawVertexData,
                           ElementsAreArray(
                               meshes_before_update[i].RawVertexData())),
                  Property(&MutableMesh::RawIndexData,
                           ElementsAreArray(
                               meshes_before_update[i].RawIndexData()))),
            arg->GetMesh(i), result_listener) ||
        !ExplainMatchResult(EnvelopeEq(bounds_before_update[i]),
                            arg->GetMeshBounds(i), result_listener) ||
        !ExplainMatchResult(ElementsAreArray(index_outlines_before_update[i]),
                            arg->GetCoatOutlines(i), result_listener)) {
      return false;
    }
  }

  return ExplainMatchResult(
      AllOf(
          (brush_before_update == nullptr
               ? Property(&InProgressStroke::GetBrush, Eq(nullptr))
               : Property(&InProgressStroke::GetBrush,
                          Pointee(BrushEq(*brush_before_update)))),
          Property(&InProgressStroke::GetInputs,
                   StrokeInputBatchEq(inputs_before_update)),
          Property(
              &InProgressStroke::GetUpdatedRegion,
              (updated_region_before_update.IsEmpty()
                   ? Property(&Envelope::AsRect, Eq(std::nullopt))
                   : Property(&Envelope::AsRect,
                              Optional(RectEq(
                                  *updated_region_before_update.AsRect())))))),
      *arg, result_listener);
}

TEST(InProgressStrokeTest, DefaultConstructed) {
  InProgressStroke stroke;

  EXPECT_EQ(stroke.GetBrush(), nullptr);
  EXPECT_TRUE(stroke.GetInputs().IsEmpty());
  EXPECT_EQ(stroke.BrushCoatCount(), 0u);
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
  EXPECT_TRUE(stroke.InputsAreFinished());
  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, MoveConstructedAndAssigned) {
  Brush brush = CreateRectangularTestBrush();
  InProgressStroke stroke;
  stroke.Start(brush);
  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      {.position = {1, 2}},
      {.position = {3, 4}},
  });
  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create({
      {.position = {5, 6}},
  });
  ASSERT_EQ(inputs.status(), absl::OkStatus());
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());
  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*inputs, *predicted_inputs));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));
  EXPECT_EQ(stroke.InputCount(), 3);
  EXPECT_EQ(stroke.RealInputCount(), 2);
  EXPECT_EQ(stroke.PredictedInputCount(), 1);

  // `stroke` is in an indeterminate state and should not be used after move.
  InProgressStroke stroke2(std::move(stroke));
  EXPECT_EQ(stroke2.InputCount(), 3);
  EXPECT_EQ(stroke2.RealInputCount(), 2);
  EXPECT_EQ(stroke2.PredictedInputCount(), 1);

  // `stroke2` is in an indeterminate state and should not be used after move.
  InProgressStroke stroke3 = std::move(stroke2);
  EXPECT_EQ(stroke3.InputCount(), 3);
  EXPECT_EQ(stroke3.RealInputCount(), 2);
  EXPECT_EQ(stroke3.PredictedInputCount(), 1);
}

TEST(InProgressStrokeTest, StartAfterConstruction) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();
  stroke.Start(brush);

  EXPECT_THAT(stroke.GetBrush(), Pointee(BrushEq(brush)));
  EXPECT_TRUE(stroke.GetInputs().IsEmpty());
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_EQ(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_EQ(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_TRUE(stroke.GetMeshBounds(0).IsEmpty());
  EXPECT_THAT(stroke.GetCoatOutlines(0), IsEmpty());
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
  EXPECT_FALSE(stroke.InputsAreFinished());
  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, ClearAfterStart) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();
  stroke.Start(brush);
  stroke.Clear();

  EXPECT_EQ(stroke.GetBrush(), nullptr);
  EXPECT_TRUE(stroke.GetInputs().IsEmpty());
  EXPECT_EQ(stroke.BrushCoatCount(), 0u);
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
  EXPECT_TRUE(stroke.InputsAreFinished());
  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, EnqueueInputsWithoutStart) {
  InProgressStroke stroke;
  EXPECT_THAT(stroke.EnqueueInputs({}, {}),
              IsFailedPreconditionErrorThat(HasSubstr("Start")));
}

TEST(InProgressStrokeTest, UpdateShapeWithoutStart) {
  InProgressStroke stroke;
  EXPECT_THAT(stroke.UpdateShape(Duration32::Zero()),
              IsFailedPreconditionErrorThat(HasSubstr("Start")));
}

TEST(InProgressStrokeTest, EnqueueInputsAfterFinishInputs) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());
  stroke.FinishInputs();
  EXPECT_TRUE(stroke.InputsAreFinished());
  EXPECT_THAT(stroke.EnqueueInputs({}, {}),
              IsFailedPreconditionErrorThat(HasSubstr("FinishInputs")));
}

TEST(InProgressStrokeTest, EmptyEnqueueInputsDoesNotNeedUpdate) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());

  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, {}));

  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, NonEmptyEnqueueInputsNeedsUpdate) {
  InProgressStroke stroke;
  stroke.Start(CreateCircularTestBrush());

  absl::StatusOr<StrokeInputBatch> real_inputs = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
  });
  ASSERT_EQ(absl::OkStatus(), real_inputs.status());
  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs, {}));

  EXPECT_TRUE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, EmptyEnqueueInputsAndUpdateAfterStart) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());

  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, {}));
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));

  EXPECT_TRUE(stroke.GetInputs().IsEmpty());
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_EQ(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_EQ(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_TRUE(stroke.GetMeshBounds(0).IsEmpty());
  EXPECT_THAT(stroke.GetCoatOutlines(0), IsEmpty());
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, EnqueueInputsPredictionOnlyAndUpdate) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());

  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
  });
  ASSERT_EQ(absl::OkStatus(), predicted_inputs.status());
  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *predicted_inputs));

  EXPECT_TRUE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());

  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));

  EXPECT_FALSE(stroke.GetInputs().IsEmpty());
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_GT(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_FALSE(stroke.GetUpdatedRegion().IsEmpty());
  EXPECT_FALSE(stroke.NeedsUpdate());
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, NonEmptyInputs) {
  InProgressStroke stroke;
  stroke.Start(CreateCircularTestBrush());

  absl::StatusOr<StrokeInputBatch> real_inputs_0 = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
      {.position = {3, 2}, .elapsed_time = Duration32::Seconds(0.1)},
  });
  ASSERT_EQ(real_inputs_0.status(), absl::OkStatus());
  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create(
      {{.position = {3, 4}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());

  EXPECT_EQ(absl::OkStatus(),
            stroke.EnqueueInputs(*real_inputs_0, *predicted_inputs));
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.15)));

  StrokeInputBatch combined_inputs = *real_inputs_0;
  ASSERT_EQ(absl::OkStatus(), combined_inputs.Append(*predicted_inputs));

  EXPECT_THAT(stroke.GetInputs(), StrokeInputBatchEq(combined_inputs));
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_NE(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_NE(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_THAT(stroke.GetMeshBounds(0).AsRect(),
              Optional(RectNear(*CalculateEnvelope(stroke.GetMesh(0)).AsRect(),
                                0.0001)));

  EXPECT_THAT(stroke.GetCoatOutlines(0), ElementsAre(Not(IsEmpty())));
  EXPECT_THAT(stroke.GetUpdatedRegion().AsRect(),
              Optional(RectNear(
                  Rect::FromTwoPoints({-0.88, 0.13}, {4.90, 5.87}), 0.01)));

  absl::StatusOr<StrokeInputBatch> real_inputs_1 = StrokeInputBatch::Create(
      {{.position = {3, 0}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(real_inputs_1.status(), absl::OkStatus());
  predicted_inputs = StrokeInputBatch::Create(
      {{.position = {4, -1}, .elapsed_time = Duration32::Seconds(0.3)}});
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());

  EXPECT_EQ(absl::OkStatus(),
            stroke.EnqueueInputs(*real_inputs_1, *predicted_inputs));
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.2)));

  combined_inputs = *real_inputs_0;
  ASSERT_EQ(absl::OkStatus(), combined_inputs.Append(*real_inputs_1));
  ASSERT_EQ(absl::OkStatus(), combined_inputs.Append(*predicted_inputs));

  EXPECT_THAT(stroke.GetInputs(), StrokeInputBatchEq(combined_inputs));
  EXPECT_NE(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_NE(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_THAT(stroke.GetMeshBounds(0).AsRect(),
              Optional(RectNear(*CalculateEnvelope(stroke.GetMesh(0)).AsRect(),
                                0.0001)));

  EXPECT_THAT(stroke.GetCoatOutlines(0), ElementsAre(Not(IsEmpty())));
  EXPECT_THAT(stroke.GetUpdatedRegion().AsRect(),
              Optional(RectNear(
                  Rect::FromTwoPoints({-0.88, -2.88}, {5.88, 5.87}), 0.01)));
}

TEST(InProgressStrokeTest, ExtendWithEmptyPredictedButNonEmptyReal) {
  InProgressStroke stroke;
  stroke.Start(CreateCircularTestBrush());

  absl::StatusOr<StrokeInputBatch> real_inputs_0 = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
      {.position = {3, 2}, .elapsed_time = Duration32::Seconds(0.1)},
  });
  ASSERT_EQ(real_inputs_0.status(), absl::OkStatus());

  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs_0, {}));
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.15)));

  EXPECT_THAT(stroke.GetInputs(), StrokeInputBatchEq(*real_inputs_0));
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_NE(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_NE(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_THAT(
      stroke.GetMeshBounds(0),
      EnvelopeNear(CalculateEnvelope(stroke.GetMesh(0)).AsRect().value(),
                   0.0001));

  EXPECT_THAT(stroke.GetCoatOutlines(0), ElementsAre(Not(IsEmpty())));
  EXPECT_THAT(stroke.GetUpdatedRegion().AsRect(),
              Optional(RectNear(
                  Rect::FromTwoPoints({-0.88, 0.13}, {4.87, 3.87}), 0.01)));

  absl::StatusOr<StrokeInputBatch> real_inputs_1 = StrokeInputBatch::Create(
      {{.position = {3, 0}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(real_inputs_1.status(), absl::OkStatus());

  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs_1, {}));
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.2)));

  StrokeInputBatch combined_inputs = *real_inputs_0;
  ASSERT_EQ(absl::OkStatus(), combined_inputs.Append(*real_inputs_1));

  EXPECT_THAT(stroke.GetInputs(), StrokeInputBatchEq(combined_inputs));
  EXPECT_NE(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_NE(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_THAT(
      stroke.GetMeshBounds(0),
      EnvelopeNear(CalculateEnvelope(stroke.GetMesh(0)).AsRect().value(),
                   0.0001));

  EXPECT_THAT(stroke.GetCoatOutlines(0), ElementsAre(Not(IsEmpty())));
  EXPECT_THAT(stroke.GetUpdatedRegion().AsRect(),
              Optional(RectNear(
                  Rect::FromTwoPoints({-0.88, -1.87}, {4.90, 3.88}), 0.01)));
}

TEST(InProgressStrokeTest, EnqueueInputsWithDifferentToolTypes) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();
  stroke.Start(brush);

  absl::StatusOr<StrokeInputBatch> mouse_input = StrokeInputBatch::Create(
      {{.tool_type = StrokeInput::ToolType::kMouse, .position = {1, 2}}});
  ASSERT_EQ(mouse_input.status(), absl::OkStatus());
  absl::StatusOr<StrokeInputBatch> touch_input = StrokeInputBatch::Create(
      {{.tool_type = StrokeInput::ToolType::kTouch, .position = {3, 4}}});
  ASSERT_EQ(touch_input.status(), absl::OkStatus());

  // We can't add real mouse inputs while simultanously predicting touch inputs.
  EXPECT_THAT(stroke.EnqueueInputs(*mouse_input, *touch_input),
              IsInvalidArgumentErrorThat(HasSubstr("tool_type")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());

  // We *can* predict touch inputs (with no real inputs so far)...
  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *touch_input));
  EXPECT_TRUE(stroke.NeedsUpdate());  // inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));
  // ...and then actually end up with real mouse inputs (which replace the
  // predicted touch inputs).
  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*mouse_input, {}));
  EXPECT_TRUE(stroke.NeedsUpdate());  // inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));

  // But now that we have real mouse inputs, we can't predict further touch
  // inputs...
  EXPECT_THAT(stroke.EnqueueInputs({}, *touch_input),
              IsInvalidArgumentErrorThat(HasSubstr("tool_type")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
  // ...nor can we add real touch inputs.
  EXPECT_THAT(stroke.EnqueueInputs(*touch_input, {}),
              IsInvalidArgumentErrorThat(HasSubstr("tool_type")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, EnqueuingInputsWithOverlappingTimeIntervals) {
  Brush brush = CreateRectangularTestBrush();

  absl::StatusOr<StrokeInputBatch> first_inputs = StrokeInputBatch::Create(
      {{.position = {1, 2}, .elapsed_time = Duration32::Seconds(0)},
       {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
       {.position = {1, 2}, .elapsed_time = Duration32::Seconds(2)}});
  ASSERT_EQ(first_inputs.status(), absl::OkStatus());

  absl::StatusOr<StrokeInputBatch> second_inputs = StrokeInputBatch::Create(
      {{.position = {3, 4}, .elapsed_time = Duration32::Seconds(1)},
       {.position = {3, 4}, .elapsed_time = Duration32::Seconds(2)},
       {.position = {3, 4}, .elapsed_time = Duration32::Seconds(3)}});
  ASSERT_EQ(second_inputs.status(), absl::OkStatus());

  {
    InProgressStroke stroke;
    stroke.Start(brush);
    // Enqueuing inputs with times that overlap is valid; we drop the new
    // inputs with timestamps earlier than latest previously queued input.
    // Note also that we allow inputs with the same time but different position.
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*first_inputs, {}));
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*second_inputs, {}));
    EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(3)));
    EXPECT_THAT(
        stroke.GetInputs(),
        StrokeInputBatchIsArray(
            {{.position = {1, 2}, .elapsed_time = Duration32::Seconds(0)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(2)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(2)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(3)}}));
  }

  {
    InProgressStroke stroke;
    stroke.Start(brush);
    // Similarly, predictions with overlap is valid; the predictions are
    // overwritten.
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *first_inputs));
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *second_inputs));
    EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(4)));
    EXPECT_THAT(
        stroke.GetInputs(),
        StrokeInputBatchIsArray(
            {{.position = {3, 4}, .elapsed_time = Duration32::Seconds(1)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(2)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(3)}}));
  }

  {
    InProgressStroke stroke;
    stroke.Start(brush);
    // Enqueuing real inputs and subsequently predictions with
    // overlap is valid; we drop the predictions with timestamps earlier than
    // the queued inputs.
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*first_inputs, {}));
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *second_inputs));
    EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(4)));
    EXPECT_THAT(
        stroke.GetInputs(),
        StrokeInputBatchIsArray(
            {{.position = {1, 2}, .elapsed_time = Duration32::Seconds(0)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(2)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(2)},
             {.position = {3, 4}, .elapsed_time = Duration32::Seconds(3)}}));
  }

  {
    InProgressStroke stroke;
    stroke.Start(brush);
    // Enqueuing predictions and subsequently queued real inputs with overlap
    // is valid; the predictions are reset.
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs({}, *second_inputs));
    EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(3)));
    EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*first_inputs, {}));
    EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(4)));
    EXPECT_THAT(
        stroke.GetInputs(),
        StrokeInputBatchIsArray(
            {{.position = {1, 2}, .elapsed_time = Duration32::Seconds(0)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(1)},
             {.position = {1, 2}, .elapsed_time = Duration32::Seconds(2)}}));
  }
}

TEST(InProgressStrokeTest, EnqueueInputsWithDifferentOptionalPropertyFormats) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();
  stroke.Start(brush);

  absl::StatusOr<StrokeInputBatch> pressure_input =
      StrokeInputBatch::Create({{.position = {1, 2}, .pressure = 0.5}});
  ASSERT_EQ(pressure_input.status(), absl::OkStatus());
  absl::StatusOr<StrokeInputBatch> no_pressure_input =
      StrokeInputBatch::Create({{.position = {2, 3}}});
  ASSERT_EQ(no_pressure_input.status(), absl::OkStatus());

  // We can't add real inputs with pressure data while simultanously predicting
  // inputs without pressure data.
  EXPECT_THAT(stroke.EnqueueInputs(*pressure_input, *no_pressure_input),
              IsInvalidArgumentErrorThat(HasSubstr("pressure")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());

  // Add some real inputs with pressure data.
  EXPECT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*pressure_input, {}));
  EXPECT_TRUE(stroke.NeedsUpdate());  // inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
  EXPECT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));

  // Now that we have real inputs with pressure data, we can't predict further
  // inputs without pressure data...
  EXPECT_THAT(stroke.EnqueueInputs({}, *no_pressure_input),
              IsInvalidArgumentErrorThat(HasSubstr("pressure")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
  // ...nor can we add further real inputs without pressure data.
  EXPECT_THAT(stroke.EnqueueInputs(*no_pressure_input, {}),
              IsInvalidArgumentErrorThat(HasSubstr("pressure")));
  EXPECT_FALSE(stroke.NeedsUpdate());  // no inputs were enqueued
  EXPECT_FALSE(stroke.ChangesWithTime());
}

TEST(InProgressStrokeTest, UpdateShapeWithNegativeCurrentElapsedTime) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();
  stroke.Start(brush);

  EXPECT_THAT(&stroke,
              UpdateShapeFailsAndDoesNotModifyStroke(
                  -Duration32::Millis(10), absl::StatusCode::kInvalidArgument,
                  HasSubstr("non-negative")));
}

TEST(InProgressStrokeTest, UpdateShapeWithDecreasingCurrentElapsedTime) {
  InProgressStroke stroke;
  Brush brush = CreateRectangularTestBrush();

  stroke.Start(brush);

  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Millis(25)));
  EXPECT_THAT(&stroke,
              UpdateShapeFailsAndDoesNotModifyStroke(
                  Duration32::Millis(24), absl::StatusCode::kInvalidArgument,
                  HasSubstr("non-decreasing")));
}

TEST(InProgressStrokeTest, ResetUpdateRegionAfterStart) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());
  ASSERT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
  stroke.ResetUpdatedRegion();
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
}

TEST(InProgressStrokeTest, ResetUpdatedRegionAfterExtendingStroke) {
  InProgressStroke stroke;
  stroke.Start(CreateRectangularTestBrush());

  absl::StatusOr<StrokeInputBatch> real_inputs = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
      {.position = {3, 2}, .elapsed_time = Duration32::Seconds(0.1)},
  });
  ASSERT_EQ(real_inputs.status(), absl::OkStatus());
  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create(
      {{.position = {3, 4}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());

  ASSERT_EQ(absl::OkStatus(),
            stroke.EnqueueInputs(*real_inputs, *predicted_inputs));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.15)));

  ASSERT_FALSE(stroke.GetUpdatedRegion().IsEmpty());
  stroke.ResetUpdatedRegion();
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
}

TEST(InProgressStrokeTest, StartAfterExtendingStroke) {
  Brush starting_brush = CreateRectangularTestBrush();
  Brush replacement_brush = CreateCircularTestBrush();
  ASSERT_THAT(replacement_brush, Not(BrushEq(starting_brush)));

  InProgressStroke stroke;
  stroke.Start(starting_brush);

  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      {.position = {1, 2}},
      {.position = {3, 2}},
      {.position = {3, 4}},
  });
  ASSERT_EQ(inputs.status(), absl::OkStatus());
  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*inputs, {}));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));

  ASSERT_THAT(stroke.GetBrush(), Pointee(BrushEq(starting_brush)));
  ASSERT_FALSE(stroke.GetInputs().IsEmpty());
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  ASSERT_NE(stroke.GetMesh(0).VertexCount(), 0u);
  ASSERT_NE(stroke.GetMesh(0).TriangleCount(), 0u);
  ASSERT_FALSE(stroke.GetMeshBounds(0).IsEmpty());
  ASSERT_THAT(stroke.GetCoatOutlines(0), ElementsAre(Not(IsEmpty())));
  ASSERT_FALSE(stroke.GetUpdatedRegion().IsEmpty());

  stroke.Start(replacement_brush);

  EXPECT_THAT(stroke.GetBrush(), Pointee(BrushEq(replacement_brush)));
  EXPECT_TRUE(stroke.GetInputs().IsEmpty());
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_EQ(stroke.GetMesh(0).VertexCount(), 0u);
  EXPECT_EQ(stroke.GetMesh(0).TriangleCount(), 0u);
  EXPECT_TRUE(stroke.GetMeshBounds(0).IsEmpty());
  EXPECT_THAT(stroke.GetCoatOutlines(0), IsEmpty());
  EXPECT_TRUE(stroke.GetUpdatedRegion().IsEmpty());
}

TEST(InProgressStrokeTest, InputCount) {
  Brush brush = CreateRectangularTestBrush();
  InProgressStroke stroke;
  stroke.Start(brush);
  absl::StatusOr<StrokeInputBatch> inputs = StrokeInputBatch::Create({
      {.position = {1, 2}},
      {.position = {3, 2}},
      {.position = {3, 4}},
  });
  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create({
      {.position = {3, 5}},
  });
  ASSERT_EQ(inputs.status(), absl::OkStatus());
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());
  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*inputs, *predicted_inputs));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Zero()));
  ASSERT_EQ(stroke.InputCount(), 4);
  ASSERT_EQ(stroke.RealInputCount(), 3);
  ASSERT_EQ(stroke.PredictedInputCount(), 1);
}

TEST(InProgressStrokeTest, CopyToStroke) {
  InProgressStroke stroke;
  Brush original_brush = CreateCircularTestBrush();
  stroke.Start(original_brush);

  absl::StatusOr<StrokeInputBatch> real_inputs_0 = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
      {.position = {3, 2}, .elapsed_time = Duration32::Seconds(0.1)},
  });
  ASSERT_EQ(real_inputs_0.status(), absl::OkStatus());
  absl::StatusOr<StrokeInputBatch> predicted_inputs = StrokeInputBatch::Create(
      {{.position = {3, 4}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(predicted_inputs.status(), absl::OkStatus());

  ASSERT_EQ(absl::OkStatus(),
            stroke.EnqueueInputs(*real_inputs_0, *predicted_inputs));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.15)));

  StrokeInputBatch all_original_inputs = *real_inputs_0;

  absl::StatusOr<StrokeInputBatch> real_inputs_1 = StrokeInputBatch::Create(
      {{.position = {3, 0}, .elapsed_time = Duration32::Seconds(0.2)}});
  ASSERT_EQ(real_inputs_1.status(), absl::OkStatus());

  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs_1, {}));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.2)));
  ASSERT_EQ(absl::OkStatus(), all_original_inputs.Append(*real_inputs_1));

  Stroke finished_stroke = stroke.CopyToStroke();

  EXPECT_THAT(finished_stroke.GetBrush(), BrushEq(original_brush));
  EXPECT_THAT(finished_stroke.GetInputs(),
              StrokeInputBatchEq(all_original_inputs));
  EXPECT_EQ(finished_stroke.GetShape().Meshes().size(), 1u);
  EXPECT_THAT(
      finished_stroke.GetShape().Bounds(),
      EnvelopeNear(Rect::FromTwoPoints({-0.88, -1.87}, {4.89, 3.88}), 0.01));

  Stroke another_finished_stroke = stroke.CopyToStroke();

  // Another `Stroke` generated with the same inputs should return an
  // equivalent `PartitionedMesh`, but not one that shares the same `Mesh`
  // instances.
  EXPECT_THAT(another_finished_stroke.GetShape(),
              PartitionedMeshDeepEq(finished_stroke.GetShape()));
  EXPECT_THAT(another_finished_stroke.GetShape(),
              Not(PartitionedMeshShallowEq(finished_stroke.GetShape())));

  // Changing the brush and inputs of the `InProgressStroke` should not affect
  // the results of `CopyToStroke`.
  stroke.Start(CreateRectangularTestBrush());
  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs_1, {}));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.1)));

  EXPECT_THAT(finished_stroke.GetBrush(), BrushEq(original_brush));
  EXPECT_THAT(finished_stroke.GetInputs(),
              StrokeInputBatchEq(all_original_inputs));
  EXPECT_EQ(finished_stroke.GetShape().Meshes().size(), 1u);
  EXPECT_THAT(
      finished_stroke.GetShape().Bounds(),
      EnvelopeNear(Rect::FromTwoPoints({-0.88, -1.87}, {4.89, 3.88}), 0.01));
}

TEST(InProgressStrokeTest, CopyToStrokeOmitUnneededAttributes) {
  InProgressStroke stroke;
  Brush original_brush = CreateCircularTestBrush();
  stroke.Start(original_brush);
  absl::StatusOr<StrokeInputBatch> real_inputs = StrokeInputBatch::Create({
      {.position = {1, 2}, .elapsed_time = Duration32::Seconds(0.0)},
      {.position = {3, 2}, .elapsed_time = Duration32::Seconds(0.1)},
  });
  ASSERT_EQ(real_inputs.status(), absl::OkStatus());
  ASSERT_EQ(absl::OkStatus(), stroke.EnqueueInputs(*real_inputs, {}));
  ASSERT_EQ(absl::OkStatus(), stroke.UpdateShape(Duration32::Seconds(0.15)));

  // The full mesh should include a color shift attribute, but since this brush
  // doesn't need that, it should be omitted from the finished stroke if we use
  // `RetainAttributes::kUsedByThisBrush`.
  ASSERT_EQ(stroke.BrushCoatCount(), 1u);
  EXPECT_THAT(GetAttributeIds(stroke.GetMeshFormat(0)),
              Contains(MeshFormat::AttributeId::kColorShiftHsl));
  Stroke finished_stroke =
      stroke.CopyToStroke(InProgressStroke::RetainAttributes::kUsedByThisBrush);
  ASSERT_EQ(finished_stroke.GetShape().RenderGroupCount(), 1u);
  EXPECT_THAT(GetAttributeIds(finished_stroke.GetShape().RenderGroupFormat(0)),
              Not(Contains(MeshFormat::AttributeId::kColorShiftHsl)));

  // The position data should be unaffected by the omission of other attributes.
  EXPECT_THAT(
      finished_stroke.GetShape().Bounds(),
      EnvelopeNear(Rect::FromTwoPoints({-0.875, 0.125}, {4.868, 3.875}), 0.01));
}

}  // namespace
}  // namespace ink
