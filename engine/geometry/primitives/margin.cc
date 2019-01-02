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

#include "ink/engine/geometry/primitives/margin.h"

#include "third_party/absl/strings/str_format.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

Rect Margin::AdditiveOutset(Rect base_rect) const {
  Rect ans = base_rect;
  ans.from.x -= left;
  ans.from.y -= bottom;
  ans.to.x += right;
  ans.to.y += top;
  return ans;
}

bool Margin::IsEmpty() const {
  return left == 0 && right == 0 && top == 0 && bottom == 0;
}

std::string Margin::ToString() const {
  return absl::StrFormat(
      "margin:{top:%.0f, bottom:%.0f, left:%.0f, right:%.0f}", top, bottom,
      left, right);
}

Margin Margin::AsFractionOf(glm::ivec2 size) const {
  ASSERT(size.x > 0);
  ASSERT(size.y > 0);
  Margin ans;
  ans.top = top / size.y;
  ans.bottom = bottom / size.y;
  ans.left = left / size.x;
  ans.right = right / size.x;
  return ans;
}

void Margin::Clamp0N(float max_value) {
  top = util::Clamp0N(max_value, top);
  bottom = util::Clamp0N(max_value, bottom);
  left = util::Clamp0N(max_value, left);
  right = util::Clamp0N(max_value, right);
}

Rect Margin::MultiplicativeInset(Rect outer_rect) const {
  ASSERT(TotalVerticalMargin() < 1);
  ASSERT(TotalHorizontalMargin() < 1);
  Rect ans = outer_rect;
  ans.from.x += outer_rect.Width() * left;
  ans.from.y += outer_rect.Height() * bottom;
  ans.to.x -= outer_rect.Width() * right;
  ans.to.y -= outer_rect.Height() * top;
  return ans;
}

Rect Margin::MultiplicativeOutset(Rect inner_rect) const {
  ASSERT(TotalVerticalMargin() < 1);
  ASSERT(TotalHorizontalMargin() < 1);
  float final_width = inner_rect.Width() / (1 - TotalHorizontalMargin());
  float final_height = inner_rect.Height() / (1 - TotalVerticalMargin());
  Rect ans = inner_rect;
  ans.from.x -= final_width * left;
  ans.from.y -= final_height * bottom;
  ans.to.x += final_width * right;
  ans.to.y += final_height * top;
  return ans;
}
}  // namespace ink
