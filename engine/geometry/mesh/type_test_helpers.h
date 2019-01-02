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

// This is a utility used in several of this tests in this directory.  It should
// not be included in actual production code.

#ifndef INK_ENGINE_GEOMETRY_MESH_TYPE_TEST_HELPERS_H_
#define INK_ENGINE_GEOMETRY_MESH_TYPE_TEST_HELPERS_H_

#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/primitive_test_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace funcs {

inline bool CheckColor(const std::vector<Vertex>& vertices,
                       const glm::vec4& color_expected) {
  bool ans = true;
  for (const Vertex& v : vertices) {
    if (!::testing::Matches(Vec4Near(color_expected, 0.0001))(v.color)) {
      ans = false;
      SLOG(SLOG_ERROR, "Expected color: $0  but found vertex: $1.",
           color_expected, v);
    }
  }
  return ans;
}

// Assumes that the four corners of the rectangle are unique, i.e. that the
// rectangle has positive area.  Output will be meaningless otherwise.
//
// The following conditions are necessary and sufficient for a trianglution to
// be valid:
//   C1. The triangulation contains 6 vertices.
//   C2. The vertices each match one of the expected corners
//   C3. The first three vertices match unique corners
//   C4. Each corner is matched exactly once or twice
//   C5. The number of times the top left and bottom right corners are matched
//   are the same.
//
// To justify the claim, note (C1-5) they imply that either:
//   a. Top Right, Bottom Left = 2, Top Left, Bottom Right  = 1,
//   b. Top Right, Bottom Left = 1, Top Left, Bottom Right  = 2,
//
template <typename T>
bool IsValidRectangleTriangulation(const std::vector<T>& triangulation,
                                   const glm::vec2& bottom_left_expected,
                                   const glm::vec2& bottom_right_expected,
                                   const glm::vec2& top_left_expected,
                                   const glm::vec2& top_right_expected) {
  bool ans = true;
  // Check C1
  if (triangulation.size() != 6) {
    ans = false;
    SLOG(SLOG_ERROR, "Expected 6 vertices in triangulation but found  $0",
         triangulation.size());
  }
  // the number of times each corner of the rectange is returned.
  int count_bl = 0;
  int count_br = 0;
  int count_tl = 0;
  int count_tr = 0;

  int i = 0;
  for (const T& v : triangulation) {
    // required as v.position may be a vec3
    glm::vec2 position(v.position);
    if (::testing::Matches(Vec2Near(position, 0.0001))(bottom_left_expected)) {
      count_bl++;
    } else if (::testing::Matches(Vec2Near(position, 0.0001))(
                   bottom_right_expected)) {
      count_br++;
    } else if (::testing::Matches(Vec2Near(position, 0.0001))(
                   top_left_expected)) {
      count_tl++;
    } else if (::testing::Matches(Vec2Near(position, 0.0001))(
                   top_right_expected)) {
      count_tr++;
    }
    i++;
    if (i == 3) {
      // Check C3
      if (count_bl > 1 || count_br > 1 || count_tl > 1 || count_tr > 1) {
        SLOG(SLOG_ERROR,
             "First three vertices should have unique locations but found: "
             "BL = $0, BR = $1, TL= $2, TR = $3.",
             count_bl, count_br, count_tl, count_tr);
        ans = false;
      }
    }
  }

  int matches = count_bl + count_br + count_tl + count_tr;
  // Check C2
  if (matches != 6) {
    SLOG(SLOG_ERROR, "Expected 6 matches but found: $0", matches);
    ans = false;
  }

  // Check C4
  if (count_bl != 1 && count_bl != 2) {
    SLOG(SLOG_ERROR, "Expected bottom left to be one or two but found: $0",
         count_bl);
    ans = false;
  }
  if (count_br != 1 && count_br != 2) {
    SLOG(SLOG_ERROR, "Expected bottom right to be one or two but found: $0",
         count_br);
    ans = false;
  }
  if (count_tl != 1 && count_tl != 2) {
    SLOG(SLOG_ERROR, "Expected top left to be one or two but found: $0",
         count_tl);
    ans = false;
  }
  if (count_tr != 1 && count_tr != 2) {
    SLOG(SLOG_ERROR, "Expected top right to be one or two but found: $0",
         count_tr);
    ans = false;
  }

  // Check C5
  if (count_tl != count_br) {
    SLOG(SLOG_ERROR, "Expected TL == BR but found: TL: $0 and BR: $1", count_tl,
         count_br);
    ans = false;
  }
  return ans;
}

template <typename T>
bool IsValidRectangleTriangulation(const std::vector<T>& triangulation,
                                   const Rect& rect_expected) {
  return IsValidRectangleTriangulation(
      triangulation, rect_expected.Leftbottom(), rect_expected.Rightbottom(),
      rect_expected.Lefttop(), rect_expected.Righttop());
}

// Warning: Rect assumes a coordinate system where (0,0) is bottom left. Use
// CheckTextureCoorsInverted must be used to check texture coordinates when the
// textures are y-axis inverted.
template <typename T>
void CheckTextureCoors(const std::vector<T>& triangulation,
                       const Rect& texture_coors, const Rect& quad_coors) {
  for (const T& v : triangulation) {
    // required as v.position may be a vec3
    glm::vec2 position(v.position);
    if (::testing::Matches(Vec2Near(quad_coors.Leftbottom(), 0.0001))(
            position)) {
      SCOPED_TRACE("Left bottom texture coord");
      EXPECT_THAT(v.texture_coords,
                  Vec2Near(texture_coors.Leftbottom(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Rightbottom(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Right bottom texture coord");
      EXPECT_THAT(v.texture_coords,
                  Vec2Near(texture_coors.Rightbottom(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Lefttop(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Left top texture coord");
      EXPECT_THAT(v.texture_coords, Vec2Near(texture_coors.Lefttop(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Righttop(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Right top texture coord");
      EXPECT_THAT(v.texture_coords, Vec2Near(texture_coors.Righttop(), 0.0001));
    }
  }
}

void CheckTextureCoors(const Mesh& mesh, const Rect& texture_coors,
                       const Rect& quad_coors);

template <typename T>
void CheckTextureCoorsInverted(const std::vector<T>& triangulation,
                               const Rect& texture_coors,
                               const Rect& quad_coors) {
  for (const T& v : triangulation) {
    // required as v.position may be a vec3
    glm::vec2 position(v.position);
    if (::testing::Matches(Vec2Near(quad_coors.Leftbottom(), 0.0001))(
            position)) {
      SCOPED_TRACE("Left bottom texture coord");
      EXPECT_THAT(v.texture_coords, Vec2Near(texture_coors.Lefttop(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Rightbottom(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Right bottom texture coord");
      EXPECT_THAT(v.texture_coords, Vec2Near(texture_coors.Righttop(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Lefttop(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Left top texture coord");
      EXPECT_THAT(v.texture_coords,
                  Vec2Near(texture_coors.Leftbottom(), 0.0001));
    } else if (::testing::Matches(Vec2Near(quad_coors.Righttop(), 0.0001))(
                   position)) {
      SCOPED_TRACE("Right top texture coord");
      EXPECT_THAT(v.texture_coords,
                  Vec2Near(texture_coors.Rightbottom(), 0.0001));
    }
  }
}

void CheckTextureCoorsInverted(const Mesh& mesh, const Rect& texture_coors,
                               const Rect& quad_coors);

bool IsValidRectangleTriangulation(const Mesh& mesh, const Rect& rect_expected);

}  // namespace funcs
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_MESH_TYPE_TEST_HELPERS_H_
