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

#include "ink/rendering/skia/native/internal/shader_cache.h"

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/type_matchers.h"
#include "ink/rendering/skia/native/texture_bitmap_store.h"
#include "ink/strokes/input/fuzz_domains.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkShader.h"

namespace ink::skia_native_internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::IsNull;
using ::testing::NotNull;

constexpr absl::string_view kTestTextureId = "test";

// A TextureBitmapStore that always returns the same bitmap regardless of
// the texture ID.
class FakeBitmapStore : public TextureBitmapStore {
 public:
  explicit FakeBitmapStore(sk_sp<SkImage> image) : image_(std::move(image)) {}

  absl::StatusOr<sk_sp<SkImage>> GetTextureBitmap(
      absl::string_view texture_id) const override {
    return image_;
  }

 private:
  sk_sp<SkImage> image_;
};

absl::StatusOr<sk_sp<SkImage>> CreateImageFromSrgbLinearPixelData(
    int width, int height, absl::Span<const uint8_t> pixel_data) {
  SkImageInfo image_info = SkImageInfo::Make(
      width, height, SkColorType::kRGBA_8888_SkColorType,
      SkAlphaType::kUnpremul_SkAlphaType, SkColorSpace::MakeSRGBLinear());
  ABSL_CHECK_EQ(pixel_data.size(), image_info.computeMinByteSize());
  SkBitmap skia_bitmap;
  bool success = skia_bitmap.tryAllocPixels(image_info);
  if (!success) {
    return absl::InternalError(absl::StrCat("failed to allocate pixels for ",
                                            width, "x", height, " SkImage"));
  }
  std::memcpy(skia_bitmap.getPixels(), pixel_data.data(), pixel_data.size());
  skia_bitmap.setImmutable();
  return skia_bitmap.asImage();
}

TEST(ShaderCacheTest, GetShaderForEmptyBrushPaint) {
  ShaderCache cache(nullptr);
  EXPECT_THAT(cache.GetShaderForPaint(BrushPaint{}, 10, StrokeInputBatch()),
              IsOkAndHolds(IsNull()));
}

TEST(ShaderCacheTest, TryGetTextureShaderWithoutTextureProvider) {
  ShaderCache cache(nullptr);
  EXPECT_THAT(
      cache.GetShaderForPaint(
          BrushPaint{{BrushPaint::TilingTexture{
              .client_texture_id = std::string(kTestTextureId)}}},
          10, StrokeInputBatch()),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("TextureBitmapStore"), HasSubstr("null"))));
}

TEST(ShaderCacheTest, GetShaderForTexturedBrushPaint) {
  auto test_image = CreateImageFromSrgbLinearPixelData(
      /*width=*/2, /*height=*/1,
      std::vector<uint8_t>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
  ASSERT_THAT(test_image, IsOk());
  FakeBitmapStore provider(*test_image);
  ShaderCache cache(&provider);
  absl::StatusOr<sk_sp<SkShader>> shader = cache.GetShaderForPaint(
      BrushPaint{{BrushPaint::TilingTexture{.client_texture_id =
                                                std::string(kTestTextureId)}}},
      10, StrokeInputBatch());
  ASSERT_THAT(shader, IsOkAndHolds(NotNull()));
  SkImage* image = (*shader)->isAImage(nullptr, nullptr);
  ASSERT_THAT(image, NotNull());
  EXPECT_EQ(image->width(), 2);
  EXPECT_EQ(image->height(), 1);
  EXPECT_EQ(image->alphaType(), SkAlphaType::kUnpremul_SkAlphaType);
  EXPECT_THAT(image->colorType(), SkColorType::kRGBA_8888_SkColorType);
  ASSERT_THAT(image->colorSpace(), NotNull());
  EXPECT_TRUE(image->colorSpace()->gammaIsLinear());
  EXPECT_FALSE(image->colorSpace()->isSRGB());
}

TEST(ShaderCacheTest, GetShaderForRotatedTilingTexture) {
  // Create a 2x1 image whose left texel is red and right texel is blue.
  absl::StatusOr<sk_sp<SkImage>> test_image =
      CreateImageFromSrgbLinearPixelData(
          /*width=*/2, /*height=*/1,
          std::vector<uint8_t>{0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff});
  ASSERT_THAT(test_image, IsOk());
  FakeBitmapStore provider(*test_image);
  ShaderCache cache(&provider);

  absl::StatusOr<sk_sp<SkShader>> shader = cache.GetShaderForPaint(
      BrushPaint{{BrushPaint::TilingTexture{
          .client_texture_id = std::string(kTestTextureId),
          .size_unit = BrushPaint::TextureSizeUnit::kStrokeCoordinates,
          .size = {10, 20},
          .offset = {0.25f, 0.5f},
          .rotation = Angle::Degrees(90),
      }}},
      /*brush_size=*/5, StrokeInputBatch());
  ASSERT_THAT(shader, IsOkAndHolds(NotNull()));

  SkMatrix local_matrix;
  ASSERT_THAT((*shader)->isAImage(&local_matrix, nullptr), NotNull());
  AffineTransform actual_transform(
      local_matrix.getScaleX(), local_matrix.getSkewX(),
      local_matrix.getTranslateX(), local_matrix.getSkewY(),
      local_matrix.getScaleY(), local_matrix.getTranslateY());
  AffineTransform expected_transform =
      AffineTransform::Scale(10, 20) *
      AffineTransform::RotateAboutPoint(Angle::Degrees(90), {0.5f, 0.5f}) *
      AffineTransform::Translate({0.25f, 0.5f}) *
      AffineTransform::Scale(0.5f, 1.0f);
  EXPECT_THAT(actual_transform, AffineTransformNear(expected_transform, 1e-5f));

  // Verify end-to-end rendering: rotating a 2x1 [red, blue] tile by +90 degrees
  // about the center of a 2x2 tile places red in the top row and blue in the
  // bottom row.
  absl::StatusOr<sk_sp<SkShader>> rotated_tile_shader = cache.GetShaderForPaint(
      BrushPaint{{BrushPaint::TilingTexture{
          .client_texture_id = std::string(kTestTextureId),
          .size_unit = BrushPaint::TextureSizeUnit::kStrokeCoordinates,
          .size = {2, 2},
          .rotation = Angle::Degrees(90),
      }}},
      /*brush_size=*/1, StrokeInputBatch());
  ASSERT_THAT(rotated_tile_shader, IsOkAndHolds(NotNull()));

  SkBitmap output;
  ASSERT_TRUE(output.tryAllocPixels(SkImageInfo::Make(
      2, 2, SkColorType::kRGBA_8888_SkColorType,
      SkAlphaType::kPremul_SkAlphaType, SkColorSpace::MakeSRGBLinear())));
  output.eraseColor(SK_ColorTRANSPARENT);
  SkCanvas canvas(output);
  SkPaint paint;
  paint.setShader(*rotated_tile_shader);
  canvas.drawPaint(paint);

  EXPECT_EQ(output.getColor(0, 0), SK_ColorRED);
  EXPECT_EQ(output.getColor(1, 0), SK_ColorRED);
  EXPECT_EQ(output.getColor(0, 1), SK_ColorBLUE);
  EXPECT_EQ(output.getColor(1, 1), SK_ColorBLUE);
}

void CanGetShaderForAnyValidInputs(const BrushPaint& brush_paint,
                                   float brush_size,
                                   const StrokeInputBatch& inputs) {
  auto test_image = CreateImageFromSrgbLinearPixelData(
      3, 2,
      {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc,
       0xdd, 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
  ASSERT_THAT(test_image, IsOk());
  FakeBitmapStore provider(*test_image);
  ShaderCache cache(&provider);
  EXPECT_THAT(cache.GetShaderForPaint(brush_paint, brush_size, inputs), IsOk());
}
FUZZ_TEST(ShaderCacheTest, CanGetShaderForAnyValidInputs)
    .WithDomains(ValidBrushPaint(), fuzztest::Positive<float>(),
                 ArbitraryStrokeInputBatch());

}  // namespace
}  // namespace ink::skia_native_internal
