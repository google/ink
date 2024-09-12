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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/rendering/fuzz_domains.h"

namespace ink {
namespace {

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
  Bitmap valid_bitmap = {
      .data = {0x12, 0x34, 0x56, 0x78},
      .width = 1,
      .height = 1,
      .pixel_format = Bitmap::PixelFormat::kRgba8888,
      .color_format = Color::Format::kGammaEncoded,
      .color_space = ColorSpace::kSrgb,
  };
  EXPECT_EQ(rendering_internal::ValidateBitmap(valid_bitmap), absl::OkStatus());

  Bitmap invalid_bitmap = valid_bitmap;
  invalid_bitmap.width = -1;
  absl::Status status = rendering_internal::ValidateBitmap(invalid_bitmap);
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("width"));

  invalid_bitmap = valid_bitmap;
  invalid_bitmap.height = 0;
  status = rendering_internal::ValidateBitmap(invalid_bitmap);
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("height"));

  invalid_bitmap = valid_bitmap;
  invalid_bitmap.pixel_format = static_cast<Bitmap::PixelFormat>(123);
  status = rendering_internal::ValidateBitmap(invalid_bitmap);
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("pixel_format"));

  invalid_bitmap = valid_bitmap;
  invalid_bitmap.data.clear();
  status = rendering_internal::ValidateBitmap(invalid_bitmap);
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("should have size 4"));
}

void CanValidateValidBitmap(const Bitmap& bitmap) {
  EXPECT_EQ(ink::rendering_internal::ValidateBitmap(bitmap), absl::OkStatus());
}
FUZZ_TEST(ShaderCacheTest, CanValidateValidBitmap)
    .WithDomains(ValidBitmapWithMaxSize(100, 100));

}  // namespace
}  // namespace ink
