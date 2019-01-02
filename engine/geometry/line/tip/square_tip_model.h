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

#ifndef INK_ENGINE_GEOMETRY_LINE_TIP_SQUARE_TIP_MODEL_H_
#define INK_ENGINE_GEOMETRY_LINE_TIP_SQUARE_TIP_MODEL_H_

#include "ink/engine/geometry/line/tip/tip_model.h"

namespace ink {

// SquareTipModel creates a line by connecting tangents of consecutive circles,
// but its start and end caps are squares (oriented based upon the line's
// direction).
class SquareTipModel : public TipModel {
 public:
  TipType GetTipType() const override { return TipType::Square; }

  void AddTurnPoints(const MidPoint& start, const MidPoint& middle,
                     const MidPoint& end, int turn_vertices,
                     std::function<void(glm::vec2 pt)> add_left,
                     std::function<void(glm::vec2 pt)> add_right) override;
  std::vector<glm::vec2> CreateStartcap(const MidPoint& first,
                                        const MidPoint& second,
                                        int turn_vertices) const override;
  std::vector<glm::vec2> CreateEndcap(const MidPoint& second_to_last,
                                      const MidPoint& last,
                                      int turn_vertices) const override;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_LINE_TIP_SQUARE_TIP_MODEL_H_
