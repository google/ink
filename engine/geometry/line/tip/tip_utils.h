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

#ifndef INK_ENGINE_GEOMETRY_LINE_FAT_LINE_UTILS_H_
#define INK_ENGINE_GEOMETRY_LINE_FAT_LINE_UTILS_H_

#include <vector>

#include "ink/engine/geometry/line/mid_point.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/segment.h"

namespace ink {

CircleTangents FindLineTangents(glm::vec2 start, float start_radius,
                                glm::vec2 end, float end_radius);

inline CircleTangents FindLineTangents(const MidPoint& start,
                                       const MidPoint& end) {
  return FindLineTangents(start.screen_position, start.tip_size.radius,
                          end.screen_position, end.tip_size.radius);
}

void JoinLineTangents(glm::vec2 turn_point, float turn_radius,
                      Segment in_tangent, Segment out_tangent, float turn_angle,
                      bool is_left_edge, uint32_t n_turn_vertices,
                      std::function<void(glm::vec2 pt)> append_function);

void AddRoundTurnPoints(const MidPoint& start, const MidPoint& middle,
                        const MidPoint& end, int turn_vertices,
                        std::function<void(glm::vec2 pt)> add_left,
                        std::function<void(glm::vec2 pt)> add_right);

// Return true if the circle described by inner is entirely within the circle
// described by outer.
bool IsCircleWithinCircle(const MidPoint& inner, const MidPoint& outer);

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_LINE_FAT_LINE_UTILS_H_
