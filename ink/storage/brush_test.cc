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

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/easing_function.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/brush/internal/brush_family_internal_accessor.h"
#include "ink/brush/type_matchers.h"
#include "ink/brush/version.h"
#include "ink/color/color.h"
#include "ink/geometry/angle.h"
#include "ink/storage/color.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"
#include "ink/storage/proto/color.pb.h"
#include "ink/storage/proto/options.pb.h"
#include "ink/storage/proto/stroke_input_batch.pb.h"
#include "ink/storage/proto_matchers.h"
#include "ink/types/duration.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

// Undefine macro that may have been introduced by Windows headers which
// clashes with Reflection::GetMessage.
#ifdef GetMessage
#undef GetMessage
#endif

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::google::protobuf::TextFormat;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsNull;
using ::testing::Le;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAreArray;
using ::testing::VariantWith;

constexpr absl::string_view kTestTextureId1 = "test-texture-one";
constexpr absl::string_view kTestTextureId2 = "test-texture-two";
constexpr absl::string_view kTestTextureId1Decoded = "test-texture-one-decoded";
constexpr absl::string_view kTestTextureId2Decoded = "test-texture-two-decoded";

// Actual strings will be encoded PNGs, but for testing purposes we use
// any string that can be confirmed unchanged before and after decoding.
std::string TestPngBytes1x1() { return "{0x12, 0x34, 0x56, 0x78}"; }
std::string TestPngBytes2x2() {
  return "{0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, "
         "0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78}";
}

TEST(BrushTest, DecodeBrushProto) {
  proto::Brush brush_proto;
  std::string test_bitmap_1 = TestPngBytes1x1();
  std::string test_bitmap_2 = TestPngBytes2x2();
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  brush_proto.mutable_color()->set_r(0);
  brush_proto.mutable_color()->set_g(1);
  brush_proto.mutable_color()->set_b(0);
  brush_proto.mutable_color()->set_a(1);
  brush_proto.mutable_color()->set_color_space(
      proto::ColorSpace::COLOR_SPACE_SRGB);
  proto::BrushFamily* family_proto = brush_proto.mutable_brush_family();
  family_proto->mutable_input_model()->mutable_passthrough_model();
  family_proto->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId1), test_bitmap_1});
  family_proto->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId2), test_bitmap_2});
  proto::BrushCoat* coat_proto = family_proto->add_coats();
  coat_proto->mutable_tip()->set_corner_rounding(0.5f);
  proto::BrushPaint* paint_proto = coat_proto->add_paint_preferences();
  proto::BrushPaint::TextureLayer* texture_layer_proto_1 =
      paint_proto->add_texture_layers();
  texture_layer_proto_1->set_client_texture_id(std::string(kTestTextureId1));
  texture_layer_proto_1->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_STAMPING);
  texture_layer_proto_1->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_1->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_BRUSH_SIZE);
  texture_layer_proto_1->set_size_x(10);
  texture_layer_proto_1->set_size_y(15);
  texture_layer_proto_1->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);
  proto::BrushPaint::TextureLayer* texture_layer_proto_2 =
      paint_proto->add_texture_layers();
  texture_layer_proto_2->set_client_texture_id(std::string(kTestTextureId2));
  texture_layer_proto_2->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_STAMPING);
  texture_layer_proto_2->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_2->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_BRUSH_SIZE);
  texture_layer_proto_2->set_size_x(4);
  texture_layer_proto_2->set_size_y(10);
  texture_layer_proto_2->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);
  proto::BrushPaint::TextureLayer* texture_layer_proto_3 =
      paint_proto->add_texture_layers();
  texture_layer_proto_3->set_client_texture_id(std::string(kTestTextureId1));
  texture_layer_proto_3->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_STAMPING);
  texture_layer_proto_3->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_3->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_BRUSH_SIZE);
  texture_layer_proto_3->set_size_x(1);
  texture_layer_proto_3->set_size_y(2);
  texture_layer_proto_3->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);

  // Expected brush family.
  absl::StatusOr<BrushFamily> expected_family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.5f},
      {.texture_layers =
           {{.client_texture_id = std::string(kTestTextureId1Decoded),
             .mapping = BrushPaint::TextureMapping::kStamping,
             .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
             .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
             .size = {10, 15},
             .blend_mode = BrushPaint::BlendMode::kDstOut},
            {.client_texture_id = std::string(kTestTextureId2Decoded),
             .mapping = BrushPaint::TextureMapping::kStamping,
             .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
             .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
             .size = {4, 10},
             .blend_mode = BrushPaint::BlendMode::kDstOut},
            {.client_texture_id = std::string(kTestTextureId1Decoded),
             .mapping = BrushPaint::TextureMapping::kStamping,
             .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
             .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
             .size = {1, 2},
             .blend_mode = BrushPaint::BlendMode::kDstOut}}},
      BrushFamily::PassthroughModel{});

  ASSERT_EQ(expected_family.status(), absl::OkStatus());
  absl::StatusOr<Brush> expected_brush =
      Brush::Create(*expected_family, Color::Green(), 10, 1.1);
  ASSERT_EQ(expected_brush.status(), absl::OkStatus());

  std::map<std::string, std::string> decoded_bitmaps = {};
  ClientTextureIdProviderAndBitmapReceiver callback =
      [&decoded_bitmaps](absl::string_view id,
                         absl::string_view bitmap) -> std::string {
    std::string new_id = "";
    if (id == kTestTextureId1) {
      new_id = kTestTextureId1Decoded;
    } else if (id == kTestTextureId2) {
      new_id = kTestTextureId2Decoded;
    }
    if (!bitmap.empty()) {
      if (decoded_bitmaps.find(new_id) == decoded_bitmaps.end()) {
        decoded_bitmaps.insert({new_id, std::string(bitmap)});
      }
    }
    return new_id;
  };
  absl::StatusOr<Brush> brush = DecodeBrush(brush_proto, callback);
  ASSERT_EQ(brush.status(), absl::OkStatus());
  EXPECT_THAT(*brush, BrushEq(*expected_brush));
  ASSERT_EQ(decoded_bitmaps.size(), 2);

  EXPECT_NE(decoded_bitmaps.find(std::string(kTestTextureId1Decoded)),
            decoded_bitmaps.end());
  std::string decoded_bitmap_1 =
      decoded_bitmaps.at(std::string(kTestTextureId1Decoded));
  EXPECT_EQ(decoded_bitmap_1, test_bitmap_1);

  EXPECT_NE(decoded_bitmaps.find(std::string(kTestTextureId2Decoded)),
            decoded_bitmaps.end());
  std::string decoded_bitmap_2 =
      decoded_bitmaps.at(std::string(kTestTextureId2Decoded));
  EXPECT_EQ(decoded_bitmap_2, test_bitmap_2);
}

TEST(BrushTest, DecodeBrushWithInvalidBrushSize) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(-8);
  brush_proto.set_epsilon_stroke_space(1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  brush_proto.mutable_brush_family()->add_coats()->mutable_tip();

  absl::Status invalid_size = DecodeBrush(brush_proto).status();
  EXPECT_EQ(invalid_size.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(invalid_size.message(), HasSubstr("size"));
}

TEST(BrushTest, DecodeBrushWithInvalidBrushEpsilon) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(20);
  brush_proto.set_epsilon_stroke_space(-1.1);
  EncodeColor(Color::Green(), *brush_proto.mutable_color());
  brush_proto.mutable_brush_family()->add_coats()->mutable_tip();

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

TEST(BrushTest, DecodeBrushBehaviorWithInvalidSourceValueRange) {
  proto::BrushBehavior behavior_proto;
  proto::BrushBehavior::SourceNode* source_node_proto =
      behavior_proto.add_nodes()->mutable_source_node();
  source_node_proto->set_source(
      proto::BrushBehavior::SOURCE_NORMALIZED_PRESSURE);
  source_node_proto->set_source_value_range_start(0);
  source_node_proto->set_source_value_range_end(0);
  source_node_proto->set_source_out_of_range_behavior(
      proto::BrushBehavior::OUT_OF_RANGE_CLAMP);
  proto::BrushBehavior::TargetNode* target_node_proto =
      behavior_proto.add_nodes()->mutable_target_node();
  target_node_proto->set_target(proto::BrushBehavior::TARGET_SIZE_MULTIPLIER);
  target_node_proto->set_target_modifier_range_start(0);
  target_node_proto->set_target_modifier_range_end(1);

  EXPECT_THAT(DecodeBrushBehavior(behavior_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("source_value_range")));
}

TEST(BrushTest, DecodeEmptyBrushBehaviorNode) {
  proto::BrushBehavior::Node node;
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(),
              HasSubstr("ink.proto.BrushBehavior.Node must specify a node"));
}

TEST(BrushTest, DecodeInvalidBrushBehaviorNode) {
  // The proto is fine, but the struct fails validation.
  proto::BrushBehavior::Node node;
  proto::BrushBehavior::SourceNode* source_node = node.mutable_source_node();
  source_node->set_source(proto::BrushBehavior::SOURCE_NORMALIZED_PRESSURE);
  source_node->set_source_out_of_range_behavior(
      proto::BrushBehavior::OUT_OF_RANGE_CLAMP);
  source_node->set_source_value_range_start(0);
  source_node->set_source_value_range_end(0);
  EXPECT_THAT(DecodeBrushBehaviorNode(node).status(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("source_value_range")));
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

TEST(BrushTest, DecodeBrushBehaviorDampingNodeWithUnspecifiedProgressDomain) {
  proto::BrushBehavior::Node node;
  node.mutable_damping_node()->set_damping_source(
      proto::BrushBehavior::PROGRESS_DOMAIN_UNSPECIFIED);
  absl::Status status = DecodeBrushBehaviorNode(node).status();
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(
      status.message(),
      HasSubstr("invalid ink.proto.BrushBehavior.ProgressDomain value"));
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

TEST(BrushTest, EncodeBrushWithoutTextureMap) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{
          .corner_rounding = 0.25f,
          .particle_gap_distance_scale = 1,
          .particle_gap_duration = Duration32::Seconds(2),
      },
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId1),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .wrap_y = BrushPaint::TextureWrap::kMirror,
                           .size = {10, 15},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn}},
       .self_overlap = BrushPaint::SelfOverlap::kDiscard},
      BrushFamily::PassthroughModel{});
  ASSERT_EQ(family.status(), absl::OkStatus());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Green(), 10, 1.1);
  ASSERT_EQ(brush.status(), absl::OkStatus());
  proto::Brush brush_proto_out;
  int callback_count = 0;
  TextureBitmapProvider callback = [&callback_count](absl::string_view id) {
    callback_count++;
    return std::nullopt;
  };
  EncodeBrush(*brush, brush_proto_out, callback);

  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  brush_proto.mutable_color()->set_r(0);
  brush_proto.mutable_color()->set_g(1);
  brush_proto.mutable_color()->set_b(0);
  brush_proto.mutable_color()->set_a(1);
  brush_proto.mutable_color()->set_color_space(
      proto::ColorSpace::COLOR_SPACE_SRGB);
  brush_proto.mutable_brush_family()
      ->mutable_input_model()
      ->mutable_passthrough_model();
  brush_proto.mutable_brush_family()->set_min_version(0);
  proto::BrushCoat* coat_proto =
      brush_proto.mutable_brush_family()->add_coats();
  coat_proto->mutable_tip()->set_scale_x(1.f);
  coat_proto->mutable_tip()->set_scale_y(1.f);
  coat_proto->mutable_tip()->set_corner_rounding(0.25f);
  coat_proto->mutable_tip()->set_slant_radians(0.f);
  coat_proto->mutable_tip()->set_pinch(0.f);
  coat_proto->mutable_tip()->set_rotation_radians(0.f);
  coat_proto->mutable_tip()->set_particle_gap_distance_scale(1);
  coat_proto->mutable_tip()->set_particle_gap_duration_seconds(2);
  proto::BrushPaint* paint_proto = coat_proto->add_paint_preferences();
  proto::BrushPaint::TextureLayer* layer_proto =
      paint_proto->add_texture_layers();
  layer_proto->set_client_texture_id("test-texture-one");
  layer_proto->set_mapping(proto::BrushPaint::TextureLayer::MAPPING_STAMPING);
  layer_proto->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_STROKE_SPACE_ORIGIN);
  layer_proto->set_size_x(10);
  layer_proto->set_size_y(15);
  layer_proto->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_BRUSH_SIZE);
  layer_proto->set_wrap_x(proto::BrushPaint::TextureLayer::WRAP_REPEAT);
  layer_proto->set_wrap_y(proto::BrushPaint::TextureLayer::WRAP_MIRROR);
  layer_proto->set_offset_x(0.f);
  layer_proto->set_offset_y(0.f);
  layer_proto->set_rotation_in_radians(0.f);
  layer_proto->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_SRC_IN);
  paint_proto->set_self_overlap(proto::BrushPaint::SELF_OVERLAP_DISCARD);

  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto));
  EXPECT_EQ(callback_count, 1);
}

TEST(BrushTest, EncodeBrushWithTextureMap) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{
          .corner_rounding = 0.25f,
          .particle_gap_distance_scale = 1,
          .particle_gap_duration = Duration32::Seconds(2),
      },
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId1),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .wrap_y = BrushPaint::TextureWrap::kMirror,
                           .size = {10, 15},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn}},
       .self_overlap = BrushPaint::SelfOverlap::kAccumulate},
      BrushFamily::PassthroughModel{});
  ASSERT_EQ(family.status(), absl::OkStatus());
  absl::StatusOr<Brush> brush = Brush::Create(*family, Color::Green(), 10, 1.1);
  ASSERT_EQ(brush.status(), absl::OkStatus());
  proto::Brush brush_proto_out;
  int callback_count = 0;
  TextureBitmapProvider callback =
      [&callback_count](absl::string_view id) -> std::optional<std::string> {
    callback_count++;
    if (kTestTextureId1 == id) {
      return TestPngBytes1x1();
    } else if (kTestTextureId2 == id) {
      return TestPngBytes2x2();
    }
    return std::nullopt;
  };
  EncodeBrush(*brush, brush_proto_out, callback);

  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  brush_proto.mutable_color()->set_r(0);
  brush_proto.mutable_color()->set_g(1);
  brush_proto.mutable_color()->set_b(0);
  brush_proto.mutable_color()->set_a(1);
  brush_proto.mutable_color()->set_color_space(
      proto::ColorSpace::COLOR_SPACE_SRGB);
  brush_proto.mutable_brush_family()
      ->mutable_input_model()
      ->mutable_passthrough_model();
  brush_proto.mutable_brush_family()->set_min_version(0);
  brush_proto.mutable_brush_family()->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId1), TestPngBytes1x1()});
  proto::BrushCoat* coat_proto =
      brush_proto.mutable_brush_family()->add_coats();
  proto::BrushTip* tip_proto = coat_proto->mutable_tip();
  tip_proto->set_scale_x(1.f);
  tip_proto->set_scale_y(1.f);
  tip_proto->set_corner_rounding(0.25f);
  tip_proto->set_slant_radians(0.f);
  tip_proto->set_pinch(0.f);
  tip_proto->set_rotation_radians(0.f);
  tip_proto->set_particle_gap_distance_scale(1);
  tip_proto->set_particle_gap_duration_seconds(2);
  proto::BrushPaint* paint_proto = coat_proto->add_paint_preferences();
  proto::BrushPaint::TextureLayer* texture_layer_proto =
      paint_proto->add_texture_layers();
  texture_layer_proto->set_client_texture_id(kTestTextureId1);
  texture_layer_proto->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_STAMPING);
  texture_layer_proto->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_STROKE_SPACE_ORIGIN);
  texture_layer_proto->set_size_x(10);
  texture_layer_proto->set_size_y(15);
  texture_layer_proto->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_BRUSH_SIZE);
  texture_layer_proto->set_wrap_x(proto::BrushPaint::TextureLayer::WRAP_REPEAT);
  texture_layer_proto->set_wrap_y(proto::BrushPaint::TextureLayer::WRAP_MIRROR);
  texture_layer_proto->set_offset_x(0.f);
  texture_layer_proto->set_offset_y(0.f);
  texture_layer_proto->set_rotation_in_radians(0.f);
  texture_layer_proto->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_SRC_IN);
  paint_proto->set_self_overlap(proto::BrushPaint::SELF_OVERLAP_ACCUMULATE);

  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto));
  EXPECT_EQ(callback_count, 1);
}

TEST(BrushTest, EncodeBrushFamilyTextureMap) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.25f},
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId1),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .wrap_y = BrushPaint::TextureWrap::kMirror,
                           .size = {10, 15},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn},
                          {.client_texture_id = std::string(kTestTextureId2),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .wrap_y = BrushPaint::TextureWrap::kMirror,
                           .size = {10, 15},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn},
                          {.client_texture_id = "unknown",
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .wrap_y = BrushPaint::TextureWrap::kMirror,
                           .size = {10, 15},
                           .blend_mode = BrushPaint::BlendMode::kSrcIn}},
       .self_overlap = BrushPaint::SelfOverlap::kDiscard});
  ASSERT_EQ(family.status(), absl::OkStatus());
  google::protobuf::Map<std::string, std::string> texture_id_to_bitmap_proto_out;
  int distinct_texture_ids_count = 0;
  TextureBitmapProvider callback =
      [&distinct_texture_ids_count](
          absl::string_view id) -> std::optional<std::string> {
    distinct_texture_ids_count++;
    if (kTestTextureId1 == id) {
      return TestPngBytes1x1();
    } else if (kTestTextureId2 == id) {
      return TestPngBytes2x2();
    }
    return std::nullopt;
  };
  EncodeBrushFamilyTextureMap(*family, texture_id_to_bitmap_proto_out,
                              callback);
  EXPECT_EQ(texture_id_to_bitmap_proto_out.size(), 2);

  auto expected_bitmap_proto_1 = TestPngBytes1x1();
  EXPECT_EQ(texture_id_to_bitmap_proto_out.at(kTestTextureId1),
            expected_bitmap_proto_1);

  auto expected_bitmap_proto_2 = TestPngBytes2x2();
  EXPECT_EQ(texture_id_to_bitmap_proto_out.at(kTestTextureId2),
            expected_bitmap_proto_2);
  EXPECT_EQ(distinct_texture_ids_count, 3);
}

TEST(BrushTest, EncodeBrushFamilyTextureMapWithNonEmptyProto) {
  absl::StatusOr<BrushFamily> family = BrushFamily();
  ASSERT_EQ(family.status(), absl::OkStatus());
  google::protobuf::Map<std::string, std::string> texture_id_to_bitmap_proto_out;
  texture_id_to_bitmap_proto_out.insert({"existing_id", TestPngBytes1x1()});

  int callback_count = 0;
  TextureBitmapProvider callback =
      [&callback_count](absl::string_view id) -> std::optional<std::string> {
    callback_count++;
    return std::nullopt;
  };

  EncodeBrushFamilyTextureMap(*family, texture_id_to_bitmap_proto_out,
                              callback);
  EXPECT_EQ(texture_id_to_bitmap_proto_out.size(), 0);
  EXPECT_EQ(callback_count, 0);
}

TEST(BrushTest, EncodeBrushFamilyIntoNonEmptyProto) {
  // Create a brush family with no ID.
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.25f},
      {.texture_layers = {{.client_texture_id = std::string(kTestTextureId1),
                           .mapping = BrushPaint::TextureMapping::kStamping,
                           .size_unit = BrushPaint::TextureSizeUnit::kBrushSize,
                           .size = {10, 15}}}});
  ASSERT_EQ(family.status(), absl::OkStatus());
  // Initialize the proto with a non-empty ID, and a different brush tip.
  proto::BrushFamily family_proto_out;
  family_proto_out.add_coats()->mutable_tip()->set_corner_rounding(1.0f);
  family_proto_out.set_client_brush_family_id("marker");

  EncodeBrushFamily(*family, family_proto_out);

  // After encoding, the old tip proto should be replaced, and the old ID
  // should get cleared.
  ASSERT_EQ(family_proto_out.coats_size(), 1u);
  EXPECT_EQ(family_proto_out.coats(0).tip().corner_rounding(), 0.25f);
  EXPECT_FALSE(family_proto_out.has_client_brush_family_id());
}

TEST(BrushTest, DecodeBrushFamilyWithNoInputModel) {
  proto::BrushFamily family_proto;
  family_proto.add_coats()->mutable_tip();
  absl::StatusOr<BrushFamily> family = DecodeBrushFamily(family_proto);
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(family->GetInputModel(),
              BrushFamilyInputModelEq(BrushFamily::DefaultInputModel()));
}

TEST(BrushTest, DecodeBrushFamilyReturnsErrorStatusFromCallback) {
  absl::Status error_status = absl::InternalError("test error");
  ClientTextureIdProviderAndBitmapReceiver callback =
      [&error_status](absl::string_view,
                      absl::string_view) -> absl::StatusOr<std::string> {
    return error_status;
  };
  proto::BrushFamily family_proto;
  family_proto.add_coats()
      ->add_paint_preferences()
      ->add_texture_layers()
      ->set_client_texture_id(kTestTextureId1);
  absl::StatusOr<BrushFamily> family =
      DecodeBrushFamily(family_proto, callback);
  ASSERT_THAT(family, StatusIs(absl::StatusCode::kInternal, "test error"));
}

TEST(BrushTest, DecodeBrushPaintReturnsErrorStatusFromCallback) {
  absl::Status error_status = absl::InternalError("test error");
  ClientTextureIdProvider callback =
      [&error_status](
          absl::string_view encoded_id) -> absl::StatusOr<std::string> {
    return error_status;
  };
  proto::BrushPaint paint_proto;
  paint_proto.add_texture_layers()->set_client_texture_id(kTestTextureId1);
  absl::StatusOr<BrushPaint> paint = DecodeBrushPaint(paint_proto, callback);
  ASSERT_THAT(paint, StatusIs(absl::StatusCode::kInternal, "test error"));
}

TEST(BrushTest, EncodeBrushPaintWithInvalidTextureMapping) {
  BrushPaint paint;
  paint.texture_layers.push_back(
      {.client_texture_id = std::string(kTestTextureId1),
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
      {.client_texture_id = std::string(kTestTextureId1),
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
  ASSERT_EQ(behavior_proto.nodes_size(), 3);
  EXPECT_TRUE(behavior_proto.nodes(0).has_source_node());
  EXPECT_EQ(behavior_proto.nodes(0).source_node().source(),
            proto::BrushBehavior::SOURCE_UNSPECIFIED);
  EXPECT_EQ(
      behavior_proto.nodes(0).source_node().source_out_of_range_behavior(),
      proto::BrushBehavior::OUT_OF_RANGE_UNSPECIFIED);
  EXPECT_TRUE(behavior_proto.nodes(1).has_response_node());
  EXPECT_TRUE(
      behavior_proto.nodes(1).response_node().has_predefined_response_curve());
  EXPECT_EQ(behavior_proto.nodes(1).response_node().predefined_response_curve(),
            proto::PREDEFINED_EASING_UNSPECIFIED);
  EXPECT_TRUE(behavior_proto.nodes(2).has_target_node());
  EXPECT_EQ(behavior_proto.nodes(2).target_node().target(),
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

TEST(BrushTest, EncodeBrushBehaviorDampingNodeWithInvalidProgressDomain) {
  BrushBehavior::DampingNode node{
      .damping_source = static_cast<BrushBehavior::ProgressDomain>(123),
      .damping_gap = 1.0f,
  };
  proto::BrushBehavior::Node node_proto;
  EncodeBrushBehaviorNode(node, node_proto);
  EXPECT_TRUE(node_proto.has_damping_node());
  EXPECT_EQ(node_proto.damping_node().damping_source(),
            proto::BrushBehavior::PROGRESS_DOMAIN_UNSPECIFIED);
  EXPECT_EQ(node_proto.damping_node().damping_gap(), 1.0f);
}

TEST(BrushTest, EncodeBrushBehaviorWithEmptyDeveloperComment) {
  BrushBehavior behavior = {};
  proto::BrushBehavior behavior_proto;
  behavior_proto.set_developer_comment("foobar");
  EncodeBrushBehavior(behavior, behavior_proto);
  EXPECT_FALSE(behavior_proto.has_developer_comment());
}

void EncodeDecodeBrushRoundTrip(const Brush& brush_in) {
  int encode_callback_count = 0;
  int decode_callback_count = 0;
  TextureBitmapProvider encode_callback =
      [&encode_callback_count](
          absl::string_view id) -> std::optional<std::string> {
    encode_callback_count++;
    return std::nullopt;
  };
  ClientTextureIdProviderAndBitmapReceiver decode_callback =
      [&decode_callback_count](
          absl::string_view id,
          absl::string_view bitmap) -> absl::StatusOr<std::string> {
    decode_callback_count++;
    return std::string(id);
  };
  proto::Brush brush_proto_in;
  EncodeBrush(brush_in, brush_proto_in, encode_callback);

  // Fuzz tests will generate brushes with development versions, so
  // max_version must be kDevelopment.
  absl::StatusOr<Brush> brush_out =
      DecodeBrush(brush_proto_in, decode_callback, Version::kDevelopment());
  ASSERT_EQ(brush_out.status(), absl::OkStatus());
  EXPECT_THAT(*brush_out, BrushEq(brush_in));
  EXPECT_EQ(encode_callback_count, decode_callback_count);

  encode_callback_count = 0;  // Reset the callback count.
  proto::Brush brush_proto_out;
  EncodeBrush(*brush_out, brush_proto_out, encode_callback);
  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto_in));
  EXPECT_EQ(encode_callback_count, decode_callback_count);
}
FUZZ_TEST(BrushTest, EncodeDecodeBrushRoundTrip)
    .WithDomains(SerializableBrush());

void EncodeDecodeBrushFamilyRoundTrip(const BrushFamily& family_in) {
  proto::BrushFamily family_proto_in;
  EncodeBrushFamily(family_in, family_proto_in);

  // Fuzz tests will generate brushes with development versions, so
  // max_version must be kDevelopment.
  absl::StatusOr<BrushFamily> family_out =
      DecodeBrushFamily(family_proto_in, Version::kDevelopment());
  ASSERT_EQ(family_out.status(), absl::OkStatus());
  EXPECT_THAT(*family_out, BrushFamilyEq(family_in));

  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto_in));
}
FUZZ_TEST(BrushTest, EncodeDecodeBrushFamilyRoundTrip)
    .WithDomains(SerializableBrushFamily());

void EncodeDecodeMultipleBrushFamiliesRoundTrip(
    const std::vector<BrushFamily>& families_in) {
  // Filter out duplicate versions.
  absl::flat_hash_set<int32_t> seen_versions;
  std::vector<BrushFamily> families_in_filtered;
  for (const BrushFamily& family : families_in) {
    int32_t version = family.CalculateMinimumRequiredVersion().value();
    if (seen_versions.contains(version)) {
      continue;
    }
    seen_versions.insert(version);
    families_in_filtered.push_back(family);
  }
  // We can pass in all the families, or just the filtered ones, and the
  // result should be the same.
  proto::BrushFamily family_proto_in;
  EncodeMultipleBrushFamilies(families_in, family_proto_in);
  proto::BrushFamily family_proto_in_filtered;
  EncodeMultipleBrushFamilies(families_in_filtered, family_proto_in_filtered);
  EXPECT_THAT(family_proto_in_filtered, EqualsProto(family_proto_in));
  EXPECT_THAT(family_proto_in_filtered.newer_brush_families(),
              SizeIs(families_in_filtered.size() - 1));

  // Fuzz tests will generate brushes with development versions, so
  // max_version must be kDevelopment.
  absl::StatusOr<std::vector<BrushFamily>> families_out =
      DecodeMultipleBrushFamilies(family_proto_in, Version::kDevelopment());
  // Compare against the filtered families, as encoding will skip duplicates.
  std::vector<::testing::Matcher<BrushFamily>> families_in_filtered_matchers;
  for (const BrushFamily& family : families_in_filtered) {
    families_in_filtered_matchers.push_back(BrushFamilyEq(family));
  }
  ASSERT_THAT(families_out, IsOk());
  EXPECT_THAT(*families_out,
              UnorderedElementsAreArray(families_in_filtered_matchers));

  proto::BrushFamily family_proto_out;
  EncodeMultipleBrushFamilies(*families_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto_in));

  // Now round trip using fallbacks, if possible.
  if (families_in_filtered.size() > 1) {
    for (const BrushFamily& f : families_in_filtered) {
      absl::StatusOr<BrushFamily> out = DecodeBrushFamily(
          family_proto_out, f.CalculateMinimumRequiredVersion());
      ASSERT_THAT(out, IsOk());
      EXPECT_THAT(*out, AllOf(BrushFamilyEq(f),
                              Property(&BrushFamily::HasFallbacks, true)));
      proto::BrushFamily out_proto;
      EncodeBrushFamily(*out, out_proto);
      EXPECT_THAT(out_proto, EqualsProto(family_proto_in));
    }
  } else {
    // If there's only one family, make sure it doesn't have fallbacks.
    EXPECT_THAT(family_proto_out.newer_brush_families(), IsEmpty());
    absl::StatusOr<BrushFamily> out =
        DecodeBrushFamily(family_proto_out, Version::kDevelopment());
    EXPECT_THAT(
        out, IsOkAndHolds(AllOf(BrushFamilyEq(families_in_filtered[0]),
                                Property(&BrushFamily::HasFallbacks, false))));
  }
}
FUZZ_TEST(BrushTest, EncodeDecodeMultipleBrushFamiliesRoundTrip)
    .WithDomains(
        fuzztest::NonEmpty(fuzztest::VectorOf(SerializableBrushFamily()))
            .WithMinSize(1)
            .WithMaxSize(10));

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
    .WithDomains(SerializableBrushCoat());

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
    .WithDomains(SerializableBrushPaint());

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
    .WithDomains(SerializableBrushTip());

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
    .WithDomains(SerializableBrushBehaviorNode());

void GetMaxProtoVersion(const google::protobuf::Message& message,
                        int32_t& max_version_out) {
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  if (descriptor->options().HasExtension(ink::proto::message_min_version)) {
    int32_t message_version =
        descriptor->options().GetExtension(ink::proto::message_min_version);
    EXPECT_THAT(message_version, Le(Version::kDevelopment().value()));
    max_version_out = std::max(max_version_out, message_version);
  }

  const google::protobuf::Reflection* reflection = message.GetReflection();
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  for (const auto* field : fields) {
    if (field->options().HasExtension(ink::proto::field_min_version)) {
      int32_t field_version =
          field->options().GetExtension(ink::proto::field_min_version);
      EXPECT_THAT(field_version, Le(Version::kDevelopment().value()));
      max_version_out = std::max(max_version_out, field_version);
    }

    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
      if (field->is_repeated()) {
        for (int i = 0; i < reflection->FieldSize(message, field); ++i) {
          const google::protobuf::EnumValueDescriptor* enum_value =
              reflection->GetRepeatedEnum(message, field, i);
          if (enum_value->options().HasExtension(
                  ink::proto::enum_value_min_version)) {
            int32_t enum_value_version = enum_value->options().GetExtension(
                ink::proto::enum_value_min_version);
            EXPECT_THAT(enum_value_version,
                        Le(Version::kDevelopment().value()));
            max_version_out = std::max(max_version_out, enum_value_version);
          }
        }
      } else {
        const google::protobuf::EnumValueDescriptor* enum_value =
            reflection->GetEnum(message, field);
        if (enum_value->options().HasExtension(
                ink::proto::enum_value_min_version)) {
          int32_t enum_value_version = enum_value->options().GetExtension(
              ink::proto::enum_value_min_version);
          EXPECT_THAT(enum_value_version, Le(Version::kDevelopment().value()));
          max_version_out = std::max(max_version_out, enum_value_version);
        }
      }
    } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      if (field->is_repeated()) {
        for (int i = 0; i < reflection->FieldSize(message, field); ++i) {
          GetMaxProtoVersion(reflection->GetRepeatedMessage(message, field, i),
                             max_version_out);
        }
      } else {
        GetMaxProtoVersion(reflection->GetMessage(message, field),
                           max_version_out);
      }
    }
  }
}

// Tests that CalculateMinimumRequiredVersion returns the same version as is
// annotated in the proto options corresponding to the messages, fields, and
// enums in the serialized form of the BrushFamily.
void CalculateMinimumRequiredVersionMatchesProtoOptions(
    const BrushFamily& family) {
  // Encoding uses CalculateMinimumRequiredVersion to set the min version.
  proto::BrushFamily family_proto;
  EncodeBrushFamily(family, family_proto);
  int32_t min_version_calculated = family_proto.min_version();
  EXPECT_THAT(min_version_calculated, Le(Version::kDevelopment().value()));

  int32_t min_version_from_options = Version::k0Jetpack1_0_0().value();
  GetMaxProtoVersion(family_proto, min_version_from_options);
  // Using reflection, examine the minimum required version of each
  // field/message/enum in the proto, and find the maximum.
  EXPECT_THAT(min_version_calculated, Eq(min_version_from_options));
}
FUZZ_TEST(BrushTest, CalculateMinimumRequiredVersionMatchesProtoOptions)
    .WithDomains(SerializableBrushFamily());

void CheckMinVersionExistsAndIsValid(
    const google::protobuf::Descriptor* descriptor,
    absl::flat_hash_set<const google::protobuf::Descriptor*>& visited_messages,
    absl::flat_hash_set<const google::protobuf::EnumDescriptor*>& visited_enums) {
  if (visited_messages.contains(descriptor) ||
      descriptor->file()->name() !=
          "third_party/ink/storage/proto/brush_family.proto" ||
      descriptor->options().map_entry()) {
    // Ignore messages that have already been visited, that are not in the brush
    // family proto file, or that are map entries (impossible to add options to,
    // since they are generated by the map field).
    return;
  }
  visited_messages.insert(descriptor);

  EXPECT_TRUE(
      descriptor->options().HasExtension(ink::proto::message_min_version))
      << "Message " << descriptor->full_name()
      << " is missing message_min_version option.";
  int32_t message_version =
      descriptor->options().GetExtension(ink::proto::message_min_version);
  if (message_version != Version::kDevelopment().value()) {
    EXPECT_THAT(message_version, Le(Version::kMaxSupported().value()));
  }

  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    EXPECT_TRUE(field->options().HasExtension(ink::proto::field_min_version))
        << "Field " << field->full_name()
        << " is missing field_min_version option.";
    int32_t field_version =
        field->options().GetExtension(ink::proto::field_min_version);
    if (field_version != Version::kDevelopment().value()) {
      EXPECT_THAT(field_version, Le(Version::kMaxSupported().value()));
    }
    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      CheckMinVersionExistsAndIsValid(field->message_type(), visited_messages,
                                      visited_enums);
    } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
      const google::protobuf::EnumDescriptor* enum_descriptor = field->enum_type();
      if (enum_descriptor->file()->name() !=
          "third_party/ink/storage/proto/brush_family.proto") {
        continue;
      }
      if (visited_enums.contains(enum_descriptor)) continue;
      visited_enums.insert(enum_descriptor);
      for (int j = 0; j < enum_descriptor->value_count(); ++j) {
        const google::protobuf::EnumValueDescriptor* enum_value =
            enum_descriptor->value(j);
        EXPECT_TRUE(enum_value->options().HasExtension(
            ink::proto::enum_value_min_version))
            << "Enum value " << enum_value->full_name()
            << " is missing enum_value_min_version option.";
        int32_t enum_value_version = enum_value->options().GetExtension(
            ink::proto::enum_value_min_version);
        if (enum_value_version != Version::kDevelopment().value()) {
          EXPECT_THAT(enum_value_version, Le(Version::kMaxSupported().value()));
        }
      }
    }
  }
}

TEST(BrushTest,
     AllBrushFamilyProtoMessagesFieldsAndEnumsHaveValidMinimumVersion) {
  // Ensure that all BrushFamily proto messages, fields, and
  // enums have a `minimum_version` option set, and it is valid, i.e.
  // less than or equal to Version::kMaxSupported, unless it equal to
  // kDevelopment. This ensures that they are well documented, valid to be
  // loaded, and will be covered by the
  // `CalculateMinimumRequiredVersionMatchesProtoOptions` test.
  absl::flat_hash_set<const google::protobuf::Descriptor*> visited_messages;
  absl::flat_hash_set<const google::protobuf::EnumDescriptor*> visited_enums;
  CheckMinVersionExistsAndIsValid(proto::BrushFamily::descriptor(),
                                  visited_messages, visited_enums);
}
TEST(BrushTest, DecodeBrushFamilyFailsWithOutOfRangeMinVersion) {
  // Test with kDevelopment (int32_t max). This is allowed by DecodeBrushFamily
  // only if max_version is set to kDevelopment.
  proto::BrushFamily family_proto;
  family_proto.set_min_version(Version::kDevelopment().value());

  EXPECT_THAT(
      DecodeBrushFamily(family_proto),
      StatusIs(absl::StatusCode::kInvalidArgument,
               absl::StrCat("Version must be less than or equal to ",
                            Version::kMaxSupported().ToFormattedString(),
                            ", but was ",
                            Version::kDevelopment().ToFormattedString())));

  // Now a version as close to Version::kMaxSupported as possible.
  family_proto.set_min_version(Version::kMaxSupported().value() + 1);

  EXPECT_THAT(
      DecodeBrushFamily(family_proto),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          absl::StrCat("Version must be less than or equal to ",
                       Version::kMaxSupported().ToFormattedString(),
                       ", but was ", (Version::kMaxSupported().value() + 1))));
}

TEST(BrushTest, DecodeBrushFamilyIsOkWithDevelopmentVersion) {
  proto::BrushFamily family_proto;
  family_proto.set_min_version(Version::kDevelopment().value());
  EXPECT_THAT(DecodeBrushFamily(family_proto, Version::kDevelopment()), IsOk());
}

TEST(BrushTest, DecodeBrushIsOkWithDevelopmentVersion) {
  proto::Brush brush_proto;
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  brush_proto.mutable_brush_family()->set_min_version(
      Version::kDevelopment().value());
  EXPECT_THAT(DecodeBrush(brush_proto, Version::kDevelopment()), IsOk());
}

TEST(BrushTest, DecodeBrushFamilyIsOkWithNoMinVersionSet) {
  proto::BrushFamily family_proto;
  EXPECT_THAT(DecodeBrushFamily(family_proto), IsOk());
}

TEST(BrushTest, DecodeBrushFamilyIsOkWithFallbacks) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString(R"pb(
                                    min_version: 0
                                    newer_brush_families { min_version: 1 }
                                    newer_brush_families { min_version: 2 }
                                  )pb",
                                  &family_proto));
  EXPECT_THAT(DecodeBrushFamily(family_proto, Version::k0Jetpack1_0_0()),
              IsOk());
  EXPECT_THAT(DecodeBrushFamily(family_proto, Version::k1Jetpack1_1_0Alpha01()),
              IsOk());
  EXPECT_THAT(DecodeBrushFamily(family_proto, Version::kDevelopment()), IsOk());
}

TEST(BrushTest, DecodeMultipleBrushFamiliesReturnsAllFallbacks) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString(R"pb(
                                    min_version: 0
                                    newer_brush_families { min_version: 1 }
                                    newer_brush_families { min_version: 2 }
                                  )pb",
                                  &family_proto));
  absl::StatusOr<std::vector<BrushFamily>> families =
      DecodeMultipleBrushFamilies(family_proto, Version::kDevelopment());
  EXPECT_THAT(families, IsOkAndHolds(SizeIs(3)));
}

TEST(BrushTest,
     DecodeMultipleBrushFamiliesFailsWithAnyNonCompatibleVersionFallback) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString(R"pb(
                                    min_version: 0
                                    newer_brush_families { min_version: 1 }
                                  )pb",
                                  &family_proto));
  EXPECT_THAT(
      DecodeMultipleBrushFamilies(family_proto, Version::k0Jetpack1_0_0()),
      StatusIs(absl::StatusCode::kInvalidArgument,
               absl::StrCat(
                   "Version must be less than or equal to ",
                   Version::k0Jetpack1_0_0().ToFormattedString(), ", but was ",
                   Version::k1Jetpack1_1_0Alpha01().ToFormattedString())));
}

TEST(BrushTest, DecodeBrushFamilyReturnsMaxCompatibleVersion) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString(R"pb(
                                    min_version: 0
                                    newer_brush_families { min_version: 1 }
                                    newer_brush_families {
                                      min_version: 2
                                      client_brush_family_id: "newest_family"
                                    }
                                  )pb",
                                  &family_proto));
  absl::StatusOr<BrushFamily> family =
      DecodeBrushFamily(family_proto, Version::kDevelopment());
  EXPECT_THAT(family, IsOkAndHolds(Property(
                          &BrushFamily::GetMetadata,
                          Field(&BrushFamily::Metadata::client_brush_family_id,
                                Eq("newest_family")))));
}

TEST(BrushTest, DecodeBrushFamilyFailsWithNoCompatibleVersion) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString(R"pb(
                                    min_version: 1
                                    newer_brush_families { min_version: 2 }
                                  )pb",
                                  &family_proto));
  EXPECT_THAT(
      DecodeBrushFamily(family_proto, Version::k0Jetpack1_0_0()),
      StatusIs(absl::StatusCode::kInvalidArgument,
               absl::StrCat(
                   "Version must be less than or equal to ",
                   Version::k0Jetpack1_0_0().ToFormattedString(), ", but was ",
                   Version::k1Jetpack1_1_0Alpha01().ToFormattedString())));
}

TEST(BrushTest, DecodeBrushFamilyIsOkWithLowestVersionBeingMaxVersion) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        min_version: 0
        newer_brush_families { min_version: 123456789 }
      )pb",
      &family_proto));
  absl::StatusOr<BrushFamily> family =
      DecodeBrushFamily(family_proto, Version::k0Jetpack1_0_0());
  EXPECT_THAT(family, IsOkAndHolds(Property(&BrushFamily::HasFallbacks, true)));
}

TEST(BrushTest, DecodeBrushFamilyPreservesOpaqueFallbacks) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        min_version: 0
        client_brush_family_id: "v0"
        newer_brush_families { min_version: 1 client_brush_family_id: "v1" }
        newer_brush_families { min_version: 2 client_brush_family_id: "v2" }
      )pb",
      &family_proto));

  // Decode with k1Jetpack1_1_0Alpha01 (should pick "v1")
  absl::StatusOr<BrushFamily> family =
      DecodeBrushFamily(family_proto, Version::k1Jetpack1_1_0Alpha01());
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(
      *family,
      AllOf(Property(&BrushFamily::GetMetadata,
                     Field(&BrushFamily::Metadata::client_brush_family_id,
                           Eq("v1"))),
            Property(&BrushFamily::HasFallbacks, true)));

  // Re-encode and verify result is identical to original proto
  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto));
}

TEST(BrushTest, DecodeBrushFamilyPreservesUnreadableFallbacks) {
  // Create proto from string with fields the deserializer/parser won't
  // understand.
  proto::BrushFamily family_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            min_version: 0
                                            client_brush_family_id: "v0"
                                            newer_brush_families {
                                              min_version: 1000
                                              client_brush_family_id: "v1000"
                                              # 9999: "Some unknown value"
                                            }
                                          )pb",
                                          &family_proto));

  family_proto.mutable_newer_brush_families(0)
      ->mutable_unknown_fields()
      ->AddLengthDelimited(
          /*field_number=*/9999, "Some unknown value");
  ASSERT_EQ(family_proto.newer_brush_families(0).unknown_fields().field_count(),
            1);
  // Decode with k0Jetpack1_0_0 (should pick "v0")
  absl::StatusOr<BrushFamily> family =
      DecodeBrushFamily(family_proto, Version::k0Jetpack1_0_0());
  ASSERT_THAT(family, IsOk());
  EXPECT_THAT(
      *family,
      AllOf(Property(&BrushFamily::GetMetadata,
                     Field(&BrushFamily::Metadata::client_brush_family_id,
                           Eq("v0"))),
            Property(&BrushFamily::HasFallbacks, true)));
  // Re-encode and verify result is identical to original proto
  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto));
  // Trying to deserialize with a version that would match the unknown fields
  // should succeed, but of course not decode the unknown fields.
  absl::StatusOr<BrushFamily> familyUnrecognized =
      DecodeBrushFamily(family_proto_out, Version::kDevelopment());
  ASSERT_THAT(familyUnrecognized, IsOk());
  EXPECT_THAT(
      *familyUnrecognized,
      AllOf(Property(&BrushFamily::GetMetadata,
                     Field(&BrushFamily::Metadata::client_brush_family_id,
                           Eq("v1000"))),
            Property(&BrushFamily::HasFallbacks, true)));
  // Re-encode and verify result is identical to original proto -- the
  // unrecognized fields are preserved by the stored proto bytes.
  proto::BrushFamily family_proto_out2;
  EncodeBrushFamily(*familyUnrecognized, family_proto_out2);
  EXPECT_THAT(family_proto_out2, EqualsProto(family_proto));
}

TEST(BrushTest, HasFallbacksReturnsExpectedValue) {
  BrushFamily family;
  EXPECT_FALSE(family.HasFallbacks());
  BrushFamilyInternalAccessor::SetOpaqueDecodedProtoBytesWithFallbacks("bytes",
                                                                       family);
  EXPECT_TRUE(family.HasFallbacks());
  BrushFamilyInternalAccessor::SetOpaqueDecodedProtoBytesWithFallbacks("",
                                                                       family);
  EXPECT_FALSE(family.HasFallbacks());
}

TEST(BrushTest, EncodeMultipleBrushFamiliesEncodesLowestVersionAtTopLevel) {
  proto::BrushFamily family_proto;
  BrushFamily family0 = BrushFamily();
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::kNormalizedPressure,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::IntegralNode{
                        .integrate_over =
                            BrushBehavior::ProgressDomain::kTimeInSeconds,
                        .integral_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .integral_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family1, IsOk());
  absl::StatusOr<BrushFamily> family2 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::
                            kTimeFromInputToStrokeEndInSeconds,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family2, IsOk());
  EncodeMultipleBrushFamilies({family0, *family1, *family2}, family_proto);
  EXPECT_THAT(family_proto.min_version(),
              Eq(Version::k0Jetpack1_0_0().value()));
  EXPECT_THAT(family_proto.newer_brush_families(), SizeIs(2));
  EXPECT_THAT(family_proto.newer_brush_families(0).min_version(),
              Eq(Version::k1Jetpack1_1_0Alpha01().value()));
  EXPECT_THAT(family_proto.newer_brush_families(1).min_version(),
              Eq(Version::kDevelopment().value()));
  // Reverse the order of the input -- top level should still be lowest version.
  // Order of newer_brush_families should be the same, as we sort them.
  proto::BrushFamily family_proto2;
  EncodeMultipleBrushFamilies({*family2, *family1, family0}, family_proto2);
  EXPECT_THAT(family_proto2.min_version(),
              Eq(Version::k0Jetpack1_0_0().value()));
  EXPECT_THAT(family_proto2.newer_brush_families(), SizeIs(2));
  EXPECT_THAT(family_proto2.newer_brush_families(0).min_version(),
              Eq(Version::k1Jetpack1_1_0Alpha01().value()));
  EXPECT_THAT(family_proto2.newer_brush_families(1).min_version(),
              Eq(Version::kDevelopment().value()));
  // Same exact proto output.
  EXPECT_THAT(family_proto, EqualsProto(family_proto2));
  EXPECT_THAT(family_proto.min_version(), Eq(family_proto2.min_version()));
}

TEST(BrushTest, EncodeMultipleBrushFamiliesIgnoresFallbacks) {
  proto::BrushFamily opaque_family0;
  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            client_brush_family_id: "opaque"
                                          )pb",
                                          &opaque_family0));
  absl::StatusOr<BrushFamily> family0 = BrushFamily::Create(
      BrushTip(), BrushPaint(), BrushFamily::InputModel(),
      BrushFamily::Metadata{.client_brush_family_id = "true family 0"});
  ASSERT_THAT(family0, IsOk());
  BrushFamilyInternalAccessor::SetOpaqueDecodedProtoBytesWithFallbacks(
      opaque_family0.SerializeAsString(), *family0);
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::
                            kTimeFromInputToStrokeEndInSeconds,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family1, IsOk());
  proto::BrushFamily family_proto;
  // Call to EncodeBrushFamily uses the fallbacks.
  EncodeBrushFamily(*family0, family_proto);
  EXPECT_THAT(family_proto, EqualsProto(opaque_family0));
  // Call to EncodeMultipleBrushFamilies does not use the existing fallbacks,
  // but rather creates new ones.
  EncodeMultipleBrushFamilies({*family0, *family1}, family_proto);
  EXPECT_THAT(family_proto.newer_brush_families(), SizeIs(1));
  absl::StatusOr<std::vector<BrushFamily>> families =
      DecodeMultipleBrushFamilies(family_proto, Version::kDevelopment());
  // Should equal the true family values, not the ignored fallbacks.
  EXPECT_THAT(families, IsOkAndHolds(AllOf(
                            SizeIs(2), ElementsAre(BrushFamilyEq(*family0),
                                                   BrushFamilyEq(*family1)))));
}

TEST(BrushTest, EncodeMultipleBrushFamiliesEmptyDoesntChangeProto) {
  proto::BrushFamily family_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(R"pb(
                                            min_version: 123456789
                                            client_brush_family_id: "123456789"
                                          )pb",
                                          &family_proto));
  EncodeMultipleBrushFamilies({}, family_proto);
  // Nothing to serialize, so no change to proto.
  EXPECT_THAT(family_proto.min_version(), Eq(123456789));
  EXPECT_THAT(family_proto.client_brush_family_id(), Eq("123456789"));
}

TEST(BrushTest, EncodeMultipleBrushFamiliesSingletonIsSameAsEncodeBrushFamily) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::
                            kTimeFromInputToStrokeEndInSeconds,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family, IsOk());
  proto::BrushFamily family_proto;
  EncodeBrushFamily(*family, family_proto);
  proto::BrushFamily family_proto2;
  EncodeMultipleBrushFamilies({*family}, family_proto2);
  EXPECT_THAT(family_proto, EqualsProto(family_proto2));
}

TEST(BrushTest, EncodeDecodeMultipleBrushFamiliesRoundTrip) {
  // Encode multiple brush families and decode them, making sure that the
  // output is the same as the input.
  proto::BrushFamily family_proto;
  BrushFamily family0 = BrushFamily();
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::kNormalizedPressure,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::IntegralNode{
                        .integrate_over =
                            BrushBehavior::ProgressDomain::kTimeInSeconds,
                        .integral_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .integral_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family1, IsOk());
  absl::StatusOr<BrushFamily> family2 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::
                            kTimeFromInputToStrokeEndInSeconds,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family2, IsOk());
  EncodeMultipleBrushFamilies({family0, *family1, *family2}, family_proto);
  absl::StatusOr<std::vector<BrushFamily>> families =
      DecodeMultipleBrushFamilies(family_proto, Version::kDevelopment());
  EXPECT_THAT(families, IsOkAndHolds(AllOf(
                            SizeIs(3), ElementsAre(BrushFamilyEq(family0),
                                                   BrushFamilyEq(*family1),
                                                   BrushFamilyEq(*family2)))));
}

TEST(BrushTest, EncodeMultipleDecodeSingleBrushFamilyRoundTrip) {
  // Encode multiple brush families and decode them at various versions, and
  // then encode them again. Regardless of the version deserializing, the
  // encoded output should be the same.
  proto::BrushFamily family_proto;
  BrushFamily family0 = BrushFamily();
  absl::StatusOr<BrushFamily> family1 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::kNormalizedPressure,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::IntegralNode{
                        .integrate_over =
                            BrushBehavior::ProgressDomain::kTimeInSeconds,
                        .integral_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .integral_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family1, IsOk());
  absl::StatusOr<BrushFamily> family2 = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::
                            kTimeFromInputToStrokeEndInSeconds,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family2, IsOk());
  EncodeMultipleBrushFamilies({family0, *family1, *family2}, family_proto);
  // Decode with kDevelopment (should pick family2)
  absl::StatusOr<BrushFamily> family2_out =
      DecodeBrushFamily(family_proto, Version::kDevelopment());
  ASSERT_THAT(family2_out, IsOk());
  EXPECT_THAT(*family2_out, BrushFamilyEq(*family2));
  // Re-encode and verify result is identical to original proto
  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family2_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto));
  // Decode with k1Jetpack1_1_0Alpha01 (should pick family1)
  absl::StatusOr<BrushFamily> family1_out =
      DecodeBrushFamily(family_proto, Version::k1Jetpack1_1_0Alpha01());
  ASSERT_THAT(family1_out, IsOk());
  EXPECT_THAT(*family1_out, BrushFamilyEq(*family1));
  // Re-encode and verify result is identical to original proto
  EncodeBrushFamily(*family1_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto));
  // Decode with k0Jetpack1_0_0 (should pick family0)
  absl::StatusOr<BrushFamily> family0_out =
      DecodeBrushFamily(family_proto, Version::k0Jetpack1_0_0());
  ASSERT_THAT(family0_out, IsOk());
  EXPECT_THAT(*family0_out, BrushFamilyEq(family0));
  // Re-encode and verify result is identical to original proto
  EncodeBrushFamily(*family0_out, family_proto_out);
  EXPECT_THAT(family_proto_out, EqualsProto(family_proto));

  // After all this, we can still pull all the brush families out.
  absl::StatusOr<std::vector<BrushFamily>> families =
      DecodeMultipleBrushFamilies(family_proto, Version::kDevelopment());
  EXPECT_THAT(families, IsOkAndHolds(AllOf(
                            SizeIs(3), ElementsAre(BrushFamilyEq(family0),
                                                   BrushFamilyEq(*family1),
                                                   BrushFamilyEq(*family2)))));
}

TEST(BrushTest, EncodeBrushFamilyWithMalformedProtoBytesLosesFallbacksButIsOk) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip({1, 1}, 1, Angle::Radians(0), 0, Angle::Radians(0), 0,
               Duration32::Zero(),
               {BrushBehavior(
                   {BrushBehavior::SourceNode{
                        .source = BrushBehavior::Source::kNormalizedPressure,
                        .source_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .source_value_range = {0, 1},
                    },
                    BrushBehavior::IntegralNode{
                        .integrate_over =
                            BrushBehavior::ProgressDomain::kTimeInSeconds,
                        .integral_out_of_range_behavior =
                            BrushBehavior::OutOfRange::kClamp,
                        .integral_value_range = {0, 1},
                    },
                    BrushBehavior::TargetNode{
                        .target = BrushBehavior::Target::kHeightMultiplier,
                        .target_modifier_range = {0, 1},
                    }})}),
      BrushPaint());
  ASSERT_THAT(family, IsOk());
  BrushFamilyInternalAccessor::SetOpaqueDecodedProtoBytesWithFallbacks(
      "invalid proto bytes", *family);
  EXPECT_TRUE(family->HasFallbacks());
  proto::BrushFamily family_proto_out;
  EncodeBrushFamily(*family, family_proto_out);
  EXPECT_THAT(family_proto_out.newer_brush_families(), IsEmpty());
  absl::StatusOr<BrushFamily> family_out =
      DecodeBrushFamily(family_proto_out, Version::kDevelopment());
  EXPECT_THAT(family_out,
              IsOkAndHolds(AllOf(BrushFamilyEq(*family),
                                 Property(&BrushFamily::HasFallbacks, false))));
}

}  // namespace
}  // namespace ink
