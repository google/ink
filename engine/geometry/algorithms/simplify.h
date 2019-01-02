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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_SIMPLIFY_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_SIMPLIFY_H_

#include <iterator>
#include <type_traits>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/primitives/segment.h"

namespace ink {
namespace geometry {
namespace simplify_internal {

template <typename InputIterator, typename OutputIterator, typename Vec2Getter>
void SimplifyRecursionHelper(Segment seg, InputIterator interior_begin,
                             InputIterator interior_end, float epsilon,
                             Vec2Getter vec2_getter, OutputIterator output) {
  if (std::distance(interior_begin, interior_end) < 1) {
    // No interior points, stop recursing.
    return;
  }

  // Find the point that is furthest from the segment.
  InputIterator furthest_point;
  float max_distance = 0;
  for (auto it = interior_begin; it != interior_end; ++it) {
    float distance = Distance(seg, vec2_getter(*it));
    if (distance > max_distance) {
      furthest_point = it;
      max_distance = distance;
    }
  }

  if (max_distance > epsilon) {
    // Recursively simplify the points before the furthest point.
    SimplifyRecursionHelper(Segment(seg.from, vec2_getter(*furthest_point)),
                            interior_begin, furthest_point, epsilon,
                            vec2_getter, output);

    // Add the furthest point.
    *output++ = *furthest_point;

    // Recursively simplify the points after the furthest point.
    SimplifyRecursionHelper(Segment(vec2_getter(*furthest_point), seg.to),
                            furthest_point + 1, interior_end, epsilon,
                            vec2_getter, output);
  }
}

template <typename T>
class DefaultVec2Getter {
 public:
  glm::vec2 operator()(const T &t) { return glm::vec2(t); }
};

}  // namespace simplify_internal

// Polyline simplification using the Ramer-Douglas-Peucker algorithm
// (go/wiki/Ramer%e2%80%93Douglas%e2%80%93Peucker_algorithm).
// InputIterator and OutputIterator must operate on the same type.
// Vec2Getter must be a functor that takes the iterator type and returns a
// glm::vec2.
template <typename InputIterator, typename OutputIterator,
          typename Vec2Getter = simplify_internal::DefaultVec2Getter<
              const typename std::iterator_traits<InputIterator>::value_type>>
void Simplify(
    InputIterator begin, InputIterator end, float epsilon,
    OutputIterator output,
    Vec2Getter vec2_getter = simplify_internal::DefaultVec2Getter<
        const typename std::iterator_traits<InputIterator>::value_type>()) {
  static_assert(std::is_base_of<std::bidirectional_iterator_tag,
                                typename std::iterator_traits<
                                    InputIterator>::iterator_category>::value,
                "InputIterator must be a bidirectional iterator.");
  static_assert(std::is_same<std::output_iterator_tag,
                             typename std::iterator_traits<
                                 OutputIterator>::iterator_category>::value,
                "OutputIterator must be an output iterator.");

  if (std::distance(begin, end) < 3) {
    // There are less than three points, so there's nothing to do -- just copy
    // the input points (if any).
    for (auto it = begin; it != end; ++it) output = *it;
    return;
  }

  // Add the first point.
  *output++ = *begin;

  // Recursively simplify the interior points.
  simplify_internal::SimplifyRecursionHelper(
      Segment(vec2_getter(*begin), vec2_getter(*(end - 1))), begin + 1, end - 1,
      epsilon, vec2_getter, output);

  // Add the last point.
  *output++ = *(end - 1);
}

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_SIMPLIFY_H_
