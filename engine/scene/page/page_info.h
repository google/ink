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

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_INFO_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_INFO_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

// PageSpec will be used by Ink to create new pages. Note that we only
// take in dimensions of the page. It is the callers responsibility
// to ensure that elements associated with this page are positioned relative
// to page bounds, which are (0,0) = page bottom left and
// (width,height) = page top right.
struct PageSpec {
  // The Group UUID associated with the page. If empty will create a new UUID
  // through the SceneGraph when PageManager::AddPage is called. If not empty,
  // will be used to lookup the valid group id.
  string uuid;
  // The original page dimensions.
  // dimensions[0] = width, dimensions[1] = height.
  glm::vec2 dimensions{0, 0};

  // Will be filled in by PageManager::AddPage to be used internally, it should
  // never by set directly otherwise.
  GroupId group_id = kInvalidElementId;

  inline PageSpec& operator=(const PageSpec& other) {
    uuid = other.uuid;
    dimensions = other.dimensions;
    group_id = other.group_id;
    return *this;
  }
};

// PageInfo will be used internally in ink.
// The transform should be used to move elements that are children of this page
// into the proper world coordinates (their stored mesh transform will actually
// define the object to page coordinates). The bounds represent the original
// dimensions (with the left,bottom at 0,0) transformed into world coordinate
// space.
struct PageInfo {
  // The LayoutStrategy should fill in the transform and bounds information.
  // The page->world transform.
  glm::mat4 transform{1};
  // The transformed bounds for this page.
  Rect bounds;

  // The original page spec. This is filled in by the PageManager.
  PageSpec page_spec;

  // The index of the page. This is filled in by GenerateLayout.
  int page_index;

  // Ensure that the transform and bounds are consistent.
  bool IsConsistent() {
    return geometry::Transform(Rect(glm::vec2(0, 0), page_spec.dimensions),
                               transform) == bounds;
  }

  inline PageInfo& operator=(const PageInfo& other) {
    transform = other.transform;
    bounds = other.bounds;
    page_spec = other.page_spec;
    page_index = other.page_index;
    return *this;
  }
};

inline std::ostream& operator<<(std::ostream& oss, const PageInfo& p) {
  oss << p.page_spec.group_id.ToStringExtended() << " " << p.bounds;
  return oss;
}

}  // namespace ink

#endif  // INK_ENGINE_SCENE_PAGE_PAGE_INFO_H_
