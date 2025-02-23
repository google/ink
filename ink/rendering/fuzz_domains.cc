#include "ink/rendering/fuzz_domains.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "fuzztest/fuzztest.h"
#include "absl/log/absl_check.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/color/fuzz_domains.h"
#include "ink/rendering/bitmap.h"

namespace ink {

using ::fuzztest::Arbitrary;
using ::fuzztest::Domain;
using ::fuzztest::ElementOf;
using ::fuzztest::FlatMap;
using ::fuzztest::InRange;
using ::fuzztest::Map;
using ::fuzztest::VectorOf;

// LINT.IfChange(pixel_format)
Domain<Bitmap::PixelFormat> ArbitraryBitmapPixelFormat() {
  return ElementOf({
      Bitmap::PixelFormat::kRgba8888,
  });
}
// LINT.ThenChange(bitmap.h:pixel_format)

Domain<std::unique_ptr<Bitmap>> ValidBitmapWithMaxSize(int max_width,
                                                       int max_height) {
  ABSL_CHECK_GE(max_width, 1);
  ABSL_CHECK_GE(max_height, 1);
  return FlatMap(
      [](int width, int height, Bitmap::PixelFormat pixel_format) {
        return Map(
            [width, height, pixel_format](Color::Format color_format,
                                          ColorSpace color_space,
                                          std::vector<uint8_t> pixel_data) {
              return std::unique_ptr<Bitmap>(std::make_unique<VectorBitmap>(
                  width, height, pixel_format, color_format, color_space,
                  std::move(pixel_data)));
            },
            ArbitraryColorFormat(), ArbitraryColorSpace(),
            VectorOf(Arbitrary<uint8_t>())
                .WithSize(static_cast<size_t>(width) *
                          static_cast<size_t>(height) *
                          PixelFormatBytesPerPixel(pixel_format)));
      },
      InRange<int>(1, max_width), InRange<int>(1, max_height),
      ArbitraryBitmapPixelFormat());
}

}  // namespace ink
