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

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_STRATEGY_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_STRATEGY_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/scene/page/page_info.h"

namespace ink {

// Defines the interface for providing a page layout. Subclasses should
// implement GenerateLayout, which takes in a set of page dimensions and a
// camera (useful to computing spacing parameters in screen space) and
// returns a list of PageInfo objects, which define the page to world transform
// of the lower-left point on a given page and the world-space bounds of the
// page. See vertical_page_layout.h for an example layout.
// This strategy is used by PageManager to send the layout transforms and bounds
// to the SceneGraph.
class PageLayoutStrategy {
 public:
  using PageInfoList = std::vector<PageInfo>;

  virtual ~PageLayoutStrategy() = default;

  // Set and regenerate the layout according to a new set of page defs.
  virtual PageInfoList GenerateLayout(
      const Camera& cam, const std::vector<PageSpec>& page_defs) const = 0;
};

class LinearLayoutStrategy : public PageLayoutStrategy {
 public:
  // Sets the inter-page spacing. You must call PageManager's
  // GenerateLayout() for this to affect layout.
  void SetSpacingWorld(float spacing_world) { spacing_world_ = spacing_world; }

  // Returns the inter-page spacing, in cm.
  float GetSpacingWorld() const { return spacing_world_; }

 private:
  float spacing_world_{0};
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_STRATEGY_H_
