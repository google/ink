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

#include "ink/rendering/bitmap.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/rendering/fuzz_domains.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

TEST(BitmapTest, StringifyPixelFormat) {
  EXPECT_EQ(absl::StrCat(Bitmap::PixelFormat::kRgba8888), "kRgba8888");
  EXPECT_EQ(absl::StrCat(static_cast<Bitmap::PixelFormat>(91)),
            "PixelFormat(91)");
}

TEST(BitmapTest, PixelFormatBytesPerPixel) {
  EXPECT_EQ(PixelFormatBytesPerPixel(Bitmap::PixelFormat::kRgba8888), 4);
  EXPECT_DEATH_IF_SUPPORTED(
      PixelFormatBytesPerPixel(static_cast<Bitmap::PixelFormat>(91)),
      "Invalid");
}

TEST(BitmapTest, ValidateBitmap) {
  std::unique_ptr<Bitmap> valid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
  EXPECT_THAT(rendering_internal::ValidateBitmap(*valid_bitmap), IsOk());

  std::unique_ptr<Bitmap> invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/-1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
  EXPECT_THAT(rendering_internal::ValidateBitmap(*invalid_bitmap),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("width")));

  invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/1, /*height=*/0, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(
      rendering_internal::ValidateBitmap(*invalid_bitmap),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("height")));

  // 2^16 x 2^15 = 2^31, which overflows int32.
  invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/65536, /*height=*/32768, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(rendering_internal::ValidateBitmap(*invalid_bitmap),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("area overflows")));

  // 2^15 x 2^15 * 4 bytes/pixel = 2^32 bytes, which overflows int32.
  invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/32768, /*height=*/32768, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb, std::vector<uint8_t>{});
  EXPECT_THAT(rendering_internal::ValidateBitmap(*invalid_bitmap),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("pixel data size overflows")));

  invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/1, /*height=*/1, static_cast<Bitmap::PixelFormat>(123),
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
  EXPECT_THAT(
      rendering_internal::ValidateBitmap(*invalid_bitmap),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("pixel format")));

  invalid_bitmap = std::make_unique<VectorBitmap>(
      /*width=*/1, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kGammaEncoded, ColorSpace::kSrgb,
      std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78, 0x9a});
  EXPECT_THAT(rendering_internal::ValidateBitmap(*invalid_bitmap),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("expected 4 bytes")));
}

void CanValidateValidBitmap(std::unique_ptr<Bitmap> bitmap) {
  EXPECT_EQ(ink::rendering_internal::ValidateBitmap(*bitmap), absl::OkStatus());
}
FUZZ_TEST(ShaderCacheTest, CanValidateValidBitmap)
    .WithDomains(ValidBitmapWithMaxSize(100, 100));

}  // namespace
}  // namespace ink
