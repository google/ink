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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"

namespace ink {
namespace rendering_internal {
namespace {

bool IsValidPixelFormat(Bitmap::PixelFormat format) {
  switch (format) {
    case Bitmap::PixelFormat::kRgba8888:
      return true;
  }
  return false;
}

}  // namespace

absl::Status ValidateBitmap(const Bitmap& bitmap) {
  return ValidateBitmap(bitmap.width(), bitmap.height(), bitmap.pixel_format(),
                        bitmap.GetPixelData().size());
}

absl::Status ValidateBitmap(int width, int height,
                            Bitmap::PixelFormat pixel_format,
                            int pixel_data_size) {
  if (width <= 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap width must be positive; was ", width));
  }
  if (height <= 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap height must be positive; was ", height));
  }
  if (!IsValidPixelFormat(pixel_format)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap's pixel format is invalid value ",
                     static_cast<int>(pixel_format)));
  }

  if (width > std::numeric_limits<int32_t>::max() / height) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap area overflows int32; was ", width, "x", height));
  }
  int area = width * height;

  int bytes_per_pixel = PixelFormatBytesPerPixel(pixel_format);
  if (area > std::numeric_limits<int32_t>::max() / bytes_per_pixel) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap pixel data size overflows int32; was ", width, "x",
                     height, " @ ", bytes_per_pixel, " bytes per pixel"));
  }
  int expected_pixel_data_size = area * bytes_per_pixel;

  if (pixel_data_size != expected_pixel_data_size) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Bitmap pixel data has incorrect size; expected ",
        expected_pixel_data_size, " bytes for a ", width, "x", height, " ",
        pixel_format, " image, but was ", pixel_data_size, " bytes"));
  }
  return absl::OkStatus();
}

std::string ToFormattedString(Bitmap::PixelFormat format) {
  switch (format) {
    case Bitmap::PixelFormat::kRgba8888:
      return "kRgba8888";
  }
  return absl::StrCat("PixelFormat(", static_cast<int>(format), ")");
}

}  // namespace rendering_internal

Bitmap::Bitmap(int width, int height, PixelFormat pixel_format,
               Color::Format color_format, ColorSpace color_space)
    : width_(width),
      height_(height),
      pixel_format_(pixel_format),
      color_format_(color_format),
      color_space_(color_space) {}

VectorBitmap::VectorBitmap(int width, int height, PixelFormat pixel_format,
                           Color::Format color_format, ColorSpace color_space,
                           std::vector<uint8_t> pixel_data)
    : Bitmap(width, height, pixel_format, color_format, color_space),
      pixel_data_(std::move(pixel_data)) {}

int PixelFormatBytesPerPixel(Bitmap::PixelFormat format) {
  switch (format) {
    case Bitmap::PixelFormat::kRgba8888:
      return 4;
  }
  ABSL_LOG(FATAL) << "Invalid PixelFormat value: " << static_cast<int>(format);
}

}  // namespace ink
