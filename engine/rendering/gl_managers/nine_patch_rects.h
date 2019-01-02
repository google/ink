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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_RECTS_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_RECTS_H_

#include <cstddef>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/nine_patch_info.h"

namespace ink {

// Helper class to translate 9patch critical points into rectangles
class NinePatchRects {
 public:
  explicit NinePatchRects(NinePatchInfo npi);

  // Calculate rectangle positions given a content rectangle to fit across.
  //
  // pxToContentDist defines the x and y distance transforms between uv space
  // and result space.
  //
  // EX: to calculate 9patch position rectangles around a rectangle
  // "worldSpaceRect" in world space:
  // calcPositionRects(transformFromPXtoWORLD, worldSpaceRect)
  //
  // Results are in row major order -- result[row * 3 + col] is the same
  // relative position as uvRectAt(row, col)
  std::vector<Rect> CalcPositionRects(glm::vec2 px_dist_to_content_dist,
                                      Rect content);

  // Rects are numbered like so:
  //  0 1 2
  //  3 4 5
  //  6 7 8
  Rect UvRectAt(size_t row, size_t col);

 private:
  Rect CreateCorner(glm::vec2 content_corner_location, size_t row, size_t col,
                    glm::vec2 uv_dist_to_world_dist);

 private:
  NinePatchInfo npi_;

  // Rects are numbered like so:
  //  0 1 2
  //  3 4 5
  //  6 7 8
  Rect uv_stretch_rects[3][3];
  Rect uv_fill_rects[3][3];

  // Center based coords arranged like so:
  // (-1,1)  (0,1)  (1,1)
  // (-1,0)  (0,0)  (1,0)
  // (-1,-1) (0,-1) (1,-1)
  glm::vec2 center_based_coords[3][3];
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_NINE_PATCH_RECTS_H_
