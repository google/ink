/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_PUBLIC_TYPES_CLIENT_BITMAP_H_
#define INK_ENGINE_PUBLIC_TYPES_CLIENT_BITMAP_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ink/engine/util/dbg/str.h"

namespace ink {

struct ImageSize {
  int width;
  int height;

  ImageSize() : width(0), height(0) {}
  ImageSize(int width, int height) : width(width), height(height) {}
  ImageSize(const ImageSize& o) : width(o.width), height(o.height) {}
  std::string ToString() const;
};

// The byte format of an image.
// To convert to gl enum values, see ink::glTextureFormat(ImageFormat)
enum class ImageFormat {
  BITMAP_FORMAT_NONE = 0,

  // START android/bitmap.h values
  // (Numerical values of enums align with android/bitmap.h)
  BITMAP_FORMAT_RGBA_8888 = 1,
  BITMAP_FORMAT_RGB_565 = 4,
  BITMAP_FORMAT_RGBA_4444 = 7,
  BITMAP_FORMAT_A_8 = 8,
  // END android/bitmap.h values

  BITMAP_FORMAT_RGB_888 = 9,
  BITMAP_FORMAT_BGRA_8888 = 10,
  BITMAP_FORMAT_LA_88 = 11,
};

size_t bytesPerTexelForFormat(const ImageFormat& format);
bool expandTexelToRGBA8888(const ImageFormat& format,
                           const unsigned char* buffer,
                           const unsigned char* bufferEnd, uint32_t* result);

template <>
inline std::string Str(const ImageFormat& format) {
  switch (format) {
    case ImageFormat::BITMAP_FORMAT_NONE:
      return "BITMAP_FORMAT_NONE";
    case ImageFormat::BITMAP_FORMAT_RGBA_8888:
      return "BITMAP_FORMAT_RGBA_8888";
    case ImageFormat::BITMAP_FORMAT_BGRA_8888:
      return "BITMAP_FORMAT_BGRA_8888";
    case ImageFormat::BITMAP_FORMAT_RGB_565:
      return "BITMAP_FORMAT_RGB_565";
    case ImageFormat::BITMAP_FORMAT_RGBA_4444:
      return "BITMAP_FORMAT_RGBA_4444";
    case ImageFormat::BITMAP_FORMAT_A_8:
      return "BITMAP_FORMAT_A_8";
    case ImageFormat::BITMAP_FORMAT_RGB_888:
      return "BITMAP_FORMAT_RGB_888";
    case ImageFormat::BITMAP_FORMAT_LA_88:
      return "BITMAP_FORMAT_LA_88";
  }
  return "(unknown image format!)";
}

///////////////////////////////////////////////////////////////////////////////

// RAII wrapper around client provided bitmap data. Subclasses are responsible
// for returning a consistent non-null pointer from imageByteData() for the
// lifetime of this object
class ClientBitmap {
 public:
  ClientBitmap(const ImageSize& size_px, ImageFormat format)
      : size_px_(size_px), format_(format) {}
  ClientBitmap() {}
  virtual ~ClientBitmap() {}

  virtual const void* imageByteData() const = 0;
  virtual void* imageByteData() = 0;
  const ImageSize sizeInPx() const;
  ImageFormat format() const;
  size_t bytesPerTexel() const;

  // Converts from this bitmap's image format to RGBA 8888.  Note that this
  // makes a copy of the data even if the bitmap is already formatted as RGBA
  // 8888.
  std::vector<uint8_t> Rgba8888ByteData() const;

  std::string toString() const;
  std::string ToString() const { return toString(); }

 protected:
  ImageSize size_px_;
  ImageFormat format_ = ImageFormat::BITMAP_FORMAT_NONE;

 private:
  // ClientBitmap is neither copyable nor movable.
  ClientBitmap(const ClientBitmap&) = delete;
  ClientBitmap& operator=(const ClientBitmap&) = delete;
};

class RawClientBitmap : public ClientBitmap {
 public:
  RawClientBitmap(const ImageSize& size, ImageFormat format)
      : RawClientBitmap(std::vector<uint8_t>(size.width * size.height *
                                             bytesPerTexelForFormat(format)),
                        size, format) {}
  RawClientBitmap(std::vector<uint8_t> data, const ImageSize& size,
                  ImageFormat format)
      : ClientBitmap(size, format), data_(std::move(data)) {}
  const void* imageByteData() const override { return data_.data(); }
  void* imageByteData() override { return data_.data(); }

 private:
  std::vector<uint8_t> data_;
};

// ClientBitmapWrapper does not take ownership of, or copy, the given bitmap
// data.
class ClientBitmapWrapper : public ClientBitmap {
 public:
  ClientBitmapWrapper(void* data, const ImageSize& size, ImageFormat format)
      : ClientBitmap(size, format), data_(data) {}
  const void* imageByteData() const override { return data_; }
  void* imageByteData() override { return data_; }

 private:
  void* data_;
};

namespace client_bitmap {
// These functions do byte order manipulations, to help with conversions from
// systems that create bitmaps not compatible with OpenGL texture formats.

// The given bitmap must have ImageFormat::BITMAP_FORMAT_RGBA_8888
void ConvertBGRAToRGBA(ClientBitmap* bitmap);

// The given bitmap must have ImageFormat::BITMAP_FORMAT_RGB_888
void ConvertBGRToRGB(ClientBitmap* bitmap);
}  // namespace client_bitmap

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_CLIENT_BITMAP_H_
