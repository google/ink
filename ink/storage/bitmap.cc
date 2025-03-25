#include "ink/storage/bitmap.h"

#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/rendering/bitmap.h"
#include "ink/storage/color.h"
#include "ink/storage/proto/bitmap.pb.h"
#include "ink/storage/proto/color.pb.h"

namespace ink {

proto::Bitmap::PixelFormat EncodePixelFormat(
    const Bitmap::PixelFormat& pixel_format) {
  switch (pixel_format) {
    case Bitmap::PixelFormat::kRgba8888:
      return proto::Bitmap::PIXEL_FORMAT_RGBA8888;
  }
  ABSL_LOG(FATAL) << "Unknown PixelFormat " << pixel_format;
  return {};
}

absl::StatusOr<Bitmap::PixelFormat> DecodePixelFormat(
    const proto::Bitmap::PixelFormat& pixel_format_proto) {
  switch (pixel_format_proto) {
    case proto::Bitmap::PIXEL_FORMAT_RGBA8888:
      return Bitmap::PixelFormat::kRgba8888;
    default:
      return absl::InvalidArgumentError(absl::StrCat(
          "Invalid ink.proto.Bitmap.PixelFormat value: ", pixel_format_proto));
  }
}

void EncodeBitmap(const Bitmap& bitmap, proto::Bitmap& bitmap_proto) {
  bitmap_proto.set_width(bitmap.width());
  bitmap_proto.set_height(bitmap.height());
  bitmap_proto.set_color_space(EncodeColorSpace(bitmap.color_space()));
  bitmap_proto.set_pixel_format(EncodePixelFormat(bitmap.pixel_format()));
  absl::Span<const uint8_t> pixel_data = bitmap.GetPixelData();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());
}

absl::StatusOr<VectorBitmap> DecodeBitmap(const proto::Bitmap& bitmap_proto) {
  if (!bitmap_proto.has_width()) {
    return absl::InvalidArgumentError("Bitmap proto: missing required width");
  }
  if (!bitmap_proto.has_height()) {
    return absl::InvalidArgumentError("Bitmap proto: missing required height");
  }
  if (bitmap_proto.width() <= 0) {
    return absl::InvalidArgumentError("Bitmap proto: width must be positive");
  }
  if (bitmap_proto.height() <= 0) {
    return absl::InvalidArgumentError("Bitmap proto: height must be positive");
  }

  absl::StatusOr<Bitmap::PixelFormat> pixel_format =
      DecodePixelFormat(bitmap_proto.pixel_format());
  if (!pixel_format.ok()) {
    return pixel_format.status();
  }
  ColorSpace color_space = DecodeColorSpace(bitmap_proto.color_space());

  // Width and height are int32_t, so this multiplication can't overflow.
  int64_t area = static_cast<int64_t>(bitmap_proto.width()) *
                 static_cast<int64_t>(bitmap_proto.height());
  if (area > std::numeric_limits<int32_t>::max()) {
    return absl::InvalidArgumentError("Bitmap proto: area overflows int32_t");
  }
  int64_t expected_pixel_data_size =
      area * static_cast<int64_t>(PixelFormatBytesPerPixel(*pixel_format));
  if (expected_pixel_data_size > std::numeric_limits<int32_t>::max()) {
    return absl::InvalidArgumentError(
        "Bitmap proto: pixel data size overflows int32_t");
  }
  if (bitmap_proto.pixel_data().size() !=
      static_cast<size_t>(expected_pixel_data_size)) {
    return absl::InvalidArgumentError(
        "Bitmap proto: pixel data has incorrect size");
  }

  std::vector<uint8_t> pixel_data(bitmap_proto.pixel_data().begin(),
                                  bitmap_proto.pixel_data().end());

  return VectorBitmap(bitmap_proto.width(), bitmap_proto.height(),
                      *pixel_format, Color::Format::kGammaEncoded, color_space,
                      std::move(pixel_data));
}

}  // namespace ink
