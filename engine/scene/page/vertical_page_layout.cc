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

#include "ink/engine/scene/page/vertical_page_layout.h"

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

PageLayoutStrategy::PageInfoList VerticalPageLayout::GenerateLayout(
    const Camera& cam, const std::vector<PageSpec>& page_defs) const {
  // For each page, the precomputed transform info.
  std::vector<PageInfo> pages;
  if (page_defs.empty()) {
    return pages;
  }
  float max_width = 0;
  // The size of each page, in original page space.
  std::vector<Rect> bounds;
  bounds.resize(page_defs.size());
  pages.resize(page_defs.size());
  max_width = 0;
  for (int i = 0; i < page_defs.size(); ++i) {
    bounds[i] = Rect(glm::vec2(0, 0), page_defs[i].dimensions);
    max_width = max(max_width, bounds[i].Width());
  }

  glm::mat4 identity{1};
  // Note that the page bounds are 0,0 -> width,height.
  // We want to transform the top of the first page to 0,0, so
  // the initial transform moves to a -height.
  // All other pages are offset from that.
  float next_y = 0;
  for (int i = 0; i < bounds.size(); ++i) {
    float offset_x = (max_width - bounds[i].Width()) / 2;
    pages[i].transform = glm::translate(
        identity, glm::vec3(offset_x, next_y - bounds[i].Height(), 0));
    pages[i].bounds = geometry::Transform(bounds[i], pages[i].transform);
    next_y = next_y - bounds[i].Height() - GetSpacingWorld();
  }
  return pages;
}

}  // namespace ink
