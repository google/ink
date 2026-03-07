#ifndef INK_BRUSH_VERSION_H_
#define INK_BRUSH_VERSION_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

namespace ink {
namespace version {
inline constexpr int32_t kMin = 0;
// LINT.IfChange(kMax)
// kMax is the maximum version which can be deserialized currently.
// Increment kMax when committing a new version of the brush customization
// format. Features still under development should be associated with
// kDevelopment until they are ready to be committed, after which they cannot
// be changed.
inline constexpr int32_t kMax = 2;
// LINT.ThenChange(:jetpack_versions)
// Using int32_t to represent versions so that we can use the max value to
// represent kDevelopment, and still store it in a proto int32 field.
inline constexpr int32_t kDevelopment = std::numeric_limits<int32_t>::max();

}  // namespace version

inline std::string ToFormattedString(int32_t version) {
  // LINT.IfChange(jetpack_versions)
  // This vector represents the Jetpack version which introduced each version
  // of the brush customization format. The index represents the version of the
  // brush customization format, and the value is the Jetpack version in which
  // that version was introduced.
  //
  // Multiple versions of the brush customization format may be associated with
  // the same Jetpack release. New versions should be mapped to the next Jetpack
  // release.
  //
  // This vector should never be modified except to add a new entry to the end,
  // which must be done every time kMax is incremented.
  static const absl::NoDestructor<std::vector<std::string>> jetpack_versions(
      {"1.0.0", "1.1.0-alpha01", "1.1.0-alpha02"});
  // LINT.ThenChange(:kMax)
  if (static_cast<size_t>(version) >= jetpack_versions->size() || version < 0) {
    return absl::StrCat(version, " (Unrecognized version)");
  }
  return absl::StrCat(version, " (included in Jetpack version: ",
                      (*jetpack_versions)[version], ")");
}

inline absl::Status ValidateThatVersionIsSupported(
    int32_t version, bool allow_development_version = false) {
  if (allow_development_version && version == version::kDevelopment) {
    return absl::OkStatus();
  }
  if (version < version::kMin) {
    return absl::InvalidArgumentError(
        absl::StrCat("Version must be greater than or equal to ",
                     ToFormattedString(version::kMin), ", but was ",
                     ToFormattedString(version)));
  }
  if (version > version::kMax) {
    return absl::InvalidArgumentError(
        absl::StrCat("Version must be less than or equal to ",
                     ToFormattedString(version::kMax), ", but was ",
                     ToFormattedString(version)));
  }
  return absl::OkStatus();
}

}  // namespace ink

#endif  // INK_BRUSH_VERSION_H_
