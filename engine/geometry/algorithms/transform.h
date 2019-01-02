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

#ifndef INK_ENGINE_GEOMETRY_ALGORITHMS_TRANSFORM_H_
#define INK_ENGINE_GEOMETRY_ALGORITHMS_TRANSFORM_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/geometry/primitives/triangle.h"

namespace ink {
namespace geometry {

// These functions apply a two-dimensional affine transformation to the given
// geometry. Note that the transformation is specified by a 4x4 matrix, and as
// such, it is assumed to have the form:
// ⎡a b 0 c⎤
// ⎢d e 0 f⎥
// ⎢0 0 1 0⎥
// ⎣0 0 0 1⎦

inline glm::vec2 Transform(glm::vec2 vec, const glm::mat4 &matrix) {
  return glm::vec2(matrix * glm::vec4{vec, 0, 1});
}
inline Segment Transform(const Segment &segment, const glm::mat4 &matrix) {
  return Segment(Transform(segment.from, matrix),
                 Transform(segment.to, matrix));
}
inline Triangle Transform(const Triangle &triangle, const glm::mat4 &matrix) {
  return Triangle(Transform(triangle[0], matrix),
                  Transform(triangle[1], matrix),
                  Transform(triangle[2], matrix));
}

// Note that, in the general case, a transformed Rect will be a parallelogram,
// and may not be rectangular or axis-aligned. As such, this function returns
// the envelope of the parallelogram.
inline Rect Transform(const Rect &rectangle, const glm::mat4 &matrix) {
  return Envelope({Transform(rectangle.Leftbottom(), matrix),
                   Transform(rectangle.Lefttop(), matrix),
                   Transform(rectangle.Rightbottom(), matrix),
                   Transform(rectangle.Righttop(), matrix)});
}

// Note that, in the general case, a transformed RotRect will be a
// parallelogram, and may not be rectangular. As such, this function returns the
// smallest RotRect that contains the parallelogram, choosing the orientation
// that is the closest match.
RotRect Transform(const RotRect &rectangle, const glm::mat4 &matrix);

// Convenience function to apply a transformation to a range of elements.
// InputIterator and OutputIterator must operate on the same type.
template <typename InputIterator, typename OutputIterator>
void Transform(InputIterator begin, InputIterator end, const glm::mat4 &matrix,
               OutputIterator output) {
  for (auto it = begin; it != end; ++it) *output++ = Transform(*it, matrix);
}

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_ALGORITHMS_TRANSFORM_H_
