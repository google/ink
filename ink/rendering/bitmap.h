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

#ifndef INK_RENDERING_BITMAP_H_
#define INK_RENDERING_BITMAP_H_

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"

namespace ink {

// Stores raw bitmap data in a platform- and renderer-independent form.
//
// To be valid:
//   - `width` and `height` must be strictly positive.
//   - The `pixel_format`, `color_format`, and `color_space` enumerator values
//     must be valid.
//   - `data` must contain the correct number of bytes for the image size and
//     pixel format.
struct Bitmap {
  // LINT.IfChange(pixel_format)
  enum class PixelFormat {
    kRgba8888,
  };
  // LINT.ThenChange(fuzz_domains.h:pixel_format)

  std::vector<uint8_t> data;
  int width;
  int height;
  PixelFormat pixel_format;
  Color::Format color_format;
  ColorSpace color_space;
};

size_t PixelFormatBytesPerPixel(Bitmap::PixelFormat format);

namespace rendering_internal {

absl::Status ValidateBitmap(const Bitmap& bitmap);

std::string ToFormattedString(Bitmap::PixelFormat format);

}  // namespace rendering_internal

template <typename Sink>
void AbslStringify(Sink& sink, Bitmap::PixelFormat format) {
  sink.Append(rendering_internal::ToFormattedString(format));
}

}  // namespace ink

#endif  // INK_RENDERING_BITMAP_H_
