// Copyright 2018 Google LLC
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

#include "ink/engine/public/types/client_bitmap.h"

#include <string>
#include <type_traits>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

std::string ImageSize::ToString() const {
  return Substitute("ImageSize(width: $0, height $1)", width, height);
}

const ImageSize ClientBitmap::sizeInPx() const { return size_px_; }

ImageFormat ClientBitmap::format() const { return format_; }

size_t ClientBitmap::bytesPerTexel() const {
  return bytesPerTexelForFormat(format_);
}

std::string ClientBitmap::toString() const {
  return Substitute("size: $0, format $1, address $2", size_px_, format_,
                    AddressStr(imageByteData()));
}

///////////////////////////////////////////////////////////////////////////////

size_t bytesPerTexelForFormat(const ImageFormat& format) {
  switch (format) {
    case ImageFormat::BITMAP_FORMAT_RGBA_8888:
    case ImageFormat::BITMAP_FORMAT_BGRA_8888:
      return 4;
    case ImageFormat::BITMAP_FORMAT_RGB_565:
      return 2;
    case ImageFormat::BITMAP_FORMAT_RGBA_4444:
      return 2;
    case ImageFormat::BITMAP_FORMAT_A_8:
      return 1;
    case ImageFormat::BITMAP_FORMAT_RGB_888:
      return 3;

    case ImageFormat::BITMAP_FORMAT_NONE:
    default:
      RUNTIME_ERROR(
          "attempt to calculate bytes per texel on unsupported format $0",
          format);
  }
}

bool expandTexelToRGBA8888(const ImageFormat& format,
                           const unsigned char* buffer,
                           const unsigned char* buffer_end, uint32_t* result) {
  ASSERT(result);
  *result = -1;
  const int kResLen = 4;
  unsigned char res[kResLen];

  const auto nbytes = bytesPerTexelForFormat(format);
  if (buffer_end < buffer              // wrong direction
      || buffer + nbytes > buffer_end  // would read past end of buffer
      || buffer + nbytes < buffer      // addition overflow
      || nbytes > kResLen) {  // we memcpy nbytes to res for some formats
    return false;
  }

  switch (format) {
    case ImageFormat::BITMAP_FORMAT_RGBA_8888:
      memcpy(res, buffer, nbytes);
      break;
    case ImageFormat::BITMAP_FORMAT_BGRA_8888:
      res[0] = buffer[2];
      res[1] = buffer[1];
      res[2] = buffer[0];
      res[3] = buffer[3];
      break;
    case ImageFormat::BITMAP_FORMAT_RGB_565: {
      int r5 = buffer[0] >> 3;
      int g6 = ((buffer[0] & 0b00000111) << 3) | (buffer[1] >> 5);
      int b5 = (buffer[1] & 0b00011111);

      res[0] = (r5 * 255 + 15) / 31;
      res[1] = (g6 * 255 + 31) / 63;
      res[2] = (b5 * 255 + 15) / 31;
      res[3] = 255;
      break;
    }
    case ImageFormat::BITMAP_FORMAT_RGBA_4444: {
      int r4 = buffer[0] >> 4;
      int g4 = buffer[0] & 0b00001111;
      int b4 = buffer[1] >> 4;
      int a4 = buffer[1] & 0b00001111;

      res[0] = (r4 * 255 + 7) / 15;
      res[1] = (g4 * 255 + 7) / 15;
      res[2] = (b4 * 255 + 7) / 15;
      res[3] = (a4 * 255 + 7) / 15;
      break;
    }
    case ImageFormat::BITMAP_FORMAT_A_8:
      res[0] = 0;
      res[1] = 0;
      res[2] = 0;
      res[3] = buffer[0];
      break;
    case ImageFormat::BITMAP_FORMAT_RGB_888:
      memcpy(res, buffer, nbytes);
      res[3] = 1;
      break;

    case ImageFormat::BITMAP_FORMAT_NONE:
    default:
      SLOG(SLOG_ERROR, "attempt to expand to RGBA8888 on unsupported format $0",
           format);
      return false;
  }

  *result = res[0] << 24 | res[1] << 16 | res[2] << 8 | res[3];
  return true;
}

namespace client_bitmap {

namespace {
void ConvertBGRToRGB(ClientBitmap* bitmap, int stride) {
  uint8_t* data = static_cast<uint8_t*>(bitmap->imageByteData());
  const int num_pixels = bitmap->sizeInPx().width * bitmap->sizeInPx().height;
  for (int i = 0; i < num_pixels; i++) {
    uint8_t* color_start = &data[i * stride];
    std::swap(*color_start, *(color_start + 2));
  }
}
}  // namespace

void ConvertBGRAToRGBA(ClientBitmap* bitmap) {
  assert(bitmap->format() == ImageFormat::BITMAP_FORMAT_RGBA_8888);
  ConvertBGRToRGB(bitmap, 4);
}

void ConvertBGRToRGB(ClientBitmap* bitmap) {
  assert(bitmap->format() == ImageFormat::BITMAP_FORMAT_RGB_888);
  ConvertBGRToRGB(bitmap, 3);
}
}  // namespace client_bitmap

}  // namespace ink
