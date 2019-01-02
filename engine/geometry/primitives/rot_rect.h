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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_ROT_RECT_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_ROT_RECT_H_

#include <array>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {

// A rotatable rectangle, defined by its center, dimensions, and the
// counter-clockwise angle between the x-axis, and the axis along which its
// width is measured.
// RotRect's width must be greater than or equal to zero, but its height may be
// negative (this is equivalent to a flipped y-axis). Additionally, its angle of
// rotation must lie in the interval [0, 2π). If you attempt to specify a
// RotRect that does not satisfy these constraints, it will be normalized to an
// equivalent RotRect that does.
// Note: If your rectangle will always be axis-aligned, you may want to use the
// Rect class instead.
class RotRect {
 public:
  // Construct an empty RotRect.
  RotRect() : RotRect({0, 0}, {0, 0}, 0) {}

  // Construct from the center, dimensions, and rotation.
  RotRect(glm::vec2 center, glm::vec2 dim, float rotation_radians)
      : center_(center), dim_(dim), rotation_radians_(rotation_radians) {
    Normalize();
  }

  // Construct from an axis-aligned rectangle.
  explicit RotRect(const Rect &r) : RotRect(r.Center(), r.Dim(), 0) {}

  glm::vec2 Center() const { return center_; }
  glm::vec2 Dim() const { return dim_; }
  float Rotation() const { return rotation_radians_; }

  void SetCenter(glm::vec2 center) { center_ = center; }
  void SetDim(glm::vec2 dim) {
    dim_ = dim;
    Normalize();
  }
  void SetRotation(float rotation_radians) {
    rotation_radians_ = rotation_radians;
    Normalize();
  }

  float Width() const { return dim_.x; }
  float Height() const { return dim_.y; }

  // Returns the signed area of the rotated rectangle -- it will be negative if
  // the height is negative.
  float SignedArea() const { return Width() * Height(); }

  // Returns a copy of the rotated rectangle with the y-axis flipped, preserving
  // the center, x-axis, and rotation.
  RotRect InvertYAxis() const {
    return RotRect(Center(), {Width(), -Height()}, Rotation());
  }

  // Returns the four corners of the rotated rectangle. If the height is
  // positive, the points are ordered counter-clockwise, starting from the
  // "unrotated bottom-left"; if negative, they are ordered clockwise, starting
  // from the "unrotated top-left".
  // Informally, you can think of labeling the points of the unrotated
  // rectangle as "bottom-left", etc., then rotating the rectangle and
  // maintaining the same labels.
  std::vector<glm::vec2> Corners() const {
    float angle_cos = std::cos(Rotation());
    float angle_sin = std::sin(Rotation());
    glm::vec2 width_vector{.5 * Width() * angle_cos, .5 * Width() * angle_sin};
    glm::vec2 height_vector{-.5 * Height() * angle_sin,
                            .5 * Height() * angle_cos};
    return {Center() - width_vector - height_vector,
            Center() + width_vector - height_vector,
            Center() + width_vector + height_vector,
            Center() - width_vector + height_vector};
  }

  // Returns a transformation matrix that, if applied to this RotRect, would
  // result in the passed-in RotRect.
  glm::mat4 CalcTransformTo(const RotRect &other) const {
    return glm::translate(glm::mat4{1}, glm::vec3(other.Center(), 0)) *
           glm::rotate(glm::mat4{1}, other.Rotation(), glm::vec3(0, 0, 1)) *
           glm::scale(glm::mat4{1}, glm::vec3(other.Width() / Width(),
                                              other.Height() / Height(), 1)) *
           glm::rotate(glm::mat4{1}, -Rotation(), glm::vec3(0, 0, 1)) *
           glm::translate(glm::mat4{1}, glm::vec3(-Center(), 0));
  }

  std::string ToString() const {
    return Substitute("RotRect center $0, dimensions $1x$2, angle $3 ($4°)",
                      center_, Width(), Height(), Rotation(),
                      glm::degrees(Rotation()));
  }

 private:
  // Corrects the dimensions and angle of rotation to satisfy the constraints.
  void Normalize() {
    if (dim_.x < 0) {
      dim_ = -dim_;
      rotation_radians_ = NormalizeAnglePositive(rotation_radians_ + M_PI);
    } else {
      rotation_radians_ = NormalizeAnglePositive(rotation_radians_);
    }
  }

  glm::vec2 center_{0, 0};
  glm::vec2 dim_{0, 0};
  float rotation_radians_ = 0;
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_ROT_RECT_H_
