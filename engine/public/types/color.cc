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

#include "ink/engine/public/types/color.h"

#include <ostream>

#include "third_party/absl/base/macros.h"  // arraysize
#include "third_party/absl/strings/str_cat.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

namespace {
// A bunch of unique colors to use for debug rendering.
// RGBA non-premultiplied.
const uint32_t kColors[] = {
    0x000000FF, 0x929292FF, 0xFF5151FF, 0xA52714FF, 0xFFBC02FF,
    0xEF8003FF, 0x02C853FF, 0x548B2FFF, 0x01B0FFFF, 0x00579BFF,
    0xD50DFAFF, 0x8D24AAFF, 0x8D6E63FF, 0x4E342EFF,
};
const size_t kColorsSize = arraysize(kColors);
}  // namespace

glm::vec4 Color::AsNonPremultipliedVec() const {
  return UintToVec4RGBA(rgba_non_premultiplied_);
}

uint32_t Color::AsNonPremultipliedUintRGBA() const {
  return rgba_non_premultiplied_;
}

uint32_t Color::AsNonPremultipliedUintABGR() const {
  const auto& c = rgba_non_premultiplied_;
  return ((c & 0xFF) << 24) | (((c >> 8) & 0xFF) << 16) |
         (((c >> 16) & 0xFF) << 8) | ((c >> 24) & 0xFF);
}

glm::vec4 Color::AsPremultipliedVec() const {
  return RGBtoRGBPremultiplied(UintToVec4RGBA(rgba_non_premultiplied_));
}

/* static */
Color Color::Lerp(Color a, Color b, float amount) {
  auto a_hsv = RGBtoHSV(a.AsNonPremultipliedVec());
  auto b_hsv = RGBtoHSV(b.AsNonPremultipliedVec());
  auto lerped = util::Lerp(a_hsv, b_hsv, amount);
  return FromNonPremultipliedRGBA(HSVtoRGB(lerped));
}

Color Color::WithAlpha(float new_alpha) const {
  auto upm = AsNonPremultipliedVec();
  upm.a = new_alpha;
  return Color::FromNonPremultipliedRGBA(upm);
}

Color::Color() : Color(0x000000FF) {}
Color::Color(uint32_t rgba_non_premultiplied)
    : rgba_non_premultiplied_(rgba_non_premultiplied) {}

/* static */
Color Color::FromNonPremultipliedRGBA(glm::vec4 rgba_non_premultiplied) {
  return Color(Vec4ToUintRGBA(rgba_non_premultiplied));
}
/* static */
Color Color::FromNonPremultipliedRGBA(uint32_t rgba_non_premultiplied) {
  return Color(rgba_non_premultiplied);
}
/*static*/
Color Color::FromNonPremultiplied(uint8_t red, uint8_t green, uint8_t blue,
                                  uint8_t alpha) {
  return Color(((red & 0xFF) << 24) | ((green & 0xFF) << 16) |
               ((blue & 0xFF) << 8) | (alpha & 0xFF));
}

/* static */
Color Color::FromPremultipliedRGBA(glm::vec4 rgba_premultiplied) {
  return FromNonPremultipliedRGBA(RGBPremultipliedtoRGB(rgba_premultiplied));
}
/* static */
Color Color::FromPremultipliedRGBA(uint32_t rgba_premultiplied) {
  return FromPremultipliedRGBA(UintToVec4RGBA(rgba_premultiplied));
}
/*static*/
Color Color::FromPremultiplied(uint8_t red, uint8_t green, uint8_t blue,
                               uint8_t alpha) {
  return FromPremultipliedRGBA(((red & 0xFF) << 24) | ((green & 0xFF) << 16) |
                               ((blue & 0xFF) << 8) | (alpha & 0xFF));
}

/* static */
Color Color::SeededColor(uint32_t seed) {
  return FromNonPremultipliedRGBA(kColors[seed % kColorsSize]);
}

/* static constants */
const Color Color::kBlack = Color::FromNonPremultipliedRGBA(0x000000ff);
const Color Color::kGray = Color::FromNonPremultipliedRGBA(0x2a2a2afF);
const Color Color::kGrey = Color::kGray;
const Color Color::kWhite = Color::FromNonPremultipliedRGBA(0xffffffff);
const Color Color::kRed = Color::FromNonPremultipliedRGBA(0xff0000ff);
const Color Color::kBlue = Color::FromNonPremultipliedRGBA(0x0000ffff);
const Color Color::kGreen = Color::FromNonPremultipliedRGBA(0x00ff00ff);
const Color Color::kTransparent = Color::FromNonPremultipliedRGBA(0x0);

std::string Color::ToString() const {
  return absl::StrCat("Color:\n  RGBA-nonpre(",
                      absl::Hex(AsNonPremultipliedUintRGBA(), absl::kZeroPad8),
                      ")\n  ABGR-nonpre(",
                      absl::Hex(AsNonPremultipliedUintABGR(), absl::kZeroPad8),
                      ")\n  vec-nonpre(", Str(AsNonPremultipliedVec()),
                      ")\n  vec-pre(", Str(AsPremultipliedVec()), ")");
}

std::ostream& operator<<(std::ostream& oss, const Color& color) {
  oss << color.ToString();
  return oss;
}

bool Color::operator==(const Color& other) const {
  return rgba_non_premultiplied_ == other.rgba_non_premultiplied_;
}

}  // namespace ink
