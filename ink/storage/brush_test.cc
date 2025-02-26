// Copyright 2024-2025 Google LLC
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

#include "ink/storage/brush.h"

#include <optional>
#include <variant>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/brush/type_matchers.h"
#include "ink/color/color.h"
#include "ink/geometry/vec.h"
#include "ink/storage/color.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/coded.pb.h"
#include "ink/storage/proto_matchers.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsNull;
using ::testing::SizeIs;

constexpr absl::string_view kTestTextureId = "test-texture";

TEST(BrushTest, DecodeBrushProto) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  proto::BrushFamily* family_proto = brush_proto.mutable_brush_family();
  family_proto->mutable_input_model()->mutable_spring_model();
  proto::BrushCoat* coat_proto = family_proto->add_coats();
  proto::BrushTip* tip_proto = coat_proto->add_tips();
  tip_proto->set_corner_rounding(0.5f);
  tip_proto->set_opacity_multiplier(1.0f);
  proto::BrushPaint* paint_proto = coat_proto->mutable_paint();
  proto::BrushPaint::TextureLayer* texture_layer_proto =
      paint_proto->add_texture_layers();
  texture_layer_proto->set_client_color_texture_id(std::string(kTestTextureId));
  texture_layer_proto->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  texture_layer_proto->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  texture_layer_proto->set_size_x(10);
  texture_layer_proto->set_size_y(15);
  proto::BrushPaint::TextureKeyframe* keyframe_proto =
      texture_layer_proto->add_keyframes();
  keyframe_proto->set_progress(0.8f);
  keyframe_proto->set_size_x(10);
  keyframe_proto->set_size_y(15);
  keyframe_proto->set_opacity(0.6f);
  texture_layer_proto->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);

  absl::StatusOr<BrushFamily> expected_family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.5f},
      {.texture_layers = {
           {.client_color_texture_id = std::string(kTestTextureId),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .size = {10, 15},
            .keyframes = {{.progress = 0.8f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kDstOut}}});
  ASSERT_EQ(expected_family.status(), absl::OkStatus());
  absl::StatusOr<Brush> expected_brush =
      Brush::Create(*expected_family, Color::Green(), 10, 1.1);
  ASSERT_EQ(expected_brush.status(), absl::OkStatus());

  absl::StatusOr<Brush> brush = DecodeBrush(brush_proto);
  ASSERT_EQ(brush.status(), absl::OkStatus());
  EXPECT_THAT(*brush, BrushEq(*expected_brush));
}

TEST(BrushTest, DecodeBrushWithInvalidBrushSize) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(-8);
  brush_proto.set_epsilon_stroke_space(1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  brush_proto.mutable_brush_family()->add_coats()->add_tips();

  absl::Status invalid_size = DecodeBrush(brush_proto).status();
  EXPECT_EQ(invalid_size.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(invalid_size.message(), HasSubstr("size"));
}

TEST(BrushTest, DecodeBrushWithInvalidBrushEpsilon) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(20);
  brush_proto.set_epsilon_stroke_space(-1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  brush_proto.mutable_brush_family()->add_coats()->add_tips();

  absl::Status invalid_epsilon = DecodeBrush(brush_proto).status();
  EXPECT_EQ(invalid_epsilon.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(invalid_epsilon.message(), HasSubstr("epsilon"));
}

// This test ensures that we set correct proto field defaults when adding new
// BrushCoat struct fields, to avoid a repeat of b/337238252.
TEST(BrushTest, EmptyBrushCoatProtoDecodesToDefaultBrushCoat) {
  proto::BrushCoat coat_proto;
  absl::StatusOr<BrushCoat> brush_coat = DecodeBrushCoat(coat_proto);
  ASSERT_EQ(brush_coat.status(), absl::OkStatus());
  EXPECT_THAT(*brush_coat, BrushCoatEq(BrushCoat()));
}

// This test ensures that we set correct proto field defaults when adding new
// BrushTip struct fields, to avoid a repeat of b/337238252.
TEST(BrushTest, EmptyBrushTipProtoDecodesToDefaultBrushTip) {
  proto::BrushTip tip_proto;
  absl::StatusOr<BrushTip> brush_tip = DecodeBrushTip(tip_proto);
  ASSERT_EQ(brush_tip.status(), absl::OkStatus());
  EXPECT_THAT(*brush_tip, BrushTipEq(BrushTip()));
}

TEST(BrushTest, DecodeEmptyBrushBehaviorNode) {
  proto::BrushBehavior::Node node;
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("ink.proto.BrushBehavior.Node must specify a node"));
}

TEST(BrushTest, DecodeBrushBehaviorBinaryOpNodeWithUnspecifiedBinaryOp) {
  proto::BrushBehavior::Node node;
  node.mutable_binary_op_node()->set_operation(
      proto::BrushBehavior::BINARY_OP_UNSPECIFIED);
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("invalid ink.proto.BrushBehavior.BinaryOp value"));
}

TEST(BrushTest, DecodeBrushBehaviorDampingNodeWithUnspecifiedDampingSource) {
  proto::BrushBehavior::Node node;
  node.mutable_damping_node()->set_damping_source(
      proto::BrushBehavior::DAMPING_SOURCE_UNSPECIFIED);
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("invalid ink.proto.BrushBehavior.DampingSource value"));
}

TEST(BrushTest, DecodeBrushBehaviorResponseNodeWithNoResponseCurve) {
  proto::BrushBehavior::Node node;
  node.mutable_response_node();
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("ink.proto.BrushBehavior.ResponseNode must specify a "
                        "response_curve"));
}

TEST(BrushTest, DecodeBrushBehaviorSourceNodeWithUnspecifiedOutOfRange) {
  proto::BrushBehavior::Node node;
  proto::BrushBehavior::SourceNode* source_node = node.mutable_source_node();
  source_node->set_source(proto::BrushBehavior::SOURCE_NORMALIZED_PRESSURE);
  source_node->set_source_out_of_range_behavior(
      proto::BrushBehavior::OUT_OF_RANGE_UNSPECIFIED);
  source_node->set_source_value_range_start(0);
  source_node->set_source_value_range_end(1);
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("invalid ink.proto.BrushBehavior.OutOfRange value"));
}

// This test ensures that we set correct proto field defaults when adding new
// BrushPaint struct fields, to avoid a repeat of b/337238252.
TEST(BrushTest, EmptyBrushPaintProtoDecodesToDefaultBrushPaint) {
  proto::BrushPaint paint_proto;
  absl::StatusOr<BrushPaint> brush_paint = DecodeBrushPaint(paint_proto);
  ASSERT_EQ(brush_paint.status(), absl::OkStatus());
  EXPECT_THAT(*brush_paint, BrushPaintEq(BrushPaint()));
}

// This test ensures that we set correct proto field defaults when adding new
// `BrushPaint::TextureLayer` struct fields, to avoid a repeat of b/337238252.
TEST(BrushTest, MostlyEmptyTextureLayerProtoDecodesWithDefaultValues) {
  proto::BrushPaint paint_proto;
  // The proto should decode successfully, and all the omitted proto fields
  // should be set to their default values in the `BrushPaint::TextureLayer`
  // struct.
  paint_proto.add_texture_layers();
  absl::StatusOr<BrushPaint> brush_paint = DecodeBrushPaint(paint_proto);
  ASSERT_EQ(brush_paint.status(), absl::OkStatus());
  EXPECT_THAT(
      brush_paint->texture_layers,
      ElementsAre(BrushPaintTextureLayerEq(BrushPaint::TextureLayer{})));
}

TEST(BrushTest, DecodeBrushPaintWithInvalidTextureKeyframe) {
  {  // Only one of size_x and size_y is set.
    proto::Brush brush_proto;
    proto::BrushFamily* family_proto = brush_proto.mutable_brush_family();

    proto::BrushPaint* paint_proto = family_proto->add_coats()->mutable_paint();
    proto::BrushPaint::TextureLayer* texture_layer_proto =
        paint_proto->add_texture_layers();
    texture_layer_proto->set_size_unit(
        proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
    texture_layer_proto->set_client_color_texture_id(
        std::string(kTestTextureId));
    texture_layer_proto->set_origin(
        proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
    texture_layer_proto->set_mapping(
        proto::BrushPaint::TextureLayer::MAPPING_WINDING);
    proto::BrushPaint::TextureKeyframe* keyframe_proto =
        texture_layer_proto->add_keyframes();
    keyframe_proto->set_progress(0.8f);
    keyframe_proto->set_size_y(15);
    keyframe_proto->set_opacity(0.6f);

    absl::Status missing_size_component = DecodeBrush(brush_proto).status();
    EXPECT_EQ(missing_size_component.code(),
              absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(missing_size_component.message(), HasSubstr("size_x"));
  }
  {  // Only one of offset_x and offset_y is set.
    proto::Brush brush_proto;
    proto::BrushFamily* family_proto = brush_proto.mutable_brush_family();

    proto::BrushPaint* paint_proto = family_proto->add_coats()->mutable_paint();
    proto::BrushPaint::TextureLayer* texture_layer_proto =
        paint_proto->add_texture_layers();
    texture_layer_proto->set_size_unit(
        proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
    texture_layer_proto->set_client_color_texture_id(
        std::string(kTestTextureId));
    texture_layer_proto->set_origin(
        proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
    texture_layer_proto->set_mapping(
        proto::BrushPaint::TextureLayer::MAPPING_WINDING);
    proto::BrushPaint::TextureKeyframe* keyframe_proto =
        texture_layer_proto->add_keyframes();
    keyframe_proto->set_progress(0.8f);
    keyframe_proto->set_size_x(10);
    keyframe_proto->set_size_y(15);
    keyframe_proto->set_offset_x(3);
    keyframe_proto->set_opacity(0.6f);

    absl::Status missing_offset_component = DecodeBrush(brush_proto).status();
    EXPECT_EQ(missing_offset_component.code(),
              absl::StatusCode::kInvalidArgument);
    EXPECT_THAT(missing_offset_component.message(), HasSubstr("offset_x"));
  }
}

void DecodeBrushDoesNotCrashOnArbitraryInput(const proto::Brush& brush_proto) {
  DecodeBrush(brush_proto).IgnoreError();
}
FUZZ_TEST(BrushTest, DecodeBrushDoesNotCrashOnArbitraryInput);

void DecodeBrushFamilyDoesNotCrashOnArbitraryInput(
    const proto::BrushFamily& family_proto) {
  DecodeBrushFamily(family_proto).IgnoreError();
}
FUZZ_TEST(BrushTest, DecodeBrushFamilyDoesNotCrashOnArbitraryInput);

void DecodeBrushTipDoesNotCrashOnArbitraryInput(
    const proto::BrushTip& tip_proto) {
  DecodeBrushTip(tip_proto).IgnoreError();
}
FUZZ_TEST(BrushTest, DecodeBrushTipDoesNotCrashOnArbitraryInput);

TEST(BrushTest, EncodeBrush) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{
          .corner_rounding = 0.25f,
          .opacity_multiplier = 0.7f,
          .particle_gap_distance_scale = 1,
          .particle_gap_duration = Duration32::Seconds(2),
      },
      {.texture_layers = {
           {.client_color_texture_id = std::string(kTestTextureId),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .wrap_y = BrushPaint::TextureWrap::kMirror,
            .size = {10, 15},
            .size_jitter = {3, 7},
            .animation_frames = 2,
            .keyframes = {{.progress = 0.8f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kSrcIn}}});
  ASSERT_EQ(family.status(), absl::OkStatus());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Green(), 10, 1.1);
  ASSERT_EQ(brush.status(), absl::OkStatus());
  proto::Brush brush_proto_out;
  EncodeBrush(*brush, brush_proto_out);

  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  proto::BrushFamily* brush_family_proto = brush_proto.mutable_brush_family();
  brush_family_proto->mutable_input_model()->mutable_spring_model();
  proto::BrushCoat* brush_coat_proto = brush_family_proto->add_coats();
  proto::BrushTip* brush_tip_proto = brush_coat_proto->add_tips();
  brush_tip_proto->set_scale_x(1.f);
  brush_tip_proto->set_scale_y(1.f);
  brush_tip_proto->set_corner_rounding(0.25f);
  brush_tip_proto->set_slant_radians(0.f);
  brush_tip_proto->set_pinch(0.f);
  brush_tip_proto->set_rotation_radians(0.f);
  brush_tip_proto->set_opacity_multiplier(0.7f);
  brush_tip_proto->set_particle_gap_distance_scale(1);
  brush_tip_proto->set_particle_gap_duration_seconds(2);
  proto::BrushPaint* brush_paint_proto = brush_coat_proto->mutable_paint();
  proto::BrushPaint::TextureLayer* texture_layer_proto =
      brush_paint_proto->add_texture_layers();
  texture_layer_proto->set_client_color_texture_id(std::string(kTestTextureId));
  texture_layer_proto->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  texture_layer_proto->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_STROKE_SPACE_ORIGIN);
  texture_layer_proto->set_size_x(10);
  texture_layer_proto->set_size_y(15);
  texture_layer_proto->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  texture_layer_proto->set_wrap_x(proto::BrushPaint::TextureLayer::WRAP_REPEAT);
  texture_layer_proto->set_wrap_y(proto::BrushPaint::TextureLayer::WRAP_MIRROR);
  texture_layer_proto->set_offset_x(0.f);
  texture_layer_proto->set_offset_y(0.f);
  texture_layer_proto->set_rotation_in_radians(0.f);
  texture_layer_proto->set_size_jitter_x(3.f);
  texture_layer_proto->set_size_jitter_y(7.f);
  texture_layer_proto->set_offset_jitter_x(0.f);
  texture_layer_proto->set_offset_jitter_y(0.f);
  texture_layer_proto->set_rotation_jitter_in_radians(0.f);
  texture_layer_proto->set_opacity(1.f);
  texture_layer_proto->set_animation_frames(2);
  proto::BrushPaint::TextureKeyframe* keyframe_proto =
      texture_layer_proto->add_keyframes();
  keyframe_proto->set_progress(0.8f);
  keyframe_proto->set_size_x(10);
  keyframe_proto->set_size_y(15);
  keyframe_proto->set_opacity(0.6f);
  texture_layer_proto->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_SRC_IN);

  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto));
}

TEST(BrushTest, EncodeBrushFamilyIntoNonEmptyProto) {
  // Create a brush family with no ID.
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.25f},
      {.texture_layers = {
           {.client_color_texture_id = std::string(kTestTextureId),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .size = {10, 15}}}});
  ASSERT_EQ(family.status(), absl::OkStatus());
  // Initialize the proto with a non-empty ID, and a different brush tip.
  proto::BrushFamily family_proto_out;
  family_proto_out.add_coats()->add_tips()->set_corner_rounding(1.0f);
  family_proto_out.set_client_brush_family_id("marker");

  EncodeBrushFamily(*family, family_proto_out);

  // After encoding, the old tip proto should be replaced, and the old ID
  // should get cleared.
  ASSERT_EQ(family_proto_out.coats_size(), 1u);
  ASSERT_EQ(family_proto_out.coats(0).tips_size(), 1u);
  EXPECT_EQ(family_proto_out.coats(0).tips(0).corner_rounding(), 0.25f);
  EXPECT_FALSE(family_proto_out.has_client_brush_family_id());
}

TEST(BrushTest, DecodeBrushFamilyWithNoInputModel) {
  proto::BrushFamily family_proto;
  family_proto.add_coats()->add_tips();
  absl::StatusOr<BrushFamily> family = DecodeBrushFamily(family_proto);
  ASSERT_THAT(family, IsOk());
  EXPECT_TRUE(std::holds_alternative<BrushFamily::SpringModel>(
      family->GetInputModel()));
}

TEST(BrushTest, EncodeBrushPaintWithInvalidTextureMapping) {
  BrushPaint paint;
  paint.texture_layers.push_back(
      {.client_color_texture_id = std::string(kTestTextureId),
       .mapping = static_cast<BrushPaint::TextureMapping>(99),
       .size = {10, 15}});
  proto::BrushPaint paint_proto;
  EncodeBrushPaint(paint, paint_proto);
  ASSERT_EQ(paint_proto.texture_layers(0).size_x(), 10);
  ASSERT_EQ(paint_proto.texture_layers(0).size_y(), 15);
  EXPECT_EQ(paint_proto.texture_layers(0).mapping(),
            proto::BrushPaint::TextureLayer::MAPPING_UNSPECIFIED);
}

TEST(BrushTest, EncodeBrushPaintWithInvalidTextureOrigin) {
  BrushPaint paint;
  paint.texture_layers.push_back(
      {.client_color_texture_id = std::string(kTestTextureId),
       .origin = static_cast<BrushPaint::TextureOrigin>(99),
       .size = {10, 15}});
  proto::BrushPaint paint_proto;
  EncodeBrushPaint(paint, paint_proto);
  ASSERT_EQ(paint_proto.texture_layers(0).size_x(), 10);
  ASSERT_EQ(paint_proto.texture_layers(0).size_y(), 15);
  EXPECT_EQ(paint_proto.texture_layers(0).origin(),
            proto::BrushPaint::TextureLayer::ORIGIN_UNSPECIFIED);
}

TEST(BrushTest, EncodeBrushTipWithInvalidEnumValues) {
  BrushTip tip;
  tip.behaviors.push_back(BrushBehavior{{
      BrushBehavior::SourceNode{
          .source = static_cast<BrushBehavior::Source>(99),
          .source_out_of_range_behavior =
              static_cast<BrushBehavior::OutOfRange>(99),
      },
      BrushBehavior::FallbackFilterNode{
          .is_fallback_for =
              static_cast<BrushBehavior::OptionalInputProperty>(99),
      },
      BrushBehavior::ResponseNode{
          .response_curve = {static_cast<EasingFunction::Predefined>(99)},
      },
      BrushBehavior::TargetNode{
          .target = static_cast<BrushBehavior::Target>(99),
      },
  }});
  proto::BrushTip tip_proto;
  EncodeBrushTip(tip, tip_proto);
  ASSERT_EQ(tip_proto.behaviors_size(), 1);
  const proto::BrushBehavior& behavior_proto = tip_proto.behaviors(0);
  ASSERT_EQ(behavior_proto.nodes_size(), 4);
  EXPECT_TRUE(behavior_proto.nodes(0).has_source_node());
  EXPECT_EQ(behavior_proto.nodes(0).source_node().source(),
            proto::BrushBehavior::SOURCE_UNSPECIFIED);
  EXPECT_EQ(
      behavior_proto.nodes(0).source_node().source_out_of_range_behavior(),
      proto::BrushBehavior::OUT_OF_RANGE_UNSPECIFIED);
  EXPECT_TRUE(behavior_proto.nodes(1).has_fallback_filter_node());
  EXPECT_EQ(behavior_proto.nodes(1).fallback_filter_node().is_fallback_for(),
            proto::BrushBehavior::OPTIONAL_INPUT_UNSPECIFIED);
  EXPECT_TRUE(behavior_proto.nodes(2).has_response_node());
  EXPECT_TRUE(
      behavior_proto.nodes(2).response_node().has_predefined_response_curve());
  EXPECT_EQ(behavior_proto.nodes(2).response_node().predefined_response_curve(),
            proto::PREDEFINED_EASING_UNSPECIFIED);
  EXPECT_TRUE(behavior_proto.nodes(3).has_target_node());
  EXPECT_EQ(behavior_proto.nodes(3).target_node().target(),
            proto::BrushBehavior::TARGET_UNSPECIFIED);
}

TEST(BrushTest, EncodeBrushTipWithInvalidStepPosition) {
  BrushTip tip;
  tip.behaviors.push_back(BrushBehavior{{
      BrushBehavior::ResponseNode{
          .response_curve = EasingFunction{EasingFunction::Steps{
              .step_count = 4,
              .step_position = static_cast<EasingFunction::StepPosition>(100),
          }},
      },
  }});
  proto::BrushTip tip_proto;
  EncodeBrushTip(tip, tip_proto);
  ASSERT_EQ(tip_proto.behaviors_size(), 1);
  const proto::BrushBehavior& behavior_proto = tip_proto.behaviors(0);
  ASSERT_EQ(behavior_proto.nodes_size(), 1);
  EXPECT_TRUE(behavior_proto.nodes(0).has_response_node());
  const proto::BrushBehavior::ResponseNode& response_node =
      behavior_proto.nodes(0).response_node();
  EXPECT_TRUE(response_node.has_steps_response_curve());
  EXPECT_EQ(response_node.steps_response_curve().step_position(),
            proto::STEP_POSITION_UNSPECIFIED);
}

TEST(BrushTest, EncodeBrushBehaviorBinaryOpNodeWithInvalidOperation) {
  BrushBehavior::BinaryOpNode node{
      .operation = static_cast<BrushBehavior::BinaryOp>(123)};
  proto::BrushBehavior::Node node_proto;
  EncodeBrushBehaviorNode(node, node_proto);
  EXPECT_TRUE(node_proto.has_binary_op_node());
  EXPECT_EQ(node_proto.binary_op_node().operation(),
            proto::BrushBehavior::BINARY_OP_UNSPECIFIED);
}

TEST(BrushTest, EncodeBrushBehaviorDampingNodeWithInvalidDampingSource) {
  BrushBehavior::DampingNode node{
      .damping_source = static_cast<BrushBehavior::DampingSource>(123),
      .damping_gap = 1.0f,
  };
  proto::BrushBehavior::Node node_proto;
  EncodeBrushBehaviorNode(node, node_proto);
  EXPECT_TRUE(node_proto.has_damping_node());
  EXPECT_EQ(node_proto.damping_node().damping_source(),
            proto::BrushBehavior::DAMPING_SOURCE_UNSPECIFIED);
  EXPECT_EQ(node_proto.damping_node().damping_gap(), 1.0f);
}

void EncodeDecodeBrushRoundTrip(const Brush& brush_in) {
  proto::Brush brush_proto_in;
  EncodeBrush(brush_in, brush_proto_in);

  absl::StatusOr<Brush> brush_out = DecodeBrush(brush_proto_in);
  ASSERT_EQ(brush_out.status(), absl::OkStatus());
  EXPECT_THAT(*brush_out, BrushEq(brush_in));

  proto::Brush brush_proto_out;
  EncodeBrush(*brush_out, brush_proto_out);
  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeBrushRoundTrip).WithDomains(ArbitraryBrush());

void EncodeDecodeBrushFamilyRoundTrip(const BrushFamily& family_in) {
  proto::BrushFamily family_proto_in;
  EncodeBrushFamily(family_in, family_proto_in);

  absl::StatusOr<BrushFamily> family_out = DecodeBrushFamily(family_proto_in);
  ASSERT_EQ(family_out.status(), absl::OkStatus());
  EXPECT_THAT(*family_out, BrushFamilyEq(family_in));

  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeBrushFamilyRoundTrip)
    .WithDomains(ArbitraryBrushFamily());

// Unlike the Brush and BrushFamily classes, BrushCoat is an open struct that
// does not enforce validity. Proto encode/decode round-tripping is only
// guaranteed for valid BrushCoat structs.
void EncodeDecodeValidBrushCoatRoundTrip(const BrushCoat& coat_in) {
  proto::BrushCoat coat_proto_in;
  EncodeBrushCoat(coat_in, coat_proto_in);

  absl::StatusOr<BrushCoat> coat_out = DecodeBrushCoat(coat_proto_in);
  ASSERT_EQ(coat_out.status(), absl::OkStatus());
  EXPECT_THAT(*coat_out, BrushCoatEq(coat_in));

  proto::BrushCoat coat_proto_out;
  EncodeBrushCoat(*coat_out, coat_proto_out);
  EXPECT_THAT(coat_proto_out, EqualsProto(coat_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeValidBrushCoatRoundTrip)
    .WithDomains(ValidBrushCoat());

// Unlike the Brush and BrushFamily classes, BrushPaint is an open struct that
// does not enforce validity. Proto encode/decode round-tripping is only
// guaranteed for valid BrushPaint structs.
void EncodeDecodeValidBrushPaintRoundTrip(const BrushPaint& paint_in) {
  proto::BrushPaint paint_proto_in;
  EncodeBrushPaint(paint_in, paint_proto_in);

  absl::StatusOr<BrushPaint> paint_out = DecodeBrushPaint(paint_proto_in);
  ASSERT_EQ(paint_out.status(), absl::OkStatus());
  EXPECT_THAT(*paint_out, BrushPaintEq(paint_in));

  proto::BrushPaint paint_proto_out;
  EncodeBrushPaint(*paint_out, paint_proto_out);
  EXPECT_THAT(paint_proto_out, EqualsProto(paint_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeValidBrushPaintRoundTrip)
    .WithDomains(ValidBrushPaint());

// Unlike the Brush and BrushFamily classes, BrushTip is an open struct that
// does not enforce validity. Proto encode/decode round-tripping is only
// guaranteed for valid BrushTip structs.
void EncodeDecodeValidBrushTipRoundTrip(const BrushTip& tip_in) {
  proto::BrushTip tip_proto_in;
  EncodeBrushTip(tip_in, tip_proto_in);

  absl::StatusOr<BrushTip> tip_out = DecodeBrushTip(tip_proto_in);
  ASSERT_EQ(tip_out.status(), absl::OkStatus());
  EXPECT_THAT(*tip_out, BrushTipEq(tip_in));

  proto::BrushTip tip_proto_out;
  EncodeBrushTip(*tip_out, tip_proto_out);
  EXPECT_THAT(tip_proto_out, EqualsProto(tip_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeValidBrushTipRoundTrip)
    .WithDomains(ValidBrushTip());

// Unlike the `Brush` and `BrushFamily` classes, `BrushBehavior::Node` is a
// variant of open structs that do not enforce validity. Proto encode/decode
// round-tripping is only guaranteed for valid `BrushBehavior::Node` structs.
void EncodeDecodeValidBrushBehaviorNodeRoundTrip(
    const BrushBehavior::Node& node_in) {
  proto::BrushBehavior::Node node_proto_in;
  EncodeBrushBehaviorNode(node_in, node_proto_in);

  absl::StatusOr<BrushBehavior::Node> node_out =
      DecodeBrushBehaviorNode(node_proto_in);
  ASSERT_EQ(node_out.status(), absl::OkStatus());
  EXPECT_THAT(*node_out, BrushBehaviorNodeEq(node_in));

  proto::BrushBehavior::Node node_proto_out;
  EncodeBrushBehaviorNode(*node_out, node_proto_out);
  EXPECT_THAT(node_proto_out, EqualsProto(node_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeValidBrushBehaviorNodeRoundTrip)
    .WithDomains(ValidBrushBehaviorNode());

}  // namespace
}  // namespace ink
