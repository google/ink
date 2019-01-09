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

#include "ink/engine/geometry/algorithms/envelope.h"

namespace ink {
namespace geometry {
namespace {

const glm::vec2& GetPosition(const glm::vec2& point) { return point; }
const glm::vec2& GetPosition(const Vertex& point) { return point.position; }

template <typename T>
Rect GetEnvelopeOfPoints(const std::vector<T>& points) {
  if (points.empty()) {
    return Rect(0, 0, 0, 0);
  }

  Rect envelope =
      Rect::CreateAtPoint(GetPosition(points[0]), /*width=*/0, /*height=*/0);
  for (int i = 1; i < points.size(); ++i) {
    // We don't use Rect::Join() here for efficiency reasons: the cost of
    // copying the returned Rect for each point proved surprisingly high.
    envelope.InplaceJoin(GetPosition(points[i]));
  }
  return envelope;
}

}  // namespace

Rect Envelope(const std::vector<glm::vec2>& points) {
  return GetEnvelopeOfPoints(points);
}

Rect Envelope(const std::vector<Vertex>& points) {
  return GetEnvelopeOfPoints(points);
}

Rect Envelope(const Triangle& triangle) {
  return Rect(triangle[0], triangle[1]).Join(triangle[2]);
}

Rect Envelope(const RotRect& rot_rect) {
  return GetEnvelopeOfPoints(rot_rect.Corners());
}

}  // namespace geometry
}  // namespace ink
