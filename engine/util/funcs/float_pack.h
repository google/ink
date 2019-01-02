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

#ifndef INK_ENGINE_UTIL_FUNCS_FLOAT_PACK_H_
#define INK_ENGINE_UTIL_FUNCS_FLOAT_PACK_H_

#include "third_party/glm/glm/glm.hpp"

namespace ink {

// Return fractional part of double.
float Fract(double value);

// Packs an ARGB [0f-1f] color and screen coordinates [0-2047] into a vec2.
//
// This will compress the colors to a7r6g7b6, which will correspond to
// fractions of 128 (7-bit) or 64 (6-bit).
//
// e.g. A red value of 0.1f will be converted to 13/128, which is 0.1015625.
//
// Values outside these ranges will be clamped to the allowed range.
glm::vec2 PackColorAndPosition(glm::vec4 color, glm::vec2 position);

// Unpack a7r6g7b6 color and [0-2047] screen coordinates.
//
// The color values will be in the range of [0-1f] but will be limited
// to fractions of 128 (7-bit) or 64 (6-bit).
void UnpackColorAndPosition(glm::vec2 packed, glm::vec4* color,
                            glm::vec2* position);

// Packs [0-4095] screen coordinates into the mantissa of a float.
float PackPosition(glm::vec2 position);

// Unpack [0-4095] screen coordinates from the mantissa of a float.
glm::vec2 UnpackPosition(float packed_position);

}  // namespace ink
#endif  // INK_ENGINE_UTIL_FUNCS_FLOAT_PACK_H_
