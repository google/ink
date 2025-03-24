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

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/base/nullability.h"
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
#include "ink/color/color_space.h"
#include "ink/geometry/vec.h"
#include "ink/rendering/bitmap.h"
#include "ink/storage/color.h"
#include "ink/storage/proto/bitmap.pb.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/coded.pb.h"
#include "ink/storage/proto/color.pb.h"
#include "ink/storage/proto_matchers.h"
#include "ink/types/duration.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsNull;
using ::testing::SizeIs;

constexpr absl::string_view kTestTextureId1 = "test-texture-one";
constexpr absl::string_view kTestTextureId2 = "test-texture-two";
constexpr absl::string_view kTestTextureId1Decoded = "test-texture-one-decoded";
constexpr absl::string_view kTestTextureId2Decoded = "test-texture-two-decoded";

VectorBitmap TestBitmap1x1() {
  return VectorBitmap(
      /*width=*/1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
}

VectorBitmap TestBitmap2x2() {
  return VectorBitmap(
      /*width=*/2, /*height=*/2, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12,
                           0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78});
}
proto::Bitmap TestBitmapProto1x1() {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(1);
  bitmap_proto.set_height(1);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  bitmap_proto.set_pixel_data(std::string(
      reinterpret_cast<const char*>(TestBitmap1x1().GetPixelData().data()),
      TestBitmap1x1().GetPixelData().size()));
  return bitmap_proto;
}
proto::Bitmap TestBitmapProto2x2() {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(2);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  bitmap_proto.set_pixel_data(std::string(
      reinterpret_cast<const char*>(TestBitmap2x2().GetPixelData().data()),
      TestBitmap2x2().GetPixelData().size()));
  return bitmap_proto;
}

TEST(BrushTest, DecodeBrushProto) {
  proto::Brush brush_proto;
  VectorBitmap test_bitmap_1 = TestBitmap1x1();
  VectorBitmap test_bitmap_2 = TestBitmap2x2();
  auto bitmap_proto_1 = TestBitmapProto1x1();
  auto bitmap_proto_2 = TestBitmapProto2x2();
  brush_proto.set_size_stroke_space(10);
  brush_proto.set_epsilon_stroke_space(1.1);
  brush_proto.mutable_color()->set_r(0);
  brush_proto.mutable_color()->set_g(1);
  brush_proto.mutable_color()->set_b(0);
  brush_proto.mutable_color()->set_a(1);
  brush_proto.mutable_color()->set_color_space(
      proto::ColorSpace::COLOR_SPACE_SRGB);
  proto::BrushFamily* family_proto = brush_proto.mutable_brush_family();
  family_proto->mutable_input_model()->mutable_spring_model();
  family_proto->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId1), bitmap_proto_1});
  family_proto->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId2), bitmap_proto_2});
  proto::BrushCoat* coat_proto = family_proto->add_coats();
  coat_proto->mutable_tip()->set_corner_rounding(0.5f);
  coat_proto->mutable_tip()->set_opacity_multiplier(1.0f);
  proto::BrushPaint* paint_proto = coat_proto->mutable_paint();
  proto::BrushPaint::TextureLayer* texture_layer_proto_1 =
      paint_proto->add_texture_layers();
  texture_layer_proto_1->set_client_texture_id(std::string(kTestTextureId1));
  texture_layer_proto_1->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  texture_layer_proto_1->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_1->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  texture_layer_proto_1->set_size_x(10);
  texture_layer_proto_1->set_size_y(15);
  proto::BrushPaint::TextureKeyframe* keyframe_proto_1 =
      texture_layer_proto_1->add_keyframes();
  keyframe_proto_1->set_progress(0.8f);
  keyframe_proto_1->set_size_x(10);
  keyframe_proto_1->set_size_y(15);
  keyframe_proto_1->set_opacity(0.6f);
  texture_layer_proto_1->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);
  proto::BrushPaint::TextureLayer* texture_layer_proto_2 =
      paint_proto->add_texture_layers();
  texture_layer_proto_2->set_client_texture_id(std::string(kTestTextureId2));
  texture_layer_proto_2->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  texture_layer_proto_2->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_2->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  texture_layer_proto_2->set_size_x(4);
  texture_layer_proto_2->set_size_y(10);
  proto::BrushPaint::TextureKeyframe* keyframe_proto_2 =
      texture_layer_proto_2->add_keyframes();
  keyframe_proto_2->set_progress(0.5f);
  keyframe_proto_2->set_size_x(10);
  keyframe_proto_2->set_size_y(15);
  keyframe_proto_2->set_opacity(0.6f);
  texture_layer_proto_2->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);
  proto::BrushPaint::TextureLayer* texture_layer_proto_3 =
      paint_proto->add_texture_layers();
  texture_layer_proto_3->set_client_texture_id(std::string(kTestTextureId1));
  texture_layer_proto_3->set_mapping(
      proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  texture_layer_proto_3->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_FIRST_STROKE_INPUT);
  texture_layer_proto_3->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  texture_layer_proto_3->set_size_x(1);
  texture_layer_proto_3->set_size_y(2);
  proto::BrushPaint::TextureKeyframe* keyframe_proto_3 =
      texture_layer_proto_3->add_keyframes();
  keyframe_proto_3->set_progress(0.3f);
  keyframe_proto_3->set_size_x(10);
  keyframe_proto_3->set_size_y(15);
  keyframe_proto_3->set_opacity(0.6f);
  texture_layer_proto_3->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_DST_OUT);

  // Expected brush family.
  absl::StatusOr<BrushFamily> expected_family = BrushFamily::Create(
      BrushTip{.corner_rounding = 0.5f},
      {.texture_layers = {
           {.client_texture_id = std::string(kTestTextureId1Decoded),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .size = {10, 15},
            .keyframes = {{.progress = 0.8f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kDstOut},
           {.client_texture_id = std::string(kTestTextureId2Decoded),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .size = {4, 10},
            .keyframes = {{.progress = 0.5f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kDstOut},
           {.client_texture_id = std::string(kTestTextureId1Decoded),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .origin = BrushPaint::TextureOrigin::kFirstStrokeInput,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .size = {1, 2},
            .keyframes = {{.progress = 0.3f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kDstOut}}});

  ASSERT_EQ(expected_family.status(), absl::OkStatus());
  absl::StatusOr<Brush> expected_brush =
      Brush::Create(*expected_family, Color::Green(), 10, 1.1);
  ASSERT_EQ(expected_brush.status(), absl::OkStatus());

  std::map<std::string, VectorBitmap> decoded_bitmaps = {};
  std::function<std::string(const std::string& encoded_id,
                            absl::Nullable<VectorBitmap*> bitmap)>
      callback = [&decoded_bitmaps](
                     const std::string& id,
                     absl::Nullable<VectorBitmap*> bitmap) -> std::string {
    std::string new_id = "";
    if (id == kTestTextureId1) {
      new_id = kTestTextureId1Decoded;
    } else if (id == kTestTextureId2) {
      new_id = kTestTextureId2Decoded;
    }
    if (bitmap != nullptr) {
      if (decoded_bitmaps.find(new_id) == decoded_bitmaps.end()) {
        VectorBitmap bitmap_copy = *bitmap;
        decoded_bitmaps.insert({new_id, std::move(bitmap_copy)});
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
  VectorBitmap decoded_bitmap_1 =
      decoded_bitmaps.at(std::string(kTestTextureId1Decoded));
  EXPECT_EQ(decoded_bitmap_1.width(), test_bitmap_1.width());
  EXPECT_EQ(decoded_bitmap_1.height(), test_bitmap_1.height());
  EXPECT_EQ(decoded_bitmap_1.pixel_format(), test_bitmap_1.pixel_format());
  EXPECT_EQ(decoded_bitmap_1.color_format(), test_bitmap_1.color_format());
  EXPECT_EQ(decoded_bitmap_1.color_space(), test_bitmap_1.color_space());
  EXPECT_EQ(decoded_bitmap_1.GetPixelData(), test_bitmap_1.GetPixelData());

  EXPECT_NE(decoded_bitmaps.find(std::string(kTestTextureId2Decoded)),
            decoded_bitmaps.end());
  VectorBitmap decoded_bitmap_2 =
      decoded_bitmaps.at(std::string(kTestTextureId2Decoded));
  EXPECT_EQ(decoded_bitmap_2.width(), test_bitmap_2.width());
  EXPECT_EQ(decoded_bitmap_2.height(), test_bitmap_2.height());
  EXPECT_EQ(decoded_bitmap_2.pixel_format(), test_bitmap_2.pixel_format());
  EXPECT_EQ(decoded_bitmap_2.color_format(), test_bitmap_2.color_format());
  EXPECT_EQ(decoded_bitmap_2.color_space(), test_bitmap_2.color_space());
  EXPECT_EQ(decoded_bitmap_2.GetPixelData(), test_bitmap_2.GetPixelData());
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
    texture_layer_proto->set_client_texture_id(std::string(kTestTextureId1));
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
    texture_layer_proto->set_client_texture_id(std::string(kTestTextureId1));
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

TEST(BrushTest, DecodeBrushPaintWithInconsistentAnimationFrames) {
  // The proto is valid, as are the individual texture layers, but the top-level
  // BrushPaint fails validation.
  proto::BrushPaint paint_proto;
  proto::BrushPaint::TextureLayer* texture_layer1 =
      paint_proto.add_texture_layers();
  texture_layer1->set_size_x(10);
  texture_layer1->set_size_y(15);
  texture_layer1->set_animation_frames(1);
  proto::BrushPaint::TextureLayer* texture_layer2 =
      paint_proto.add_texture_layers();
  texture_layer2->set_size_x(10);
  texture_layer2->set_size_y(15);
  texture_layer2->set_animation_frames(2);
  EXPECT_THAT(DecodeBrushPaint(paint_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("`BrushPaint::TextureLayer::animation_frames` "
                                 "must be the same")));
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
          .opacity_multiplier = 0.7f,
          .particle_gap_distance_scale = 1,
          .particle_gap_duration = Duration32::Seconds(2),
      },
      {.texture_layers = {
           {.client_texture_id = std::string(kTestTextureId1),
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
  int callback_count = 0;
  std::function<std::optional<VectorBitmap>(const std::string& id)> callback =
      [&callback_count](const std::string& id) {
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
      ->mutable_spring_model();
  proto::BrushCoat* coat_proto =
      brush_proto.mutable_brush_family()->add_coats();
  coat_proto->mutable_tip()->set_scale_x(1.f);
  coat_proto->mutable_tip()->set_scale_y(1.f);
  coat_proto->mutable_tip()->set_corner_rounding(0.25f);
  coat_proto->mutable_tip()->set_slant_radians(0.f);
  coat_proto->mutable_tip()->set_pinch(0.f);
  coat_proto->mutable_tip()->set_rotation_radians(0.f);
  coat_proto->mutable_tip()->set_opacity_multiplier(0.7f);
  coat_proto->mutable_tip()->set_particle_gap_distance_scale(1);
  coat_proto->mutable_tip()->set_particle_gap_duration_seconds(2);
  proto::BrushPaint::TextureLayer* layer_proto =
      coat_proto->mutable_paint()->add_texture_layers();
  layer_proto->set_client_texture_id("test-texture-one");
  layer_proto->set_mapping(proto::BrushPaint::TextureLayer::MAPPING_WINDING);
  layer_proto->set_origin(
      proto::BrushPaint::TextureLayer::ORIGIN_STROKE_SPACE_ORIGIN);
  layer_proto->set_size_x(10);
  layer_proto->set_size_y(15);
  layer_proto->set_size_unit(
      proto::BrushPaint::TextureLayer::SIZE_UNIT_STROKE_SIZE);
  layer_proto->set_wrap_x(proto::BrushPaint::TextureLayer::WRAP_REPEAT);
  layer_proto->set_wrap_y(proto::BrushPaint::TextureLayer::WRAP_MIRROR);
  layer_proto->set_offset_x(0.f);
  layer_proto->set_offset_y(0.f);
  layer_proto->set_rotation_in_radians(0.f);
  layer_proto->set_size_jitter_x(3.f);
  layer_proto->set_size_jitter_y(7.f);
  layer_proto->set_offset_jitter_x(0.f);
  layer_proto->set_offset_jitter_y(0.f);
  layer_proto->set_rotation_jitter_in_radians(0.f);
  layer_proto->set_opacity(1.f);
  layer_proto->set_animation_frames(2);
  proto::BrushPaint::TextureKeyframe* keyframe_proto =
      layer_proto->add_keyframes();
  keyframe_proto->set_progress(0.8f);
  keyframe_proto->set_size_x(10);
  keyframe_proto->set_size_y(15);
  keyframe_proto->set_opacity(0.6f);
  layer_proto->set_blend_mode(
      proto::BrushPaint::TextureLayer::BLEND_MODE_SRC_IN);

  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto));
  EXPECT_EQ(callback_count, 1);
}

TEST(BrushTest, EncodeBrushWithTextureMap) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{
          .corner_rounding = 0.25f,
          .opacity_multiplier = 0.7f,
          .particle_gap_distance_scale = 1,
          .particle_gap_duration = Duration32::Seconds(2),
      },
      {.texture_layers = {
           {.client_texture_id = std::string(kTestTextureId1),
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
  int callback_count = 0;
  std::function<std::optional<VectorBitmap>(const std::string& id)> callback =
      [&callback_count](
          const std::string& id) -> std::optional<ink::VectorBitmap> {
    callback_count++;
    if (kTestTextureId1 == id) {
      return TestBitmap1x1();
    } else if (kTestTextureId2 == id) {
      return TestBitmap2x2();
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
      ->mutable_spring_model();
  brush_proto.mutable_brush_family()->mutable_texture_id_to_bitmap()->insert(
      {std::string(kTestTextureId1), TestBitmapProto1x1()});
  proto::BrushTip* tip_proto =
      brush_proto.mutable_brush_family()->add_coats()->mutable_tip();
  tip_proto->set_scale_x(1.f);
  tip_proto->set_scale_y(1.f);
  tip_proto->set_corner_rounding(0.25f);
  tip_proto->set_slant_radians(0.f);
  tip_proto->set_pinch(0.f);
  tip_proto->set_rotation_radians(0.f);
  tip_proto->set_opacity_multiplier(0.7f);
  tip_proto->set_particle_gap_distance_scale(1);
  tip_proto->set_particle_gap_duration_seconds(2);
  proto::BrushPaint* paint_proto =
      brush_proto.mutable_brush_family()->mutable_coats(0)->mutable_paint();
  proto::BrushPaint::TextureLayer* texture_layer_proto =
      paint_proto->add_texture_layers();
  texture_layer_proto->set_client_texture_id(kTestTextureId1);
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
  EXPECT_EQ(callback_count, 1);
}

TEST(BrushTest, EncodeBrushFamilyTextureMap) {
  absl::StatusOr<BrushFamily> family = BrushFamily::Create(
      BrushTip{
          .corner_rounding = 0.25f,
          .opacity_multiplier = 0.7f,
      },
      {.texture_layers = {
           {.client_texture_id = std::string(kTestTextureId1),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .wrap_y = BrushPaint::TextureWrap::kMirror,
            .size = {10, 15},
            .size_jitter = {3, 7},
            .animation_frames = 2,
            .keyframes = {{.progress = 0.8f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kSrcIn},
           {.client_texture_id = std::string(kTestTextureId2),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
            .wrap_y = BrushPaint::TextureWrap::kMirror,
            .size = {10, 15},
            .size_jitter = {3, 7},
            .animation_frames = 2,
            .keyframes = {{.progress = 0.8f,
                           .size = std::optional<Vec>({10, 15}),
                           .opacity = std::optional<float>(0.6f)}},
            .blend_mode = BrushPaint::BlendMode::kSrcIn},
           {.client_texture_id = "unknown",
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
  ::google::protobuf::Map<std::string, ::ink::proto::Bitmap>
      texture_id_to_bitmap_proto_out;
  int distinct_texture_ids_count = 0;
  std::function<std::optional<VectorBitmap>(const std::string& id)> callback =
      [&distinct_texture_ids_count](
          const std::string& id) -> std::optional<ink::VectorBitmap> {
    distinct_texture_ids_count++;
    if (kTestTextureId1 == id) {
      return TestBitmap1x1();
    } else if (kTestTextureId2 == id) {
      return TestBitmap2x2();
    }
    return std::nullopt;
  };
  EncodeBrushFamilyTextureMap(*family, texture_id_to_bitmap_proto_out,
                              callback);
  EXPECT_EQ(texture_id_to_bitmap_proto_out.size(), 2);

  auto expected_bitmap_proto_1 = TestBitmapProto1x1();
  EXPECT_THAT(texture_id_to_bitmap_proto_out.at(kTestTextureId1),
              EqualsProto(expected_bitmap_proto_1));

  auto expected_bitmap_proto_2 = TestBitmapProto2x2();
  EXPECT_THAT(texture_id_to_bitmap_proto_out.at(kTestTextureId2),
              EqualsProto(expected_bitmap_proto_2));
  EXPECT_EQ(distinct_texture_ids_count, 3);
}

TEST(BrushTest, EncodeBrushFamilyTextureMapWithNonEmptyProto) {
  absl::StatusOr<BrushFamily> family = BrushFamily();
  ASSERT_EQ(family.status(), absl::OkStatus());
  ::google::protobuf::Map<std::string, ::ink::proto::Bitmap>
      texture_id_to_bitmap_proto_out;
  texture_id_to_bitmap_proto_out.insert(
      {"existing_id", ::ink::proto::Bitmap()});

  int callback_count = 0;
  std::function<std::optional<VectorBitmap>(const std::string& id)> callback =
      [&callback_count](
          const std::string& id) -> std::optional<ink::VectorBitmap> {
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
      {.texture_layers = {
           {.client_texture_id = std::string(kTestTextureId1),
            .mapping = BrushPaint::TextureMapping::kWinding,
            .size_unit = BrushPaint::TextureSizeUnit::kStrokeSize,
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
  EXPECT_TRUE(std::holds_alternative<BrushFamily::SpringModel>(
      family->GetInputModel()));
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
  int encode_callback_count = 0;
  int decode_callback_count = 0;
  std::function<std::optional<VectorBitmap>(const std::string& id)>
      encode_callback =
          [&encode_callback_count](
              const std::string& id) -> std::optional<VectorBitmap> {
    encode_callback_count++;
    return std::nullopt;
  };
  std::function<std::string(const std::string& encoded_id,
                            absl::Nullable<VectorBitmap*> bitmap)>
      decode_callback =
          [&decode_callback_count](
              const std::string& id,
              absl::Nullable<VectorBitmap*> bitmap) -> std::string {
    decode_callback_count++;
    return id;
  };
  proto::Brush brush_proto_in;
  EncodeBrush(brush_in, brush_proto_in, encode_callback);

  absl::StatusOr<Brush> brush_out =
      DecodeBrush(brush_proto_in, decode_callback);
  ASSERT_EQ(brush_out.status(), absl::OkStatus());
  EXPECT_THAT(*brush_out, BrushEq(brush_in));
  EXPECT_EQ(encode_callback_count, decode_callback_count);

  encode_callback_count = 0;  // Reset the callback count.
  proto::Brush brush_proto_out;
  EncodeBrush(*brush_out, brush_proto_out, encode_callback);
  EXPECT_THAT(brush_proto_out, EqualsProto(brush_proto_in));
  EXPECT_EQ(encode_callback_count, decode_callback_count);
}
FUZZ_TEST(BrushTest, EncodeDecodeBrushRoundTrip).WithDomains(ValidBrush());

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
    .WithDomains(ValidBrushFamily());

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
