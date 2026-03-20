#ifndef INK_BRUSH_VERSION_H_
#define INK_BRUSH_VERSION_H_

#include <compare>
#include <cstdint>
#include <limits>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"

namespace ink {

class Version {
 public:
  std::strong_ordering operator<=>(const Version& other) const = default;

  int32_t value() const { return value_; }

  std::string ToFormattedString() const { return absl::StrCat(value_); }

  // Brush version is incremented when new features are ready to be released.
  // This is a versioning process independent of the Jetpack version. A Jetpack
  // version may have multiple brush version increments, or none at all
  // (containing only previously released brush versions).

  // Included with Jetpack 1.1.0-alpha01.
  static constexpr Version k1Jetpack1_1_0Alpha01() { return Version(1); }
  // Included with Jetpack 1.0.0.
  static constexpr Version k0Jetpack1_0_0() { return Version(0); }

  // Features associated with kDevelopment will always be rejected by the
  // deserializer due to kDevelopment > kMaxSupported. The only exception is
  // when `max_version`, an optional parameter to DecodeBrush and
  // DecodeBrushFamily is set to
  // kDevelopment.
  static constexpr Version kDevelopment() {
    return Version(std::numeric_limits<int32_t>::max());
  }
  // kMaxSupported is the maximum version which can be deserialized by default.
  // Increment kMaxSupported when releasing a new version of the brush
  // customization format. Features still under development should be associated
  // with kDevelopment until they are ready to be released, after which they
  // are expected to be supported long-term.
  static constexpr Version kMaxSupported() { return k1Jetpack1_1_0Alpha01(); }

 private:
  explicit constexpr Version(int32_t value) : value_(value) {
    // Negative values are invalid.
    ABSL_CHECK_GE(value, 0);
  }
  // Using int32_t to represent versions so that we can use the max value to
  // represent kDevelopment, and still store it in a proto int32 field.
  int32_t value_;
};
}  // namespace ink

#endif  // INK_BRUSH_VERSION_H_
