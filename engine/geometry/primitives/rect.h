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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_POINTS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_POINTS_H_

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "third_party/absl/strings/substitute.h"
#include "third_party/absl/types/optional.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace proto {
class Rect;
}  // namespace proto

// An axis aligned rectangle as specified by its bottom left corner and top
// right corner (x increases to the right, y increases as you go up).
// Note: If your rectangle needs to be rotatable, you'll likely want to use the
// RotRect class instead.
struct Rect {
  glm::vec2 to{0, 0};    // strictly >= from
  glm::vec2 from{0, 0};  // strictly <= to

  // A rectangle with all corners at (0,0), i.e. with area == 0.
  Rect();

  // The smallest rectangle containing u and v.
  Rect(glm::vec2 u, glm::vec2 v);

  // The smallest rectangle containing the points (x1,y1) and (x2,y2).
  Rect(float x1, float y1, float x2, float y2);

  static Rect CreateAtPoint(glm::vec2 center, float width, float height);
  static Rect CreateAtPoint(glm::vec2 center) {
    return Rect::CreateAtPoint(center, 0, 0);
  }

  glm::vec2 Center() const;
  float Width() const;
  float Height() const;
  float Area() const;
  bool Empty() const;

  // The vector (width(), height())
  glm::vec2 Dim() const;

  // Moves the center of the rectangle without changing the width or height.
  void SetCenter(glm::vec2 new_center);

  float Left() const;
  float Top() const;
  float Right() const;
  float Bottom() const;

  Segment LeftSegment() const;
  Segment TopSegment() const;
  Segment RightSegment() const;
  Segment BottomSegment() const;

  // Translate this s.t. left() = v, width and height unchanged.
  void SetLeft(float v);

  // Translate this s.t. top() = v, width and height unchanged.
  void SetTop(float v);

  // Translate this s.t. right() = v, width and height unchanged.
  void SetRight(float v);

  // Translate this s.t. bottom() = v, width and height unchanged.
  void SetBottom(float v);

  glm::vec2 Leftbottom() const;
  glm::vec2 Lefttop() const;
  glm::vec2 Rightbottom() const;
  glm::vec2 Righttop() const;

  bool operator==(const Rect& other) const;
  bool operator!=(const Rect& other) const;

  // Returns this translated by + (other,other)
  Rect operator+(float other) const;

  // Returns this translated by (-other,-other)
  Rect operator-(float other) const;

  // Returns this translated by + other
  Rect operator+(glm::vec2 other) const;

  // Returns this translated by - other
  Rect operator-(glm::vec2 other) const;

  // Returns this with each corner's coordinates scaled by * other
  Rect operator*(float other) const;

  size_t length() const { return 4; }
  size_t size() const { return 4; }

  // order is from.x, from.y, to.x, to.y
  float operator[](size_t i) const;
  float& operator[](size_t i);

  // Returns matrix M such that for each corner, M*this.corner == other.corner
  glm::mat4 CalcTransformTo(const Rect& other) const;
  glm::mat4 CalcTransformTo(const Rect& other, bool invert_yaxis) const;

  // Returns this with amount.x/amount.y taken off each side, e.g.:
  // Rect outer(0,0,10,8);
  // outer.inset(glm::vec2(3,2));
  //        ===> Rect(3,2,7,6)
  Rect Inset(glm::vec2 amount) const;
  Rect Inset(int amount) const { return Inset({amount, amount}); }

  // Returns this, scaled around center() by "amount"
  // Ex: scale(1.0) -> this
  //     scale(1.1) 10% larger in each dimension
  //                (new.width()  == old.width()  * 1.1)
  //                (new.height() == old.height() * 1.1)
  //     scale(0.9) 10% smaller in each dimension
  //                (new.width()  == old.width()  * 0.9)
  //                (new.height() == old.height() * 0.9)
  //
  // If you need to scale a Rect about a point other than the center, you can
  // do:
  // geometry::Transform(rect, matrix_utils::ScaleAboutPoint(scale, point))
  Rect Scale(float amount) const;

  // Checks if other lies within this, bounds checking inclusive.
  bool Contains(const Rect& other) const;

  // Checks if other lies within this, bounds checking inclusive.
  bool Contains(glm::vec2 other) const;

  // Returns the smallest rectangle containing this and other
  Rect Join(const Rect& other) const;

  // Returns the smallest rectangle containing this and pt
  Rect Join(glm::vec2 pt) const;

  // Updates this rectangle to be the smallest rectangle containing this and
  // other.
  void InplaceJoin(const Rect& other);

  // Updates this rectangle to be the smallest rectangle containing this and pt.
  void InplaceJoin(const glm::vec2& pt);

  // Returns the smallest rectangle such that:
  //   1. width/height = "targetAspectRatio"
  //   2. center of return equals center of this
  //   3. returned Rect contains this.
  Rect ContainingRectWithAspectRatio(float target_aspect_ratio) const;

  // Returns the smallest rectangle such that:
  //   1. width/height = "targetAspectRatio"
  //   2. center of return equals center of this
  //   3. returned Rect is contained by this.
  Rect InteriorRectWithAspectRatio(float target_aspect_ratio) const;

  // Returns the smallest rectangle such that:
  //   1. width >= min_dimensions.x
  //   2. height >= min_dimensions.y
  //   3. center of return equals center of this
  //   4. returned Rect contains this.
  Rect ContainingRectWithMinDimensions(glm::vec2 min_dimensions) const;

  // Returns a rectangle such that:
  //   1. width = the smaller of this' width and other's width
  //   2. height = the smaller of this' height and other's height
  //   3. returned Rect is contained by this
  //   4. the distance between the returned Rect's center and other's center is
  //      minimized
  // Informally, you can think of it as moving other to lie centered on this,
  // trimming off anything lies outside, and then moving it as close to its
  // original position while still remaining inside this.
  Rect ClosestInteriorRect(const Rect& other) const;

  // Returns width/height
  float AspectRatio() const;

  std::string ToString() const;

  static void WriteToProto(proto::Rect* proto_rect, const Rect& obj_rect);
  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const proto::Rect& proto_rect, Rect* obj_rect);
  bool IsValid() const;

  static Rect Lerpnc(Rect from, Rect to, float amount);
  static Rect Smoothstep(Rect from, Rect to, float amount);

 private:
  void CheckValid() const;
};

// Force Inplace* and CheckValid to be inlined.
inline void Rect::InplaceJoin(const Rect& other) {
  from.x = std::min(from.x, other.from.x);
  from.y = std::min(from.y, other.from.y);
  to.x = std::max(to.x, other.to.x);
  to.y = std::max(to.y, other.to.y);
  CheckValid();
}

inline void Rect::InplaceJoin(const glm::vec2& pt) {
  from.x = std::min(from.x, pt.x);
  from.y = std::min(from.y, pt.y);
  to.x = std::max(to.x, pt.x);
  to.y = std::max(to.y, pt.y);
  CheckValid();
}

inline void Rect::CheckValid() const { ASSERT(IsValid()); }

std::ostream& operator<<(std::ostream& oss, const Rect& Rect);

using OptRect = absl::optional<Rect>;

namespace util {
template <>
Rect Lerpnc(Rect from, Rect to, float amount);
template <>
Rect Smoothstep(Rect from, Rect to, float amount);

// If rect and other have values join other to rect, else assign other to rect.
void AssignOrJoinTo(const OptRect& other, OptRect* rect);

}  // namespace util

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_POINTS_H_
