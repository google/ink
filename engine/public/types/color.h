/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_PUBLIC_TYPES_COLOR_H_
#define INK_ENGINE_PUBLIC_TYPES_COLOR_H_

#include <iosfwd>
#include <string>

#include "third_party/glm/glm/glm.hpp"

namespace ink {

// A color.
class Color {
 public:
  glm::vec4 AsPremultipliedVec() const;
  glm::vec4 AsNonPremultipliedVec() const;
  uint32_t AsNonPremultipliedUintRGBA() const;
  uint32_t AsNonPremultipliedUintABGR() const;

  // Linear interpolation in HSV-space between a,b.
  static Color Lerp(Color a, Color b, float amount);

  // Returns a new color that is *this with new_alpha.
  Color WithAlpha(float new_alpha) const;

  // Create a color from various formats.
  Color();  // Defaults to black.
  bool operator==(const Color& other) const;

  static Color FromNonPremultipliedRGBA(glm::vec4 rgba_non_premultiplied);
  static Color FromNonPremultipliedRGBA(uint32_t rgba_non_premultiplied);
  static Color FromNonPremultiplied(uint8_t red, uint8_t green, uint8_t blue,
                                    uint8_t alpha);
  static Color FromPremultipliedRGBA(glm::vec4 rgba_premultiplied);
  static Color FromPremultipliedRGBA(uint32_t rgba_premultiplied);
  static Color FromPremultiplied(uint8_t red, uint8_t green, uint8_t blue,
                                 uint8_t alpha);

  // Predefined colors.
  static const Color kBlack;
  static const Color kGray;
  static const Color kGrey;
  static const Color kWhite;
  static const Color kRed;
  static const Color kBlue;
  static const Color kGreen;
  static const Color kTransparent;

  // Given a number, choose an arbitrary color stable to that number.
  static Color SeededColor(uint32_t seed);

  inline uint8_t RedByteNonPremultiplied() const {
    return static_cast<uint8_t>((rgba_non_premultiplied_ >> 24) & 0xFF);
  }
  inline uint8_t GreenByteNonPremultiplied() const {
    return static_cast<uint8_t>((rgba_non_premultiplied_ >> 16) & 0xFF);
  }
  inline uint8_t BlueByteNonPremultiplied() const {
    return static_cast<uint8_t>((rgba_non_premultiplied_ >> 8) & 0xFF);
  }
  inline uint8_t AlphaByte() const {
    return static_cast<uint8_t>(rgba_non_premultiplied_ & 0xFF);
  }

  std::string ToString() const;
  friend std::ostream& operator<<(std::ostream& oss, const Color& color);

 private:
  explicit Color(uint32_t rgba_non_premultiplied);
  uint32_t rgba_non_premultiplied_;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_COLOR_H_
