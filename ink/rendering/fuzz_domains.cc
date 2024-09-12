#include "ink/rendering/fuzz_domains.h"

#include <cstddef>
#include <cstdint>

#include "fuzztest/fuzztest.h"
#include "absl/log/check.h"
#include "ink/color/fuzz_domains.h"
#include "ink/rendering/bitmap.h"

namespace ink {

using ::fuzztest::Arbitrary;
using ::fuzztest::Domain;
using ::fuzztest::ElementOf;
using ::fuzztest::FlatMap;
using ::fuzztest::InRange;
using ::fuzztest::Just;
using ::fuzztest::StructOf;
using ::fuzztest::VectorOf;

// LINT.IfChange(pixel_format)
Domain<Bitmap::PixelFormat> ArbitraryBitmapPixelFormat() {
  return ElementOf({
      Bitmap::PixelFormat::kRgba8888,
  });
}
// LINT.ThenChange(bitmap.h:pixel_format)

Domain<Bitmap> ValidBitmapWithMaxSize(int max_width, int max_height) {
  CHECK_GE(max_width, 1);
  CHECK_GE(max_height, 1);
  return FlatMap(
      [](int width, int height, Bitmap::PixelFormat pixel_format) {
        return StructOf<Bitmap>(
            VectorOf(Arbitrary<uint8_t>())
                .WithSize(static_cast<size_t>(width) *
                          static_cast<size_t>(height) *
                          PixelFormatBytesPerPixel(pixel_format)),
            Just(width), Just(height), Just(pixel_format),
            ArbitraryColorFormat(), ArbitraryColorSpace());
      },
      InRange<int>(1, max_width), InRange<int>(1, max_height),
      ArbitraryBitmapPixelFormat());
}

}  // namespace ink
