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
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/fuzz_domains.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/rendering/bitmap.h"
#include "ink/rendering/fuzz_domains.h"
#include "ink/rendering/texture_bitmap_store.h"
#include "ink/strokes/input/fuzz_domains.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkShader.h"

namespace ink::skia_native_internal {
namespace {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::IsNull;
using ::testing::NotNull;

constexpr absl::string_view kTestTextureId = "test";

// A TextureBitmapStore that always returns the same bitmap regardless of
// the texture id.
class FakeBitmapStore : public TextureBitmapStore {
 public:
  explicit FakeBitmapStore(std::shared_ptr<Bitmap> bitmap)
      : bitmap_(std::move(bitmap)) {}

  absl::StatusOr<absl::Nonnull<std::shared_ptr<Bitmap>>> GetTextureBitmap(
      absl::string_view texture_id) const override {
    return bitmap_;
  }

 private:
  std::shared_ptr<Bitmap> bitmap_;
};

TEST(ShaderCacheTest, GetShaderForEmptyBrushPaint) {
  ShaderCache cache(nullptr);
  absl::StatusOr<sk_sp<SkShader>> shader =
      cache.GetShaderForPaint(BrushPaint{}, 10, StrokeInputBatch());
  ASSERT_EQ(shader.status(), absl::OkStatus());
  EXPECT_THAT(*shader, IsNull());
}

TEST(ShaderCacheTest, TryGetTextureShaderWithoutTextureProvider) {
  ShaderCache cache(nullptr);
  absl::StatusOr<sk_sp<SkShader>> shader = cache.GetShaderForPaint(
      BrushPaint{{{.color_texture_id = std::string(kTestTextureId)}}}, 10,
      StrokeInputBatch());
  EXPECT_EQ(shader.status().code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_THAT(shader.status().message(),
              AllOf(HasSubstr("TextureBitmapStore"), HasSubstr("null")));
}

TEST(ShaderCacheTest, GetShaderForTexturedBrushPaint) {
  FakeBitmapStore provider(std::make_shared<VectorBitmap>(
      /*width=*/2, /*height=*/1, Bitmap::PixelFormat::kRgba8888,
      Color::Format::kLinear, ColorSpace::kDisplayP3,
      std::vector<uint8_t>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}));
  ShaderCache cache(&provider);
  absl::StatusOr<sk_sp<SkShader>> shader = cache.GetShaderForPaint(
      BrushPaint{{{.color_texture_id = std::string(kTestTextureId)}}}, 10,
      StrokeInputBatch());
  ASSERT_EQ(shader.status(), absl::OkStatus());
  ASSERT_THAT(*shader, NotNull());
  SkImage* image = (*shader)->isAImage(nullptr, nullptr);
  ASSERT_THAT(image, NotNull());
  EXPECT_EQ(image->width(), 2);
  EXPECT_EQ(image->height(), 1);
  EXPECT_EQ(image->alphaType(), SkAlphaType::kUnpremul_SkAlphaType);
  EXPECT_EQ(image->colorType(), SkColorType::kRGBA_8888_SkColorType);
  ASSERT_THAT(image->colorSpace(), NotNull());
  EXPECT_TRUE(image->colorSpace()->gammaIsLinear());
  EXPECT_FALSE(image->colorSpace()->isSRGB());
}

void CanGetShaderForAnyValidInputs(std::shared_ptr<Bitmap> bitmap,
                                   const BrushPaint& brush_paint,
                                   float brush_size,
                                   const StrokeInputBatch& inputs) {
  FakeBitmapStore provider(std::move(bitmap));
  ShaderCache cache(&provider);
  absl::StatusOr<sk_sp<SkShader>> shader =
      cache.GetShaderForPaint(brush_paint, brush_size, inputs);
  EXPECT_EQ(shader.status(), absl::OkStatus());
}
FUZZ_TEST(ShaderCacheTest, CanGetShaderForAnyValidInputs)
    .WithDomains(ValidBitmapWithMaxSize(100, 100), ValidBrushPaint(),
                 fuzztest::Positive<float>(), ArbitraryStrokeInputBatch());

}  // namespace
}  // namespace ink::skia_native_internal
