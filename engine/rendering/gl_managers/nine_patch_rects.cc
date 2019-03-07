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

#include "ink/engine/rendering/gl_managers/nine_patch_rects.h"

#include <type_traits>

namespace ink {

NinePatchRects::NinePatchRects(NinePatchInfo npi) : npi_(npi) {
  static_assert(
      std::extent<decltype(npi.x_stretch_critical_points)>::value == 4,
      "xStretchCriticalPoints is an unexpected size");
  static_assert(
      std::extent<decltype(npi.y_stretch_critical_points)>::value == 4,
      "yStretchCriticalPoints is an unexpected size");
  static_assert(std::extent<decltype(npi.x_fill_critical_points)>::value == 4,
                "xFillCriticalPoints is an unexpected size");
  static_assert(std::extent<decltype(npi.y_fill_critical_points)>::value == 4,
                "yFillCriticalPoints is an unexpected size");

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      uv_stretch_rects[row][col] = Rect(npi.x_stretch_critical_points[col],
                                        npi.y_stretch_critical_points[row],
                                        npi.x_stretch_critical_points[col + 1],
                                        npi.y_stretch_critical_points[row + 1]);
      uv_fill_rects[row][col] =
          Rect(npi.x_fill_critical_points[col], npi.y_fill_critical_points[row],
               npi.x_fill_critical_points[col + 1],
               npi.y_fill_critical_points[row + 1]);
      center_based_coords[row][col] = glm::vec2(col - 1, 1 - row);
    }
  }
}

std::vector<Rect> NinePatchRects::CalcPositionRects(
    glm::vec2 px_dist_to_content_dist, Rect content) {
  glm::vec2 uv_dist_to_world_dist =
      px_dist_to_content_dist *
      glm::vec2(npi_.uv_to_texel[0][0], npi_.uv_to_texel[1][1]);

  Rect world_rects[3][3];

  // row 1
  world_rects[0][0] =
      CreateCorner(content.Lefttop(), 0, 0, uv_dist_to_world_dist);
  world_rects[0][2] =
      CreateCorner(content.Righttop(), 0, 2, uv_dist_to_world_dist);
  world_rects[0][1] =
      Rect(world_rects[0][0].Rightbottom(), world_rects[0][2].Lefttop());

  // row 3
  world_rects[2][0] =
      CreateCorner(content.Leftbottom(), 2, 0, uv_dist_to_world_dist);
  world_rects[2][2] =
      CreateCorner(content.Rightbottom(), 2, 2, uv_dist_to_world_dist);
  world_rects[2][1] =
      Rect(world_rects[2][0].Rightbottom(), world_rects[2][2].Lefttop());

  // row 2
  world_rects[1][0] =
      Rect(world_rects[2][0].Lefttop(), world_rects[0][0].Rightbottom());
  world_rects[1][2] =
      Rect(world_rects[2][2].Lefttop(), world_rects[0][2].Rightbottom());
  // don't create [1][1] (it should be transparent)

  return std::vector<Rect>(reinterpret_cast<Rect*>(world_rects),
                           reinterpret_cast<Rect*>(world_rects) + 9);
}

Rect NinePatchRects::UvRectAt(size_t row, size_t col) {
  return uv_stretch_rects[row][col];
}

Rect NinePatchRects::CreateCorner(glm::vec2 content_corner_location, size_t row,
                                  size_t col, glm::vec2 uv_dist_to_world_dist) {
  const auto& stretch = uv_stretch_rects[row][col];
  const auto& fill = uv_fill_rects[row][col];
  const auto& cbcoords = center_based_coords[row][col];

  // create a Rect of the correct size centered at the content corner
  glm::vec2 uv_dim = stretch.Dim() * uv_dist_to_world_dist;
  Rect world = Rect::CreateAtPoint(content_corner_location, uv_dim.x, uv_dim.y);

  // move the Rect away from the content corner, in the direction of no longer
  // covering the content
  world = world + ((0.5f * cbcoords) * world.Dim());

  // move the Rect back into the content, according to how much of the stretch
  // Rect is set as "content fill"
  world = world +
          uv_dist_to_world_dist * (-cbcoords * (stretch.Dim() - fill.Dim()));
  return world;
}
}  // namespace ink
