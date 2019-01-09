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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_TRIANGLE_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_TRIANGLE_H_

#include <array>
#include <string>
#include <vector>

#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {
namespace geometry {

class Triangle {
 public:
  Triangle() {}
  Triangle(float x1, float y1, float x2, float y2, float x3, float y3)
      : Triangle(glm::vec2(x1, y1), glm::vec2(x2, y2), glm::vec2(x3, y3)) {}
  Triangle(const glm::vec2 &point1, const glm::vec2 &point2,
           const glm::vec2 &point3)
      : points_{{point1, point2, point3}} {}

  // Returns the signed area of the triangle. A counter-clockwise triangle
  // yields a positive value, while a clockwise one yields a negative value.
  float SignedArea() const {
    return .5 * Determinant(points_[1] - points_[0], points_[2] - points_[1]);
  }

  // Returns true if all three points of the triangle line on a single line,
  // allowing for floating-point error (see Orientation() in vector_utils.h).
  bool IsDegenerate() const;

  // Returns true if the point is inside the triangle. Note that points along
  // the edge are considered inside.
  bool Contains(glm::vec2 test_point) const;

  std::vector<glm::vec2> Points() const {
    return {points_[0], points_[1], points_[2]};
  }

  // Returns the barycentric coordinates of the given position w.r.t. this
  // triangle (go/wiki/Barycentric_coordinate_system).
  glm::vec3 ConvertToBarycentric(glm::vec2 position) const {
    glm::vec2 b = glm::inverse(glm::mat2(points_[0] - points_[2],
                                         points_[1] - points_[2])) *
                  (position - points_[2]);
    return {b, 1 - b.x - b.y};
  }

  glm::vec2 &operator[](size_t index) { return points_[index]; }
  const glm::vec2 &operator[](size_t index) const { return points_[index]; }

  std::string ToString() const {
    return Substitute("Triangle: $0, $1, $2", points_[0], points_[1],
                      points_[2]);
  }

 private:
  std::array<glm::vec2, 3> points_;
};

// Performs a linear barycentric interpolation over the given values
// (go/wiki/Barycentric_coordinate_system).
template <typename T>
T BarycentricInterpolate(glm::vec3 barycentric, const T &t0, const T &t1,
                         const T &t2) {
  //  ASSERT(barycentric.x + barycentric.y + barycentric.z == 1);
  return barycentric.x * t0 + barycentric.y * t1 + barycentric.z * t2;
}

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_TRIANGLE_H_
