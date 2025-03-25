

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

#include "ink/storage/bitmap.h"

#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/match.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/rendering/bitmap.h"
#include "ink/rendering/fuzz_domains.h"
#include "ink/storage/proto/bitmap.pb.h"
#include "ink/storage/proto/color.pb.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

std::vector<uint8_t> TestPixelData2x4Rgba8888() {
  return std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
}
std::vector<uint8_t> TestPixelData1x3Rgba8888() {
  return std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
}

TEST(BitmapTest, EncodePixelFormat) {
  EXPECT_EQ(EncodePixelFormat(Bitmap::PixelFormat::kRgba8888),
            proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  EXPECT_DEATH_IF_SUPPORTED(
      EncodePixelFormat(static_cast<Bitmap::PixelFormat>(-1)), "Unknown");
}

TEST(BitmapTest, DecodePixelFormat) {
  auto result = DecodePixelFormat(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  ASSERT_THAT(result, IsOk());
  EXPECT_EQ(*result, Bitmap::PixelFormat::kRgba8888);
}

TEST(BitmapTest, DecodePixelFormatWithInvalidValue) {
  auto result = DecodePixelFormat(proto::Bitmap::PIXEL_FORMAT_UNSPECIFIED);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_TRUE(absl::StrContains(result.status().message(), "PixelFormat"));
}

TEST(BitmapTest, EncodeBitmap2x4) {
  const std::vector<uint8_t> pixel_data = TestPixelData2x4Rgba8888();
  VectorBitmap bitmap(2, 4, Bitmap::PixelFormat::kRgba8888,
                      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
                      pixel_data);
  proto::Bitmap bitmap_proto;
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto), IsOk());
  EXPECT_EQ(bitmap_proto.width(), 2);
  EXPECT_EQ(bitmap_proto.height(), 4);
  EXPECT_EQ(bitmap_proto.pixel_format(), proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  EXPECT_EQ(bitmap_proto.color_space(), proto::ColorSpace::COLOR_SPACE_SRGB);
  EXPECT_EQ(bitmap_proto.pixel_data(),
            absl::string_view(reinterpret_cast<const char*>(pixel_data.data()),
                              pixel_data.size()));
}

TEST(BitmapTest, EncodeBitmap1x3) {
  const std::vector<uint8_t> pixel_data = TestPixelData1x3Rgba8888();
  VectorBitmap bitmap(1, 3, Bitmap::PixelFormat::kRgba8888,
                      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
                      pixel_data);
  proto::Bitmap bitmap_proto;
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto), IsOk());
  EXPECT_EQ(bitmap_proto.width(), 1);
  EXPECT_EQ(bitmap_proto.height(), 3);
  EXPECT_EQ(bitmap_proto.pixel_format(), proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  EXPECT_EQ(bitmap_proto.color_space(), proto::ColorSpace::COLOR_SPACE_SRGB);
  EXPECT_EQ(bitmap_proto.pixel_data(),
            std::string_view(reinterpret_cast<const char*>(pixel_data.data()),
                             pixel_data.size()));
}

TEST(BitmapTest, EncodeBitmapFailsOnInvalidBitmap) {
  proto::Bitmap bitmap_proto;

  VectorBitmap bitmap = VectorBitmap(
      /*width=*/-1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));

  bitmap = VectorBitmap(
      /*width=*/0, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));

  bitmap = VectorBitmap(
      /*width=*/1, /*height=*/-1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(
      EncodeBitmap(bitmap, bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));

  bitmap = VectorBitmap(
      /*width=*/1, /*height=*/0, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(
      EncodeBitmap(bitmap, bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));

  // 2^16 x 2^15 = 2^31, which overflows int32.
  bitmap = VectorBitmap(
      /*width=*/65536, /*height=*/32768, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("area overflows")));

  // 2^15 x 2^15 * 4 bytes/pixel = 2^32 bytes, which overflows int32.
  bitmap = VectorBitmap(
      /*width=*/32768, /*height=*/32768, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("pixel data size overflows")));

  bitmap = VectorBitmap(
      /*width=*/1, /*height=*/1, static_cast<Bitmap::PixelFormat>(123),
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(
      EncodeBitmap(bitmap, bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("pixel format")));

  bitmap = VectorBitmap(
      /*width=*/1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x10, 0x20});
  EXPECT_THAT(EncodeBitmap(bitmap, bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("expected 4 bytes")));
}

TEST(BitmapTest, DecodeBitmapProto2x4) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  const std::vector<uint8_t> pixel_data = TestPixelData2x4Rgba8888();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  VectorBitmap bitmap = DecodeBitmap(bitmap_proto).value();
  EXPECT_EQ(bitmap.width(), 2);
  EXPECT_EQ(bitmap.height(), 4);
  EXPECT_EQ(bitmap.pixel_format(), Bitmap::PixelFormat::kRgba8888);
  EXPECT_EQ(bitmap.color_space(), ColorSpace::kSrgb);
  EXPECT_EQ(bitmap.GetPixelData().size(), 32);
  EXPECT_EQ(bitmap.GetPixelData(), pixel_data);
}

TEST(BitmapTest, DecodeBitmapProto1x3) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(1);
  bitmap_proto.set_height(3);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  const std::vector<uint8_t> pixel_data = TestPixelData1x3Rgba8888();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  VectorBitmap bitmap = DecodeBitmap(bitmap_proto).value();
  EXPECT_EQ(bitmap.width(), 1);
  EXPECT_EQ(bitmap.height(), 3);
  EXPECT_EQ(bitmap.pixel_format(), Bitmap::PixelFormat::kRgba8888);
  EXPECT_EQ(bitmap.color_space(), ColorSpace::kSrgb);
  EXPECT_EQ(bitmap.GetPixelData().size(), 12);
  EXPECT_EQ(bitmap.GetPixelData(), pixel_data);
}

proto::Bitmap CreateValidTestBitmapProto2x4() {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  const std::vector<uint8_t> pixel_data = TestPixelData2x4Rgba8888();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());
  return bitmap_proto;
}

// This test ensures that all the DecodeBitmapProto* tests below that expect an
// InvalidArgumentError are actually testing the right thing.
TEST(BitmapTest, AssertThatCreateValidTestBitmapProto2x4Decodes) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  auto bitmap = DecodeBitmap(bitmap_proto);
  ASSERT_THAT(bitmap, IsOk());
  ASSERT_EQ(bitmap->width(), 2);
  ASSERT_EQ(bitmap->height(), 4);
  ASSERT_EQ(bitmap->pixel_format(), Bitmap::PixelFormat::kRgba8888);
  ASSERT_EQ(bitmap->color_space(), ColorSpace::kSrgb);
  ASSERT_EQ(bitmap->GetPixelData().size(), 32);
  ASSERT_EQ(bitmap->GetPixelData(), TestPixelData2x4Rgba8888());
}

TEST(BitmapTest, DecodeBitmapProtoInvalidPixelFormat) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_UNSPECIFIED);

  EXPECT_THAT(
      DecodeBitmap(bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("PixelFormat")));
}

TEST(BitmapTest, DecodeBitmapProtoInvalidColorSpaceDefaultsToSrgb) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_UNSPECIFIED);
  const std::vector<uint8_t> pixel_data = TestPixelData2x4Rgba8888();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_THAT(result.status(), IsOk());
  EXPECT_EQ(result->color_space(), ColorSpace::kSrgb);
}

TEST(BitmapTest, DecodeBitmapProtoInvalidPixelData) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  // 31 bytes instead of the expected 32.
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size() - 1);

  EXPECT_THAT(
      DecodeBitmap(bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("pixel data")));
}

TEST(BitmapTest, DecodeBitmapProtoZeroHeight) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_height(0);

  EXPECT_THAT(
      DecodeBitmap(bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));
}

TEST(BitmapTest, DecodeBitmapProtoNegativeHeight) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_height(-1);

  EXPECT_THAT(
      DecodeBitmap(bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));
}

TEST(BitmapTest, DecodeBitmapProtoMissingHeight) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.clear_height();

  EXPECT_THAT(
      DecodeBitmap(bitmap_proto),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));
}

TEST(BitmapTest, DecodeBitmapProtoZeroWidth) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_width(0);

  EXPECT_THAT(DecodeBitmap(bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));
}

TEST(BitmapTest, DecodeBitmapProtoNegativeWidth) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_width(-1);

  EXPECT_THAT(DecodeBitmap(bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));
}

TEST(BitmapTest, DecodeBitmapProtoMissingWidth) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.clear_width();

  EXPECT_THAT(DecodeBitmap(bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));
}

TEST(BitmapTest, DecodeBitmapProtoAreaOverflowsInt32) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_width(65536);   // 2^16
  bitmap_proto.set_height(32768);  // 2^15
  // Total area is 2^31, which overflows int32_t.

  EXPECT_THAT(DecodeBitmap(bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("area")));
}

TEST(BitmapTest, DecodeBitmapProtoPixelDataSizeOverflowsInt32) {
  proto::Bitmap bitmap_proto = CreateValidTestBitmapProto2x4();
  bitmap_proto.set_width(32768);   // 2^15
  bitmap_proto.set_height(32768);  // 2^15
  // Total area is 2^30. 4 bytes per pixel, so the data size is 2^32, which
  // overflows int32_t.
  EXPECT_THAT(DecodeBitmap(bitmap_proto),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("pixel data size")));
}

void EncodeDecodeBitmapRoundTrip(std::unique_ptr<Bitmap> bitmap) {}
FUZZ_TEST(BitmapTest, EncodeDecodeBitmapRoundTrip)
    .WithDomains(ValidBitmapWithMaxSize(128, 128));

}  // namespace
}  // namespace ink
