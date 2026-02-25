#ifndef INK_BRUSH_VERSION_H_
#define INK_BRUSH_VERSION_H_

#include <compare>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

namespace ink {

struct Version {
  int major = 1;
  int minor = 0;
  int bug = 0;

  enum class Cycle {
    kAlpha = 0,
    kBeta = 1,
    kReleaseCandidate = 2,
    kStable = 3
  };
  Cycle cycle = Cycle::kStable;
  int release = 1;

  std::strong_ordering operator<=>(const Version& other) const = default;
};

inline std::string ToFormattedString(Version::Cycle cycle) {
  switch (cycle) {
    case Version::Cycle::kAlpha:
      return "-alpha";
    case Version::Cycle::kBeta:
      return "-beta";
    case Version::Cycle::kReleaseCandidate:
      return "-rc";
    case Version::Cycle::kStable:
      return "";
  }
}

inline std::string ToFormattedString(Version version) {
  return absl::StrCat(
      version.major, ".", version.minor, ".", version.bug,
      ToFormattedString(version.cycle),
      (version.cycle == Version::Cycle::kStable
           ? ""
           : absl::StrCat(version.release < 10 ? "0" : "", version.release)));
}

namespace version {

constexpr Version k1_0_0 = {1, 0, 0, Version::Cycle::kStable, 1};
constexpr Version kMin = k1_0_0;
constexpr Version k1_1_0_alpha_01 = {1, 1, 0, Version::Cycle::kAlpha, 1};
constexpr Version kMax = k1_1_0_alpha_01;

}  // namespace version

inline absl::Status ValidateVersion(Version version) {
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
  if (version.release < 1) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Version::release must be greater than 0, but was ", version.release));
  }
  if (version.cycle == Version::Cycle::kStable && version.release != 1) {
    return absl::InvalidArgumentError(
        absl::StrCat("Version::release must be 1 for stable cycle, but was ",
                     version.release));
  }
  return absl::OkStatus();
}

}  // namespace ink

#endif  // INK_BRUSH_VERSION_H_
