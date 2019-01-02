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

#ifndef INK_ENGINE_GEOMETRY_LINE_TIP_MODEL_H_
#define INK_ENGINE_GEOMETRY_LINE_TIP_MODEL_H_

#include "ink/engine/geometry/line/mid_point.h"
#include "ink/engine/geometry/line/tip_type.h"

namespace ink {

// A tip model is a helper for FatLine that converts MidPoints (modeled points)
// to vertices for tessellation.
class TipModel {
 public:
  virtual ~TipModel() {}

  // Clear state between lines
  virtual void Clear() {}

  virtual void AddTurnPoints(const MidPoint& start, const MidPoint& middle,
                             const MidPoint& end, int turn_vertices,
                             std::function<void(glm::vec2 pt)> add_left,
                             std::function<void(glm::vec2 pt)> add_right) = 0;

  virtual TipType GetTipType() const = 0;

  // turn_vertices is the number of vertices used for a turn at this size and
  // does not necessarily impact the number of vertices of the startcap.
  virtual std::vector<glm::vec2> CreateStartcap(const MidPoint& first,
                                                const MidPoint& second,
                                                int turn_vertices) const {
    // No cap by default.
    return std::vector<glm::vec2>();
  }

  // turn_vertices is the number of vertices used for a turn at this size and
  // does not necessarily impact the number of vertices of the endcap.
  virtual std::vector<glm::vec2> CreateEndcap(const MidPoint& second_to_last,
                                              const MidPoint& last,
                                              int turn_vertices) const {
    // No cap by default.
    return std::vector<glm::vec2>();
  }

  // If this method returns true, all preexisting vertices and midpoints for
  // this line should be removed before adding the new one.
  virtual bool ShouldPruneBeforeNewPoint(
      const std::vector<MidPoint>& existing_points, const MidPoint& new_point) {
    return false;
  }

  // If this method returns true, the given new_point should be ignored.
  virtual bool ShouldDropNewPoint(const MidPoint& previous_point,
                                  const MidPoint& new_point) {
    return false;
  }
};
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_LINE_TIP_MODEL_H_
