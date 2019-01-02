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

#ifndef INK_ENGINE_COLORS_COLORS_H_
#define INK_ENGINE_COLORS_COLORS_H_

#include <cstdint>

#include "third_party/glm/glm/glm.hpp"
namespace ink {

glm::vec4 RGBtoHSV(glm::vec4 rgb);
glm::vec4 HSVtoRGB(glm::vec4 hsv);
glm::vec4 RGBtoRGBPremultiplied(glm::vec4 rgb);
glm::vec4 RGBPremultipliedtoRGB(glm::vec4 rgb);

// abgr from most significant to least significant bit
glm::vec4 UintToVec4ABGR(uint32_t abgr);
uint32_t Vec4ToUintABGR(glm::vec4 c);

// argb from most significant to least significant bit
glm::vec4 UintToVec4ARGB(uint32_t c);
uint32_t Vec4ToUintARGB(glm::vec4 c);

// rgba from most significant to least significant bit
glm::vec4 UintToVec4RGBA(uint32_t rgba);
uint32_t Vec4ToUintRGBA(glm::vec4 c);

const glm::vec4 kGoogleBlue500(0.259f, 0.522f, 0.957f, 1.0f);
const glm::vec4 kGoogleBlue200(0.631f, 0.761f, 0.980f, 1.0f);

}  // namespace ink
#endif  // INK_ENGINE_COLORS_COLORS_H_
