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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_MARGIN_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_MARGIN_H_

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"

namespace ink {

struct Margin {
  float top;
  float bottom;
  float left;
  float right;

  Margin() : top(0), bottom(0), left(0), right(0) {}

  Rect AdditiveOutset(Rect base_rect) const;

  bool IsEmpty() const;

  // Returns a new margin where each component is given as a fraction of the
  // corresponding dimensions in size, e.g.:
  //
  // ans.top = top/size.y
  // ans.right = right/size.x
  //
  Margin AsFractionOf(glm::ivec2 size) const;

  float TotalVerticalMargin() const { return top + bottom; }
  float TotalHorizontalMargin() const { return left + right; }

  // Clamps each component to lie in [0,maxSize]
  void Clamp0N(float max_value);

  // Interpreting the size of each margin as a fraction of the total length of a
  // side including the margin, and outerRect as the box containing both the
  // content and the margin, returns the box containing the content.
  //
  // E.g. if
  //   outerRect = (left = 0, bottom =  0, right =   2, top =   2)
  //   this      = (left =.1, bottom = .2, right =  .3, top =  .4),
  // then
  //   ans       = (left =.2, bottom = .4, right = 1.4, top = 1.2),
  //
  // It is an error to call this function if totalVerticalMargin() or
  // totalHorizontalMargin() are greater than one.
  Rect MultiplicativeInset(Rect outer_rect) const;

  // Interpreting the size of each margin as a fraction of the total length of a
  // side including the margin, returns a Rect "ans" such that "innerRect" is
  // fractionally inset from ans by this.
  //
  // E.g. if
  //   innerRect = (left =.2, bottom = .4, right = 1.4, top = 1.2),
  //   this      = (left =.1, bottom = .2, right =  .3, top =  .4),
  // then
  //   ans       = (left = 0, bottom =  0, right =   2, top =   2)
  //
  // Note: multiplicativeInset and multiplicativeOutset are inverses.
  //
  // It is an error to call this function if totalVerticalMargin() or
  // totalHorizontalMargin() are greater than one.
  Rect MultiplicativeOutset(Rect inner_rect) const;

  std::string ToString() const;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_MARGIN_H_
