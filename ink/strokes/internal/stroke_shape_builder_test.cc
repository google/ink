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

#include <cstdint>
#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/internal/algorithms.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/strokes/internal/stroke_vertex.h"
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
  EXPECT_TRUE(builder.GetOutlines().empty());
}

TEST(StrokeShapeBuilderTest, FirstStartStrokeHasEmptyMeshAndOutline) {
  StrokeShapeBuilder builder;
  BrushCoat brush_coat;
  builder.StartStroke(brush_coat, 10, 0.1);

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());
  EXPECT_THAT(builder.GetOutlines(), IsEmpty());
}

TEST(StrokeShapeBuilderTest, EmptyExtendHasEmptyUpdateMeshAndOutline) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat;
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  input_modeler.ExtendStroke({}, {}, Duration32::Zero());
  StrokeShapeUpdate update = builder.ExtendStroke(input_modeler);

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());

  EXPECT_TRUE(update.region.IsEmpty());
  EXPECT_THAT(update.first_index_offset, Eq(std::nullopt));
  EXPECT_THAT(update.first_vertex_offset, Eq(std::nullopt));
  EXPECT_THAT(builder.GetOutlines(), IsEmpty());
}

TEST(StrokeShapeBuilderTest, NonEmptyExtend) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat;
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

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

  input_modeler.ExtendStroke(*real_inputs, *predicted_inputs,
                             Duration32::Zero());
  StrokeShapeUpdate update = builder.ExtendStroke(input_modeler);

  EXPECT_NE(builder.GetMesh().VertexCount(), 0);
  EXPECT_NE(builder.GetMesh().TriangleCount(), 0);
  EXPECT_THAT(builder.GetMeshBounds().AsRect(),
              Optional(RectNear(*CalculateEnvelope(builder.GetMesh()).AsRect(),
                                0.0001)));
  EXPECT_FALSE(update.region.IsEmpty());
  // The first update should include the entire mesh:
  EXPECT_THAT(update.first_index_offset, Optional(Eq(0)));
  EXPECT_THAT(update.first_vertex_offset, Optional(Eq(0)));
  EXPECT_THAT(builder.GetOutlines(), ElementsAre(Not(IsEmpty())));

  real_inputs = StrokeInputBatch::Create({
      {.position = {7, 8}, .elapsed_time = Duration32::Seconds(3. / 60)},
  });
  ASSERT_EQ(real_inputs.status(), absl::OkStatus());
  input_modeler.ExtendStroke(*real_inputs, {}, Duration32::Zero());
  update = builder.ExtendStroke(input_modeler);

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
  EXPECT_THAT(builder.GetOutlines(), ElementsAre(Not(IsEmpty())));
}

TEST(StrokeShapeBuilderTest, StartAfterExtendEmptiesMeshAndOutline) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat;
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);
  ASSERT_NE(builder.GetMesh().VertexCount(), 0);
  ASSERT_NE(builder.GetMesh().TriangleCount(), 0);
  ASSERT_THAT(builder.GetMeshBounds().AsRect(),
              Optional(RectNear(*CalculateEnvelope(builder.GetMesh()).AsRect(),
                                0.0001)));
  ASSERT_THAT(builder.GetOutlines(), ElementsAre(Not(IsEmpty())));

  builder.StartStroke(brush_coat, 10, 0.1);

  EXPECT_EQ(builder.GetMesh().VertexCount(), 0);
  EXPECT_EQ(builder.GetMesh().TriangleCount(), 0);
  EXPECT_TRUE(builder.GetMeshBounds().IsEmpty());
  EXPECT_THAT(builder.GetOutlines(), IsEmpty());
}

TEST(StrokeShapeBuilderTest, NonTexturedNonParticleBrushDoesNotHaveSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat;
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    EXPECT_THAT(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i),
                PointEq({0, 0}));
  }
}

TEST(StrokeShapeBuilderTest, StampingNonParticleBrushDoesNotHaveSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{
      .tip = BrushTip{},
      .paint_preferences = {
          {.texture_layers = {
               {.mapping = BrushPaint::TextureMapping::kStamping}}}}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    EXPECT_THAT(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i),
                PointEq({0, 0}));
  }
}

TEST(StrokeShapeBuilderTest, NonTexturedParticleDistanceBrushHasSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{.tip = BrushTip{.particle_gap_distance_scale = 0.05}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  // For strokes that don't use the surface UV, all UV values are set to (0, 0).
  // We test that this isn't the case by finding the envelope; if its width and
  // height are both greater than zero, than surface UVs must have been set.
  Envelope uv_envelope;
  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    uv_envelope.Add(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i));
  }
  ASSERT_FALSE(uv_envelope.IsEmpty());
  EXPECT_GT(uv_envelope.AsRect()->Width(), 0);
  EXPECT_GT(uv_envelope.AsRect()->Height(), 0);
}

TEST(StrokeShapeBuilderTest, TiledTextureParticleDistanceBrushHasSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{
      .tip = BrushTip{.particle_gap_distance_scale = 0.05},
      .paint_preferences = {
          {.texture_layers = {
               {.mapping = BrushPaint::TextureMapping::kTiling}}}}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  // For strokes that don't use the surface UV, all UV values are set to (0, 0).
  // We test that this isn't the case by finding the envelope; if its width and
  // height are both greater than zero, than surface UVs must have been set.
  Envelope uv_envelope;
  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    uv_envelope.Add(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i));
  }
  ASSERT_FALSE(uv_envelope.IsEmpty());
  EXPECT_GT(uv_envelope.AsRect()->Width(), 0);
  EXPECT_GT(uv_envelope.AsRect()->Height(), 0);
}

TEST(StrokeShapeBuilderTest, NonStampingParticleDurationBrushHasSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{
      .tip = BrushTip{.particle_gap_duration = Duration32::Seconds(0.05)}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  // For strokes that don't use the surface UV, all UV values are set to (0, 0).
  // We test that this isn't the case by finding the envelope; if its width and
  // height are both greater than zero, than surface UVs must have been set.
  Envelope uv_envelope;
  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    uv_envelope.Add(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i));
  }
  ASSERT_FALSE(uv_envelope.IsEmpty());
  EXPECT_GT(uv_envelope.AsRect()->Width(), 0);
  EXPECT_GT(uv_envelope.AsRect()->Height(), 0);
}

TEST(StrokeShapeBuilderTest, StampingParticleDistanceBrushHasSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{
      .tip = BrushTip{.particle_gap_distance_scale = 0.05},
      .paint_preferences = {
          {.texture_layers = {
               {.mapping = BrushPaint::TextureMapping::kStamping}}}}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  // For strokes that don't use the surface UV, all UV values are set to (0, 0).
  // We test that this isn't the case by finding the envelope; if its width and
  // height are both greater than zero, than surface UVs must have been set.
  Envelope uv_envelope;
  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    uv_envelope.Add(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i));
  }
  ASSERT_FALSE(uv_envelope.IsEmpty());
  EXPECT_GT(uv_envelope.AsRect()->Width(), 0);
  EXPECT_GT(uv_envelope.AsRect()->Height(), 0);
}

TEST(StrokeShapeBuilderTest, StampingParticleDurationBrushHasSurfaceUvs) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{
      .tip = BrushTip{.particle_gap_duration = Duration32::Seconds(0.05)},
      .paint_preferences = {
          {.texture_layers = {
               {.mapping = BrushPaint::TextureMapping::kStamping}}}}};
  float brush_epsilon = 0.1;
  input_modeler.StartStroke(BrushFamily::DefaultInputModel(), brush_epsilon);
  builder.StartStroke(brush_coat, /* brush_size = */ 10, brush_epsilon);

  absl::StatusOr<StrokeInputBatch> inputs =
      StrokeInputBatch::Create({{.position = {5, 7}}});
  ASSERT_EQ(inputs.status(), absl::OkStatus());

  input_modeler.ExtendStroke(*inputs, {}, Duration32::Zero());
  builder.ExtendStroke(input_modeler);

  // For strokes that don't use the surface UV, all UV values are set to (0, 0).
  // We test that this isn't the case by finding the envelope; if its width and
  // height are both greater than zero, than surface UVs must have been set.
  Envelope uv_envelope;
  for (uint32_t i = 0; i < builder.GetMesh().VertexCount(); ++i) {
    uv_envelope.Add(StrokeVertex::GetSurfaceUvFromMesh(builder.GetMesh(), i));
  }
  ASSERT_FALSE(uv_envelope.IsEmpty());
  EXPECT_GT(uv_envelope.AsRect()->Width(), 0);
  EXPECT_GT(uv_envelope.AsRect()->Height(), 0);
}

TEST(StrokeShapeBuilderDeathTest, StartWithZeroBrushSize) {
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{.tip = BrushTip()};
  EXPECT_DEATH_IF_SUPPORTED(builder.StartStroke(brush_coat, 0, 0.1), "");
}

TEST(StrokeShapeBuilderDeathTest, StartWithZeroBrushEpsilon) {
  StrokeShapeBuilder builder;
  BrushCoat brush_coat{.tip = BrushTip()};
  EXPECT_DEATH_IF_SUPPORTED(builder.StartStroke(brush_coat, 1, 0), "");
}

TEST(StrokeShapeBuilderDeathTest, ExtendWithoutStart) {
  StrokeInputModeler input_modeler;
  StrokeShapeBuilder builder;
  EXPECT_DEATH_IF_SUPPORTED(builder.ExtendStroke(input_modeler), "");
}

}  // namespace
}  // namespace ink::strokes_internal
