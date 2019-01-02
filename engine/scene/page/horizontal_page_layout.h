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

#ifndef INK_ENGINE_SCENE_PAGE_HORIZONTAL_PAGE_LAYOUT_H_
#define INK_ENGINE_SCENE_PAGE_HORIZONTAL_PAGE_LAYOUT_H_

#include "ink/engine/scene/page/page_layout_strategy.h"

namespace ink {

// A Horizontal Page layout tries to center-align all pages vertically
// and will separate each page by spacing_cm centimeters. The first page
// will go from 0, 0 (bottom left) to width, height (upper right).
// The second page will start at width + spacing, 0 (bottom left).
class HorizontalPageLayout : public LinearLayoutStrategy {
 public:
  // --------------------------------------------------------------------
  // PageLayoutStrategy implementation. See page_layout_strategy.h for
  // comments.
  // --------------------------------------------------------------------
  // Set and regenerate the layout according to a new set of page defs.
  PageInfoList GenerateLayout(
      const Camera& cam, const std::vector<PageSpec>& page_defs) const override;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_PAGE_HORIZONTAL_PAGE_LAYOUT_H_
