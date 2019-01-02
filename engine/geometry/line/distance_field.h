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

#ifndef INK_ENGINE_GEOMETRY_LINE_DISTANCE_FIELD_H_
#define INK_ENGINE_GEOMETRY_LINE_DISTANCE_FIELD_H_

#include <cstddef>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
namespace ink {

// Computes the distance between a point and nearest point within the last n
// segments of a polyline, (where n = "bufferSize_" - 1).
//
// NOTE: The distance may be to a point in the interior of one of the line
// segments.
//
// E.g. for the polyline connecting the points (0,0), (2,0), and (2, -2),
// DistanceField::distance((1,1)) = 1, as the closest point on the line is
// (1,0).
class DistanceField {
 public:
  DistanceField();
  DistanceField(size_t buffer_size, float min_dist_to_add);

  // Adds pt to the end of the polyline if it is at least "minDistToAdd_" away
  // from the current line end.  If total points exceeds "bufferSize_", oldest
  // point is evicted.
  void AddPt(glm::vec2 pt);

  // The minimum distance between pt and any point in the end of the polyline.
  // Errors if buffer is empty.
  float Distance(glm::vec2 pt);
  // Removes all points from the current buffer.
  void Clear();

 private:
  // Buffer of recent segment endpoints in the polyline.
  std::vector<glm::vec2> linepts_;

  // Maximum number of points to store in the polyline.
  size_t buffer_size_;

  // Points will only be added to the buffer if they are at least
  // "minDistToAdd_" from the current end.
  float min_dist_to_add_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_LINE_DISTANCE_FIELD_H_
