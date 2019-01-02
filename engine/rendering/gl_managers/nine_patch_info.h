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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_INFO_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_INFO_H_

#include <cstddef>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/client_bitmap.h"

namespace ink {

// Holds points defining critical texture coords for a nine patch image
struct NinePatchInfo {
  bool is_nine_patch;
  glm::ivec2 size_in_px{0, 0};

  // Coordinates are in uv space [0-1]
  //
  // Given patches ordered like so:
  //  0 1 2
  //  3 4 5
  //  6 7 8
  //
  // xCriticalPoints[0] == 0.left()
  // xCriticalPoints[1] == 0.right() == 1.left()
  // xCriticalPoints[2] == 1.right() == 2.left()
  // xCriticalPoints[4] == 2.right()
  //
  // yCriticalPoints[0] == 0.upper()
  // yCriticalPoints[1] == 0.lower() == 3.upper()
  // yCriticalPoints[2] == 3.lower() == 6.upper()
  // yCriticalPoints[4] == 6.lower()
  //
  // StretchCriticalPoints mark the strechable content areas
  // (on the 9 patch image, these are the left and top borders)
  //
  // FillCriticalPoints mark the fill content areas
  // (on the 9 patch image, these are the right and bottom borders)
  float x_stretch_critical_points[4];
  float y_stretch_critical_points[4];
  float x_fill_critical_points[4];
  float y_fill_critical_points[4];
  glm::mat4 uv_to_texel{1};

  NinePatchInfo();
  explicit NinePatchInfo(const ClientBitmap& img);

  enum class BorderId { LEFT, TOP, RIGHT, BOTTOM };

 private:
  bool ReadBorderStartStop(const ClientBitmap& img, BorderId which_border,
                           std::pair<size_t, size_t>* result);
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_INFO_H_
