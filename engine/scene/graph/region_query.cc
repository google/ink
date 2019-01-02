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

#include "ink/engine/scene/graph/region_query.h"

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/vector_utils.h"

namespace ink {
namespace {
using glm::mat4;
using glm::vec2;
using glm::vec3;
using input::InputType;

constexpr float kTouchMinSelectionSizeCm = .6f;
constexpr float kMouseMinSelectionSizeCm = .3f;
constexpr float kTouchSegmentMinSelectionSizeCm = .3f;
constexpr float kMouseSegmentMinSelectionSizeCm = .2f;
}  // namespace

float RegionQuery::MinSelectionSizeCm(InputType input_type) {
  return input_type == InputType::Touch ? kTouchMinSelectionSizeCm
                                        : kMouseMinSelectionSizeCm;
}

float RegionQuery::MinSegmentSelectionSizeCm(InputType input_type) {
  return input_type == InputType::Touch ? kTouchSegmentMinSelectionSizeCm
                                        : kMouseSegmentMinSelectionSizeCm;
}

RegionQuery RegionQuery::MakeRectangleQuery(Rect region, float min_size_world) {
  return RegionQuery(region.ContainingRectWithMinDimensions(
      glm::vec2(min_size_world, min_size_world)));
}

RegionQuery RegionQuery::MakePointQuery(vec2 point, float min_size_world) {
  return RegionQuery(
      Rect::CreateAtPoint(point, min_size_world, min_size_world));
}

RegionQuery RegionQuery::MakeSegmentQuery(Segment seg, float min_size_world) {
  if (seg.Length() == 0) return MakePointQuery(seg.from, min_size_world);
  return RegionQuery(Rect::CreateAtPoint(vec2(0, 0),
                                         min_size_world + seg.Length(),
                                         min_size_world))
      .SetTransform(glm::rotate(
          glm::translate(mat4{1}, vec3(.5f * (seg.from + seg.to), 0)),
          VectorAngle(seg.to - seg.from), vec3(0, 0, 1)));
}

RegionQuery RegionQuery::MakeCameraQuery(const Camera& camera) {
  return RegionQuery(camera.WorldWindow());
}

Mesh RegionQuery::MakeDebugMesh() const {
  Mesh m;
  MakeRectangleMesh(&m, region_, glm::vec4(1, 0, 0, .5));
  m.object_matrix *= transform_;
  return m;
}

}  // namespace ink
