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

#include "ink/engine/util/funcs/float_pack.h"

#include <cmath>
#include <cstdint>

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

// #defines for powers of 2 integers and floats
#define I6 64
#define I7 128
#define I11 2048
#define I12 4096
#define I18 262144

#define F6 64.0f
#define F7 128.0f
#define F11 2048.0f
#define F12 4096.0f
#define F18 262144.0f

#define D24 16777216.0

float Fract(double value) { return static_cast<float>(std::fmod(value, 1.0)); }

/////////////////////////////////

float PackPosition(glm::vec2 position) {
  // format is 24 bits in the mantissa -- x12y12

  // range of position is [0u-4096u)
  auto x = static_cast<uint32_t>(util::Clamp0N(F12 - 1, roundf(position.x)));
  auto y = static_cast<uint32_t>(util::Clamp0N(F12 - 1, roundf(position.y)));

  uint32_t upacked = (x << 12) | (y << 0);
  float fpacked = static_cast<float>(static_cast<double>(upacked) / D24);

  return fpacked;
}

glm::vec2 UnpackPosition(float packed_position) {
  // format is 24 bit -- x12y12
  // positions are packed into the range [0f-1f), and unpack to the range
  // [0u-4096u)

  glm::vec2 position{0, 0};

  position.x = Fract(packed_position);
  position.x *= F12;
  position.x = floorf(position.x);

  position.y = Fract(packed_position * F12);
  position.y *= F12;

  return position;
}

glm::vec2 PackColorAndPosition(glm::vec4 color, glm::vec2 position) {
  // Format is 48 bit -- x11a7r6, y11g7b6 -- using 24 bits in the mantissas of
  // each of the components of the return.
  //
  // Positions are given in [0u-2048u), and are packed into [0f-1f).
  //
  // Colors rgba are given in [0-1f].
  // Color rb are compressed to [0u-64u) then packed into [0f-1f).  Note the
  // half-open intervals.
  // Color ag are compressed to [0u-128u) then packed into [0f-1f).

  // x and y are given in [0-2048)
  auto x = static_cast<uint32_t>(util::Clamp0N(F11 - 1, roundf(position.x)));
  auto y = static_cast<uint32_t>(util::Clamp0N(F11 - 1, roundf(position.y)));

  // rgba are in [0f-1f] so multiply by 127 or 63.
  auto a =
      static_cast<uint32_t>(util::Clamp0N(F7 - 1, roundf(color.a * (I7 - 1))));
  auto r =
      static_cast<uint32_t>(util::Clamp0N(F6 - 1, roundf(color.r * (I6 - 1))));
  auto g =
      static_cast<uint32_t>(util::Clamp0N(F7 - 1, roundf(color.g * (I7 - 1))));
  auto b =
      static_cast<uint32_t>(util::Clamp0N(F6 - 1, roundf(color.b * (I6 - 1))));

  uint32_t upacked1 = (x << 13) | (a << 6) | (r << 0);
  uint32_t upacked2 = (y << 13) | (g << 6) | (b << 0);
  float fpacked1 = static_cast<float>(static_cast<double>(upacked1) / D24);
  float fpacked2 = static_cast<float>(static_cast<double>(upacked2) / D24);

  return glm::vec2(fpacked1, fpacked2);
}

void UnpackColorAndPosition(glm::vec2 packed, glm::vec4* color,
                            glm::vec2* position) {
  // Format is 48 bit -- x11a7r6, y11g7b6 -- using 24 bits in the mantissas of
  // each of the components of the return.
  //
  // Positions are given in [0u-2048u), and are packed into [0f-1f).
  //
  // Color rb are given in [0f-1f], and are compressed to [0u-64u) then packed
  // into [0f-1f).  Note the closed and half-open intervals.
  //
  // Color ag are given in the range [0f-1f], and are compress to [0u-128u) then
  // packed into [0f-1f).

  position->x = Fract(packed.x);
  position->x *= (F11);
  position->x = floorf(position->x);

  color->a = Fract(packed.x * F11);
  color->a *= (F7);  // expand [0f-1f) to [0-127]
  color->a = floorf(color->a);
  color->a /= (F7 - 1);  // convert [0-127] to [0f-1f]

  color->r = Fract(packed.x * F18);
  color->r *= (F6);  // expand [0f-1f) to [0-63]
  color->r = floorf(color->r);
  color->r /= (F6 - 1);  // convert [0-63] to [0f-1f]

  position->y = Fract(packed.y);
  position->y *= (F11);
  position->y = floorf(position->y);

  color->g = Fract(packed.y * F11);
  color->g *= (F7);  // expand [0f-1f) to [0-127]
  color->g = floorf(color->g);
  color->g /= (F7 - 1);  // convert [0-127] to [0f-1f]

  color->b = Fract(packed.y * F18);
  color->b *= (F6);  // expand [0f-1f) to [0-63]
  color->b = floorf(color->b);
  color->b /= (F6 - 1);  // convert [0-63] to [0f-1f]
}
}  // namespace ink
