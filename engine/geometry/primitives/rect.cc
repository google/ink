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

#include "ink/engine/geometry/primitives/rect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <ostream>

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/proto/geometry_portable_proto.pb.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;

namespace ink {
Rect::Rect() {}
Rect::Rect(vec2 u, vec2 v) {
  from.y = std::min(u.y, v.y);
  from.x = std::min(u.x, v.x);
  to.y = std::max(u.y, v.y);
  to.x = std::max(u.x, v.x);
}
Rect::Rect(float x1, float y1, float x2, float y2)
    : Rect(vec2(x1, y1), vec2(x2, y2)) {}

// static
Rect Rect::CreateAtPoint(vec2 center, float width, float height) {
  Rect ans(0, 0, width, height);
  ans.SetCenter(center);
  return ans;
}

bool Rect::IsValid() const {
  return (from.y == std::min(from.y, to.y)) &&
         (from.x == std::min(from.x, to.x)) &&
         (to.y == std::max(from.y, to.y)) && (to.x == std::max(from.x, to.x));
}

vec2 Rect::Center() const { return (to + from) / 2.0f; }

void Rect::SetCenter(vec2 new_center) {
  auto offset = -Center();
  offset += new_center;
  to += offset;
  from += offset;
}

float Rect::Width() const { return std::abs(to.x - from.x); }
float Rect::Height() const { return std::abs(to.y - from.y); }
float Rect::Area() const { return Width() * Height(); }
bool Rect::Empty() const {
  return Area() < std::numeric_limits<float>::epsilon();
}
vec2 Rect::Dim() const { return vec2(Width(), Height()); }
bool Rect::operator==(const Rect& other) const {
  return to == other.to && from == other.from;
}
bool Rect::operator!=(const Rect& other) const { return !(*this == other); }
float Rect::Left() const {
  CheckValid();
  return from.x;
}
float Rect::Right() const {
  CheckValid();
  return to.x;
}
float Rect::Top() const {
  CheckValid();
  return to.y;
}
float Rect::Bottom() const {
  CheckValid();
  return from.y;
}

Segment Rect::LeftSegment() const { return Segment(Leftbottom(), Lefttop()); }
Segment Rect::TopSegment() const { return Segment(Lefttop(), Righttop()); }
Segment Rect::RightSegment() const {
  return Segment(Rightbottom(), Righttop());
}
Segment Rect::BottomSegment() const {
  return Segment(Leftbottom(), Rightbottom());
}

void Rect::SetLeft(float v) {
  CheckValid();
  auto w = Width();
  from.x = v;
  to.x = v + w;
}
void Rect::SetTop(float v) {
  CheckValid();
  auto h = Height();
  to.y = v;
  from.y = v - h;
}
void Rect::SetRight(float v) {
  CheckValid();
  auto w = Width();
  to.x = v;
  from.x = v - w;
}
void Rect::SetBottom(float v) {
  CheckValid();
  auto h = Height();
  from.y = v;
  to.y = v + h;
}

vec2 Rect::Lefttop() const { return vec2(Left(), Top()); }
vec2 Rect::Leftbottom() const {
  CheckValid();
  return from;
}
vec2 Rect::Righttop() const {
  CheckValid();
  return to;
}
vec2 Rect::Rightbottom() const { return vec2(Right(), Bottom()); }

bool Rect::Contains(const Rect& other) const {
  return Left() <= other.Left() && Top() >= other.Top() &&
         Right() >= other.Right() && Bottom() <= other.Bottom();
}

bool Rect::Contains(glm::vec2 other) const {
  return Left() <= other.x && Top() >= other.y && Right() >= other.x &&
         Bottom() <= other.y;
}

std::string Rect::ToString() const {
  return Substitute("$0x$1 $2 -> $3, center $4", Width(), Height(), from, to,
                    Center());
}

Rect Rect::Join(const Rect& other) const {
  Rect res;
  res.from.x = std::min(Left(), other.Left());
  res.from.y = std::min(Bottom(), other.Bottom());
  res.to.x = std::max(Right(), other.Right());
  res.to.y = std::max(Top(), other.Top());

  res.CheckValid();
  return res;
}

Rect Rect::Join(vec2 pt) const {
  Rect res;
  res.from.x = std::min(Left(), pt.x);
  res.from.y = std::min(Bottom(), pt.y);
  res.to.x = std::max(Right(), pt.x);
  res.to.y = std::max(Top(), pt.y);

  res.CheckValid();
  return res;
}

Rect Rect::operator+(vec2 other) const {
  Rect res = *this;
  res.from += other;
  res.to += other;
  return res;
}

Rect Rect::operator-(vec2 other) const { return (*this + (-other)); }

Rect Rect::operator+(float other) const {
  Rect res = *this;
  res.from += other;
  res.to += other;
  return res;
}

Rect Rect::operator-(float other) const { return (*this + (-other)); }

Rect Rect::operator*(float other) const {
  return Rect(from * other, to * other);
}

float Rect::operator[](size_t i) const { return (*const_cast<Rect*>(this))[i]; }

float& Rect::operator[](size_t i) {
  ASSERT(i < 4);
  if (i <= 1) {
    return from[i];
  } else {
    return to[i - 2];
  }
}

glm::mat4 Rect::CalcTransformTo(const Rect& other) const {
  return CalcTransformTo(other, false);
}

glm::mat4 Rect::CalcTransformTo(const Rect& other, bool invert_yaxis) const {
  return glm::translate(mat4{1}, vec3(other.Center(), 0)) *
         glm::scale(mat4{1}, vec3(other.Width() / Width(),
                                  other.Height() / Height(), 1)) *
         (invert_yaxis ? glm::scale(mat4{1}, vec3(1, -1, 1)) : mat4{1}) *
         glm::translate(mat4{1}, vec3(-Center(), 0));
}

Rect Rect::Inset(vec2 amount) const {
  Rect res = *this;
  res.to -= amount;
  res.from += amount;
  return res;
}

Rect Rect::Scale(float amount) const {
  return Inset(0.5f * ((1 - amount) * Dim()));
}

Rect Rect::ContainingRectWithAspectRatio(float target_aspect_ratio) const {
  float current_aspect_ratio = AspectRatio();
  float corrected_width;
  float corrected_height;
  if (target_aspect_ratio > current_aspect_ratio) {
    corrected_height = Height();
    corrected_width = Height() * target_aspect_ratio;
  } else {
    corrected_width = Width();
    corrected_height = Width() / target_aspect_ratio;
  }
  return CreateAtPoint(Center(), corrected_width, corrected_height);
}

Rect Rect::InteriorRectWithAspectRatio(float target_aspect_ratio) const {
  float current_aspect_ratio = AspectRatio();
  float corrected_width;
  float corrected_height;
  if (target_aspect_ratio > current_aspect_ratio) {
    corrected_width = Width();
    corrected_height = Width() / target_aspect_ratio;
  } else {
    corrected_height = Height();
    corrected_width = Height() * target_aspect_ratio;
  }
  return CreateAtPoint(Center(), corrected_width, corrected_height);
}

Rect Rect::ContainingRectWithMinDimensions(glm::vec2 min_dimensions) const {
  return CreateAtPoint(Center(), std::max(Width(), min_dimensions.x),
                       std::max(Height(), min_dimensions.y));
}

Rect Rect::ClosestInteriorRect(const Rect& other) const {
  Rect interior = other;
  if (Width() < interior.Width()) {
    interior.from.x = Left();
    interior.to.x = Right();
  } else if (Left() > interior.Left()) {
    interior.SetLeft(Left());
  } else if (Right() < interior.Right()) {
    interior.SetRight(Right());
  }

  if (Height() < interior.Height()) {
    interior.from.y = Bottom();
    interior.to.y = Top();
  } else if (Bottom() > interior.Bottom()) {
    interior.SetBottom(Bottom());
  } else if (Top() < interior.Top()) {
    interior.SetTop(Top());
  }

  return interior;
}

float Rect::AspectRatio() const { return Width() / Height(); }

// static
void Rect::WriteToProto(ink::proto::Rect* proto_rect, const Rect& obj_rect) {
  proto_rect->set_xlow(obj_rect.from.x);
  proto_rect->set_ylow(obj_rect.from.y);
  proto_rect->set_xhigh(obj_rect.to.x);
  proto_rect->set_yhigh(obj_rect.to.y);
}

// static
Status Rect::ReadFromProto(const ink::proto::Rect& proto_rect, Rect* obj_rect) {
  obj_rect->from.x = proto_rect.xlow();
  obj_rect->from.y = proto_rect.ylow();
  obj_rect->to.x = proto_rect.xhigh();
  obj_rect->to.y = proto_rect.yhigh();
  return obj_rect->IsValid()
             ? OkStatus()
             : status::InvalidArgument("$0 is not valid", *obj_rect);
}

Rect Rect::Lerpnc(Rect from, Rect to, float amount) {
  Rect res;
  res.from = util::Lerpnc(from.from, to.from, amount);
  res.to = util::Lerpnc(from.to, to.to, amount);
  return res;
}

Rect Rect::Smoothstep(Rect from, Rect to, float amount) {
  return Rect::CreateAtPoint(
      util::Smoothstep(from.Center(), to.Center(), amount),   // center
      util::Smoothstep(from.Width(), to.Width(), amount),     // width
      util::Smoothstep(from.Height(), to.Height(), amount));  // height
}

std::ostream& operator<<(std::ostream& oss, const Rect& Rect) {
  oss << Rect.ToString();
  return oss;
}

namespace util {

template <>
Rect Lerpnc(Rect from, Rect to, float amount) {
  return Rect::Lerpnc(from, to, amount);
}

template <>
Rect Smoothstep(Rect from, Rect to, float amount) {
  return Rect::Smoothstep(from, to, amount);
}

void AssignOrJoinTo(const OptRect& other, OptRect* rect) {
  if (rect->has_value() && other.has_value()) {
    rect->value().InplaceJoin(other.value());
  } else if (!rect->has_value()) {
    *rect = other;
  }
}

}  // namespace util

}  // namespace ink
