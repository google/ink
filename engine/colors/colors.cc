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

#include "ink/engine/colors/colors.h"

#include <algorithm>  // for max, min
#include <cmath>      // for isnan

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"  // for EXPECT
namespace ink {

using util::Lerp;
using util::Normalize;

glm::vec4 RGBtoHSV(glm::vec4 rgb) {
  glm::vec4 hsv = glm::vec4(0, 0, 0, rgb.a);

  float r = std::isnan(rgb.r) ? 0 : rgb.r;
  float g = std::isnan(rgb.g) ? 0 : rgb.g;
  float b = std::isnan(rgb.b) ? 0 : rgb.b;

  float M = std::max(std::max(r, g), b);
  float m = std::min(std::min(r, g), b);
  float c = M - m;

  // calc hue
  float hp = 0;
  if (c == 0) {
    hp = 0;
  } else if (M == r) {
    hp = glm::mod(((g - b) / c), 6.0f);
  } else if (M == g) {
    hp = ((b - r) / c) + 2;
  } else if (M == b) {
    hp = ((r - g) / c) + 4;
  } else {
    EXPECT(false);
  }
  float h = hp * 60;

  // value
  float v = M;

  // saturation
  float s = 0;
  if (c != 0) s = c / v;

  hsv.x = h / 360.0f;
  hsv.y = s;
  hsv.z = v;
  return hsv;
}

glm::vec4 HSVtoRGB(glm::vec4 hsv) {
  float h = glm::mod(hsv.x, 1.0f);
  float hp = (h * 360.0f) / 60.0f;
  float c = hsv.y * hsv.z;
  float x = c * (1.0f - glm::abs(glm::mod(hp, 2.0f) - 1.0f));
  float m = hsv.z - c;
  glm::vec4 rgb{0, 0, 0, hsv.w};

  if (hp < 1.0f) {
    rgb.r = c;
    rgb.g = x;
  } else if (hp < 2.0f) {
    rgb.r = x;
    rgb.g = c;
  } else if (hp < 3.0f) {
    rgb.g = c;
    rgb.b = x;
  } else if (hp < 4.0f) {
    rgb.g = x;
    rgb.b = c;
  } else if (hp < 5.0f) {
    rgb.r = x;
    rgb.b = c;
  } else {
    rgb.r = c;
    rgb.b = x;
  }

  rgb.r += m;
  rgb.g += m;
  rgb.b += m;
  return rgb;
}

glm::vec4 RGBtoRGBPremultiplied(glm::vec4 rgb) {
  auto res = rgb;
  res.r *= rgb.a;
  res.g *= rgb.a;
  res.b *= rgb.a;
  return res;
}

glm::vec4 RGBPremultipliedtoRGB(glm::vec4 rgb) {
  if (rgb.a == 0.0f) return glm::vec4(0.0f);
  auto res = rgb;
  res.r /= rgb.a;
  res.g /= rgb.a;
  res.b /= rgb.a;
  return res;
}

glm::vec4 UintToVec4ABGR(uint32_t c) {
  return glm::vec4{
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 0 & 0xff)),    // R
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 8 & 0xff)),    // G
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 16 & 0xff)),   // B
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 24 & 0xff))};  // A
}

uint32_t Vec4ToUintABGR(glm::vec4 c) {
  uint32_t res = 0;
  res |= SRgbFloatToByte(c.a) << 24;
  res |= SRgbFloatToByte(c.b) << 16;
  res |= SRgbFloatToByte(c.g) << 8;
  res |= SRgbFloatToByte(c.r);

  return res;
}

glm::vec4 UintToVec4ARGB(uint32_t c) {
  return glm::vec4{
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 16 & 0xff)),   // R
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 8 & 0xff)),    // G
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 0 & 0xff)),    // B
      Normalize(0.0f, 255.0f, static_cast<float>(c >> 24 & 0xff))};  // A
}

uint32_t Vec4ToUintARGB(glm::vec4 c) {
  uint32_t res = 0;
  res |= SRgbFloatToByte(c.a) << 24;
  res |= SRgbFloatToByte(c.r) << 16;
  res |= SRgbFloatToByte(c.g) << 8;
  res |= SRgbFloatToByte(c.b);

  return res;
}

glm::vec4 UintToVec4RGBA(uint32_t rgba) {
  return glm::vec4{
      Normalize(0.0f, 255.0f, static_cast<float>(rgba >> 24 & 0xff)),  // R
      Normalize(0.0f, 255.0f, static_cast<float>(rgba >> 16 & 0xff)),  // G
      Normalize(0.0f, 255.0f, static_cast<float>(rgba >> 8 & 0xff)),   // B
      Normalize(0.0f, 255.0f, static_cast<float>(rgba >> 0 & 0xff))};  // A
}

uint32_t Vec4ToUintRGBA(glm::vec4 c) {
  uint32_t res = 0;
  res |= SRgbFloatToByte(c.r) << 24;
  res |= SRgbFloatToByte(c.g) << 16;
  res |= SRgbFloatToByte(c.b) << 8;
  res |= SRgbFloatToByte(c.a);

  return res;
}
}  // namespace ink
