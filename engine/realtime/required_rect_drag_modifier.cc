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

#include "ink/engine/realtime/required_rect_drag_modifier.h"

#include <cmath>

#include "ink/engine/geometry/algorithms/transform.h"

namespace ink {

RequiredRectDragModifier::RequiredRectDragModifier(
    std::shared_ptr<Camera> camera, std::shared_ptr<PageBounds> page_bounds)
    : enabled_(false), camera_(camera), page_bounds_(page_bounds) {}

void RequiredRectDragModifier::StartEnforcement(const Rect& Rect,
                                                float minimum_scale) {
  required_rect_ = Rect;
  minimum_scale_ = minimum_scale;
  enabled_ = true;
}

void RequiredRectDragModifier::StopEnforcement() { enabled_ = false; }

void RequiredRectDragModifier::ConstrainDragEvent(input::DragData* drag) {
  if (!enabled_) {
    return;
  }

  auto page_bounds_world = page_bounds_->Bounds();
  Camera candidate = *camera_;

  // Ensure the drag's scale would at least leave the camera's scale factor at
  // minimum_scale_.
  drag->scale = std::max(drag->scale, minimum_scale_ * camera_->ScaleFactor());
  candidate.Scale(1.0f / drag->scale, drag->world_scale_center);
  if (!PageBoundsContainsRequired(candidate)) {
    // Required Rect goes outside the page bounds in at least one direction.

    auto world_req =
        geometry::Transform(required_rect_, candidate.ScreenToWorld());

    const bool top = page_bounds_world.Top() < world_req.Top();
    const bool bottom = page_bounds_world.Bottom() > world_req.Bottom();
    const bool left = page_bounds_world.Left() > world_req.Left();
    const bool right = page_bounds_world.Right() < world_req.Right();

    // Note that both top and bottom may be true, but it only matters that
    // we modify the scale center's Y coordinate once (and the drag->scale param
    // would then need to be constrained below).
    if (top) {
      // Required box goes over the top.
      drag->world_scale_center.y = page_bounds_world.Top();
    } else if (bottom) {
      drag->world_scale_center.y = page_bounds_world.Bottom();
    }
    if (left) {
      drag->world_scale_center.x = page_bounds_world.Left();
    } else if (right) {
      drag->world_scale_center.x = page_bounds_world.Right();
    }

    // If we've modified the world_scale_center, potentially change the
    // drag's scale to ensure we aren't zooming out so much that the required
    // box is no longer contained.
    if (top || bottom) {
      // Portion of the screen height occupied by the required box.
      float minRatio = required_rect_.Height() / camera_->ScreenDim().y;
      // Portion of the world window's height occupied by the page
      float currentRatio =
          page_bounds_world.Height() / camera_->WorldWindow().Height();
      // Scale value must be at least minRatio / currentRatio
      drag->scale = std::fmax(drag->scale, minRatio / currentRatio);
    }

    if (left || right) {
      float minRatio = required_rect_.Width() / camera_->ScreenDim().x;
      float currentRatio =
          page_bounds_world.Width() / camera_->WorldWindow().Width();
      drag->scale = std::fmax(drag->scale, minRatio / currentRatio);
    }

    // Reset the candidate and scale again.
    candidate.SetWorldWindow(camera_->WorldWindow());
    candidate.Scale(1.0f / drag->scale, drag->world_scale_center);
  }

  // At this point, the scale magnitude and position have been constrained such
  // that the required Rect can fit within the page. Thus panning violations
  // can all be addressed simply by modifying panning.
  candidate.Translate(-drag->world_drag);
  if (!PageBoundsContainsRequired(candidate)) {
    // Panned out of bounds.
    auto world_req =
        geometry::Transform(required_rect_, candidate.ScreenToWorld());

    // How far out the crop bounds go outside the page in each direction
    float top, bottom, left, right;
    top = world_req.Top() - page_bounds_world.Top();
    bottom = page_bounds_world.Bottom() - world_req.Bottom();
    left = page_bounds_world.Left() - world_req.Left();
    right = world_req.Right() - page_bounds_world.Right();

    // Top and bottom should not BOTH be > 0, as the scaling checks should have
    // ensured that a fit is possible and only translation is needed.
    if (top > 0) {
      drag->world_drag.y += top;
    } else if (bottom > 0) {
      drag->world_drag.y -= bottom;
    }

    // Same for left/right
    if (left > 0) {
      drag->world_drag.x -= left;
    } else if (right > 0) {
      drag->world_drag.x += right;
    }
  }
}

bool RequiredRectDragModifier::PageBoundsContainsRequired(
    const Camera& camera) {
  auto world_req = geometry::Transform(required_rect_, camera.ScreenToWorld());
  return !page_bounds_->HasBounds() ||
         page_bounds_->Bounds().Contains(world_req);
}

}  // namespace ink
