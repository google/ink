#include "ink/storage/bitmap.h"

#include <sys/types.h>

#include <cstdint>
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

absl::Status EncodeBitmap(const Bitmap& bitmap, proto::Bitmap& bitmap_proto) {
  if (absl::Status status = rendering_internal::ValidateBitmap(bitmap);
      !status.ok()) {
    return status;
  }
  bitmap_proto.set_width(bitmap.width());
  bitmap_proto.set_height(bitmap.height());
  bitmap_proto.set_color_space(EncodeColorSpace(bitmap.color_space()));
  bitmap_proto.set_pixel_format(EncodePixelFormat(bitmap.pixel_format()));
  absl::Span<const uint8_t> pixel_data = bitmap.GetPixelData();
  bitmap_proto.set_pixel_data(pixel_data.data(), pixel_data.size());
  return absl::OkStatus();
}

absl::StatusOr<VectorBitmap> DecodeBitmap(const proto::Bitmap& bitmap_proto) {
  if (!bitmap_proto.has_width()) {
    return absl::InvalidArgumentError("Bitmap proto: missing required width");
  }
  if (!bitmap_proto.has_height()) {
    return absl::InvalidArgumentError("Bitmap proto: missing required height");
  }

  absl::StatusOr<Bitmap::PixelFormat> pixel_format =
      DecodePixelFormat(bitmap_proto.pixel_format());
  if (!pixel_format.ok()) {
    return pixel_format.status();
  }
  ColorSpace color_space = DecodeColorSpace(bitmap_proto.color_space());

  if (absl::Status status = rendering_internal::ValidateBitmap(
          bitmap_proto.width(), bitmap_proto.height(), *pixel_format,
          bitmap_proto.pixel_data().size());
      !status.ok()) {
    return status;
  }

  std::vector<uint8_t> pixel_data(bitmap_proto.pixel_data().begin(),
                                  bitmap_proto.pixel_data().end());

  return VectorBitmap(bitmap_proto.width(), bitmap_proto.height(),
                      *pixel_format, Color::Format::kGammaEncoded, color_space,
                      std::move(pixel_data));
}

}  // namespace ink
