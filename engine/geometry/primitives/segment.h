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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_SEGMENT_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_SEGMENT_H_

#include <limits>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

struct Segment {
  glm::vec2 from{0, 0};
  glm::vec2 to{0, 0};

  Segment() {}
  Segment(glm::vec2 from, glm::vec2 to) : from(from), to(to) {}
  Segment(float x1, float y1, float x2, float y2) : from(x1, y1), to(x2, y2) {}

  float Slope() const {
    float dy = to.y - from.y;
    float dx = to.x - from.x;
    if (dx == 0) {
      if (dy == 0) {
        return std::numeric_limits<float>::quiet_NaN();
      } else {
        return std::numeric_limits<float>::infinity();
      }
    }
    return dy / dx;
  }

  glm::vec2 DeltaVector() const { return to - from; }
  float Length() const { return glm::length(DeltaVector()); }

  // Evaluates the line defined by the points of the segment, such that the
  // start point is at t = 0 and the end point is at t = 1.
  glm::vec2 Eval(float t) const { return util::Lerpnc(from, to, t); }

  // Finds the parameter value of the closest point along the segment to the
  // given point.
  float NearestPoint(glm::vec2 point) {
    if (from == to) return 0;

    glm::vec2 segment_vector = to - from;
    glm::vec2 vector_to_project = point - from;
    return util::Clamp01(glm::dot(vector_to_project, segment_vector) /
                         glm::dot(segment_vector, segment_vector));
  }

  bool operator==(const Segment& other) const {
    return other.from == from && other.to == to;
  }

  bool operator!=(const Segment& other) const {
    return !(other.from == from && other.to == to);
  }

  std::string ToString() const { return Substitute("$0~$1", from, to); }
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_SEGMENT_H_
