#ifndef INK_BRUSH_VERSION_H_
#define INK_BRUSH_VERSION_H_

#include <compare>
#include <string>

#include "absl/strings/str_cat.h"

namespace ink {

struct Version {
  int major;
  int minor;
  int bug;

  enum class Cycle { kAlpha = 0, kBeta = 1, kRC = 2, kStable = 3 };
  Cycle cycle;
  int release;

  std::strong_ordering operator<=>(const Version& other) const = default;
};

inline Version MaxVersion(Version lhs, Version rhs) {
  return lhs > rhs ? lhs : rhs;
}

inline std::string ToFormattedString(Version::Cycle cycle) {
  switch (cycle) {
    case Version::Cycle::kAlpha:
      return "alpha";
    case Version::Cycle::kBeta:
      return "beta";
    case Version::Cycle::kRC:
      return "rc";
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
constexpr Version k1_1_0_alpha_01 = {1, 1, 0, Version::Cycle::kAlpha, 1};
constexpr Version kMax = k1_1_0_alpha_01;

}  // namespace version
}  // namespace ink

#endif  // INK_BRUSH_VERSION_H_
