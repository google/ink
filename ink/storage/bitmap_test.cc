

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

#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "ink/color/color_space.h"
#include "ink/rendering/bitmap.h"
#include "ink/rendering/fuzz_domains.h"
#include "ink/storage/proto/bitmap.pb.h"
#include "ink/storage/proto/color.pb.h"
namespace ink {
namespace {

TEST(BitmapTest, DecodeBitmapProto) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  VectorBitmap bitmap = DecodeBitmap(bitmap_proto).value();
  EXPECT_EQ(bitmap.width(), 2);
  EXPECT_EQ(bitmap.height(), 4);
  EXPECT_EQ(bitmap.pixel_format(), Bitmap::PixelFormat::kRgba8888);
  EXPECT_EQ(bitmap.color_space(), ColorSpace::kSrgb);
  EXPECT_EQ(bitmap.GetPixelData().size(), 32);
  EXPECT_EQ(bitmap.GetPixelData(), pixel_data);
}

TEST(BitmapTest, DecodeBitmapProtoInvalidPixelFormat) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_UNSPECIFIED);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_TRUE(absl::StrContains(result.status().message(), "PixelFormat"));
}

TEST(BitmapTest, DecodeBitmapProtoInvalidColorSpaceDefaultsToSrgb) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_UNSPECIFIED);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kOk);
  EXPECT_EQ(result.value().color_space(), ColorSpace::kSrgb);
}

TEST(BitmapTest, DecodeBitmapProtoInvalidPixelData) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size() - 1);

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_TRUE(absl::StrContains(result.status().message(), "pixel data"));
}

TEST(BitmapTest, DecodeBitmapProtoInvalidHeight) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(2);
  bitmap_proto.set_height(0);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_TRUE(absl::StrContains(result.status().message(), "height"));
}

TEST(BitmapTest, DecodeBitmapProtoInvalidWidth) {
  proto::Bitmap bitmap_proto;
  bitmap_proto.set_width(0);
  bitmap_proto.set_height(4);
  bitmap_proto.set_pixel_format(proto::Bitmap::PIXEL_FORMAT_RGBA8888);
  bitmap_proto.set_color_space(proto::ColorSpace::COLOR_SPACE_SRGB);
  auto pixel_data = std::vector<uint8_t>{
      0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56,
      0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34,
      0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
  };
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());

  auto result = DecodeBitmap(bitmap_proto);
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_TRUE(absl::StrContains(result.status().message(), "width"));
}

TEST(BitmapTest, EncodeBitmap) {}
void EncodeDecodeBitmapRoundTrip(std::unique_ptr<Bitmap> bitmap) {}
FUZZ_TEST(BitmapTest, EncodeDecodeBitmapRoundTrip)
    .WithDomains(ValidBitmapWithMaxSize(128, 128));

}  // namespace
}  // namespace ink
