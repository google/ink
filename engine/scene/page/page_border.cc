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

#include "ink/engine/scene/page/page_border.h"

#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/nine_patch_info.h"
#include "ink/engine/rendering/gl_managers/nine_patch_rects.h"
#include "ink/engine/rendering/gl_managers/texture.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

PageBorder::PageBorder(std::shared_ptr<GLResourceManager> gl_resources,
                       std::shared_ptr<PageBounds> page_bounds,
                       std::shared_ptr<PageManager> page_manager)
    : page_bounds_(std::move(page_bounds)),
      gl_resources_(std::move(gl_resources)),
      page_manager_(std::move(page_manager)),
      renderer_(gl_resources_) {}

void PageBorder::Draw(const Camera& cam, FrameTimeS draw_time) const {
  if (!page_bounds_->HasBounds() || texture_info_ == nullptr) return;

  Texture* tx;
  if (!gl_resources_->texture_manager->GetTexture(*texture_info_, &tx)) {
    // The texture isn't loaded yet.
    return;
  }

  NinePatchInfo np_info;
  if (!tx->getNinePatchInfo(&np_info)) {
    SLOG(SLOG_ERROR,
         "attempting to draw a 9 patch border, but the texture does not have 9 "
         "patch data!");
    return;
  }

  NinePatchRects np_data(np_info);

  // Find a transform between uv and world space by matching a uv texel to a
  // screen pixel, preserving the screen mapping to uv space.
  glm::vec2 px_to_world_dist =
      cam.ConvertVector(glm::vec2(1), CoordType::kScreen, CoordType::kWorld);

  std::vector<Rect> rects;
  if (page_manager_->MultiPageEnabled() && page_manager_->GetNumPages() > 0) {
    // Find currently visible pages.
    const auto& visible = cam.WorldWindow();
    int left = 0;
    int right = page_manager_->GetNumPages() - 1;
    int n = -1;
    while (left <= right) {
      n = (left + right) / 2;
      const auto& page = page_manager_->GetPageInfo(n);
      if (geometry::Intersects(page.bounds, visible)) break;
      if (page.bounds.Bottom() > visible.Top()) {
        left = n + 1;
      } else {
        right = n - 1;
      }
    }
    // We know that page n is visible, now check its neighbors.
    left = right = n;
    while (left > 0 &&
           geometry::Intersects(page_manager_->GetPageInfo(left - 1).bounds,
                                visible))
      --left;
    while (right < page_manager_->GetNumPages() - 1 &&
           geometry::Intersects(page_manager_->GetPageInfo(right + 1).bounds,
                                visible))
      ++right;
    for (int n = left; n <= right; n++)
      rects.push_back(page_manager_->GetPageInfo(n).bounds);
  } else {
    rects.push_back(page_bounds_->Bounds());
  }

  for (const auto& rect : rects) {
    auto world_rects =
        np_data.CalcPositionRects(scale_ * px_to_world_dist, rect);
    EXPECT(world_rects.size() == 9);

    // We have all the Rect mappings, create a mesh for each region and append
    // it to our main mesh
    Mesh nine_patch_mesh;
    nine_patch_mesh.texture = absl::make_unique<TextureInfo>(*texture_info_);
    Mesh worker_mesh;
    for (int col = 0; col < 3; col++) {
      for (int row = 0; row < 3; row++) {
        if (row == 1 && col == 1) continue;  // don't draw the center!

        const auto& world = world_rects[row * 3 + col];
        const auto& uv = np_data.UvRectAt(row, col);
        MakeRectangleMesh(&worker_mesh, world, glm::vec4{1},
                          world.CalcTransformTo(uv, true));

        nine_patch_mesh.Append(worker_mesh);
      }
    }

    gl_resources_->mesh_vbo_provider->GenVBO(&nine_patch_mesh, GL_DYNAMIC_DRAW);
    renderer_.Draw(cam, draw_time, nine_patch_mesh);
  }
}
}  // namespace ink
