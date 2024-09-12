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
#include <string>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

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
  if (bitmap.width <= 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("`Bitmap::width` must be positive; was ", bitmap.width));
  }
  if (bitmap.height <= 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Bitmap height must be positive; was ", bitmap.height));
  }
  if (!IsValidPixelFormat(bitmap.pixel_format)) {
    return absl::InvalidArgumentError(
        absl::StrCat("`Bitmap::pixel_format` holds non-enumerator value ",
                     static_cast<int>(bitmap.pixel_format)));
  }
  size_t expected_data_size = static_cast<size_t>(bitmap.width) *
                              static_cast<size_t>(bitmap.height) *
                              PixelFormatBytesPerPixel(bitmap.pixel_format);
  if (bitmap.data.size() != expected_data_size) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`Bitmap::data` should have size ", expected_data_size, " for a ",
        bitmap.width, "x", bitmap.height, " ", bitmap.pixel_format,
        " image, but has size ", bitmap.data.size()));
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

size_t PixelFormatBytesPerPixel(Bitmap::PixelFormat format) {
  switch (format) {
    case Bitmap::PixelFormat::kRgba8888:
      return 4;
  }
  LOG(FATAL) << "Invalid PixelFormat value: " << static_cast<int>(format);
}

}  // namespace ink
