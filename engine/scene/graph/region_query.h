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

#ifndef INK_ENGINE_SCENE_GRAPH_REGION_QUERY_H_
#define INK_ENGINE_SCENE_GRAPH_REGION_QUERY_H_

#include <functional>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_metadata.h"

namespace ink {

// An builder for a region query into the SceneGraph.
class RegionQuery {
 public:
  using ElementFilterFn = std::function<bool(const ElementId&)>;

  // Get the standard minimum selection size based on the input type.
  static float MinSelectionSizeCm(input::InputType input_type);
  static float MinSegmentSelectionSizeCm(input::InputType input_type);

  // Creates a query for the given axis-aligned rectangle. If any side of the
  // rectangle is less than min_size_world, then the query will be for the
  // smallest axis-aligned rectangle containing both region, and a square with
  // side length min_size_world centered on region.
  static RegionQuery MakeRectangleQuery(Rect region, float min_size_world);

  // Creates a square query with side length min_size_world, centered on point.
  static RegionQuery MakePointQuery(glm::vec2 point, float min_size_world);

  // Creates a rectangular query, aligned with seg and centered on seg's
  // midpoint, with dimensions min_size_width + seg.Length() by min_size_world.
  static RegionQuery MakeSegmentQuery(Segment seg, float min_size_world);

  // Creates a query from the camera's world window.
  static RegionQuery MakeCameraQuery(const Camera& camera);

  RegionQuery() : RegionQuery(Rect(0, 0, 0, 0), {POLY}) {}
  explicit RegionQuery(Rect r) : RegionQuery(r, {POLY}) {}
  explicit RegionQuery(Rect r,
                       const std::unordered_set<ElementType>& type_filter)
      : region_(r), type_filter_{type_filter} {}

  RegionQuery& SetCustomFilter(ElementFilterFn filter) {
    custom_filter_fn_ = filter;
    return *this;
  }

  // Filters out elements which are not descendants of this group_id.
  // Defaults to kInvalidElementId which will search from the root of
  // the scene_graph.
  RegionQuery& SetGroupFilter(GroupId group_id) {
    group_filter_ = group_id;
    return *this;
  }

  // The specified region is typically a world coordinates region: this
  // transform is an additional matrix that could be applied to the region to
  // get a world coordinates query (this enables non-world axis aligned
  // rectangle queries).
  RegionQuery& SetTransform(glm::mat4 transform) {
    transform_ = transform;
    return *this;
  }

  void SetAllowedTypes(const std::unordered_set<ElementType>& types) {
    type_filter_ = types;
  }

  const Rect& Region() const { return region_; }
  const std::unordered_set<ElementType>& TypeFilter() const {
    return type_filter_;
  }
  ElementFilterFn CustomFilter() const { return custom_filter_fn_; }
  GroupId GroupFilter() const { return group_filter_; }
  const glm::mat4& Transform() const { return transform_; }

  // A mesh that can be used for debugging that covers region in world
  // coordinates that the query is for.
  Mesh MakeDebugMesh() const;

 private:
  Rect region_;
  std::unordered_set<ElementType> type_filter_;  // empty means all.
  ElementFilterFn custom_filter_fn_;
  GroupId group_filter_ = kInvalidElementId;
  glm::mat4 transform_{1};
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_REGION_QUERY_H_
