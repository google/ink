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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_BEZIER_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_BEZIER_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"

namespace ink {

class Bezier {
 public:
  Bezier();
  void MoveTo(glm::vec2 v);
  void LineTo(glm::vec2 v);
  void CurveTo(glm::vec2 cp1, glm::vec2 cp2, glm::vec2 to);
  void CurveTo(glm::vec2 cp, glm::vec2 to);
  void Close();

  // Where the pen is.
  const glm::vec2& Tip() { return last_seen_; }

  const std::vector<std::vector<glm::vec2>>& Polyline() const {
    return polyline_;
  }
  std::vector<std::vector<glm::vec2>>& Polyline() { return polyline_; }

  void SetNumEvalPoints(int num_eval_points);

  // The length along the path.
  float PathLength() const;

  void SetTransform(glm::mat4 transform) { transform_ = transform; }
  glm::mat4 Transform() const { return transform_; }

 private:
  int num_eval_points_ = 20;
  std::vector<std::vector<glm::vec2>> polyline_;
  std::vector<glm::vec2>* current_segment_;
  glm::vec2 last_moved_to_{0, 0};  // Last point that was moveTo'd.
  glm::vec2 last_seen_{0, 0};      // Current tip of the curve.
  float current_length_ = 0.0f;
  // From this object's local space to scene world space.
  glm::mat4 transform_{1};

  void AdvanceSegment();
  void PushToCurrentSegment(glm::vec2 pt);
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_BEZIER_H_
