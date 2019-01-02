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

#include "ink/engine/camera_controller/camera_constraints.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/margin.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

// These constants are exposed for testing.
namespace camera_constraints {
const float kMaxPagesShownInWindow = 3.5;
const float kMinPagesShownInWindow = 0.25;
const float kMultiPageMarginCm = .75;
const float kBoundsComparisonSlopFactor = 1.01f;
}  // namespace camera_constraints

CameraConstraints::CameraConstraints(std::shared_ptr<PageManager> page_manager,
                                     std::shared_ptr<PageBounds> bounds,
                                     std::shared_ptr<settings::Flags> flags)
    : fraction_padding_zoomed_out_(0.1),
      page_manager_(page_manager),
      bounds_(bounds),
      never_show_margins_(flags->GetFlag(settings::Flag::StrictNoMargins)) {
  flags->AddListener(this);
}

void CameraConstraints::SetZoomBoundsMarginPx(Margin margin) {
  zoom_bounds_margin_px_ = margin;
}

void CameraConstraints::SetFractionPaddingZoomedOut(
    float fraction_padding_zoomed_out) {
  fraction_padding_zoomed_out_ = fraction_padding_zoomed_out;
}

Rect CameraConstraints::CalculateTargetBounds(Rect world_window,
                                              TargetBoundsPolicy policy,
                                              const Camera& camera) const {
  return CalculateTargetBounds(world_window, policy, camera,
                               world_window.Center());
}

Rect CameraConstraints::CalculateTargetBounds(
    Rect world_window, TargetBoundsPolicy policy, const Camera& camera,
    glm::vec2 scale_center_world) const {
  if (!bounds_->HasBounds()) return world_window;

  if (never_show_margins_) {
    return CalculateTargetBoundsNoMargin(world_window, camera,
                                         scale_center_world);
  } else {
    return CalculateTargetBoundsWithMargin(world_window, policy, camera);
  }
}

Rect CameraConstraints::CalculateTargetBoundsWithMargin(
    Rect world_window, TargetBoundsPolicy policy, const Camera& camera) const {
  const auto slop = (policy == TargetBoundsPolicy::kStrict)
                        ? 1
                        : camera_constraints::kBoundsComparisonSlopFactor;

  const Rect bounds = bounds_->Bounds();

  // These multi-page constraints are instantaneous; they are not enforced with
  // animations, but simply stop camera movement at the desired bounds.
  if (page_manager_->MultiPageEnabled() && page_manager_->GetNumPages() > 1 &&
      bounds.AspectRatio() < 1) {
    world_window = GetVerticalMultipageConstrainedBounds(camera, world_window);
  }

  // If the user is zoomed out, return the document to fit the screen.
  // Note that this takes precedence over any other behavior.
  Rect zoom_out_bounds = ZoomOutBounds(bounds, camera.ScreenDim());
  if (world_window.Width() > zoom_out_bounds.Width() * slop &&
      world_window.Height() > zoom_out_bounds.Height() * slop) {
    SLOG(SLOG_CAMERA, "zoomed out too far, constraining to $0 to $1",
         world_window, zoom_out_bounds);
    return zoom_out_bounds;
  }

  // Don't apply any more rules if we can already see the whole document.
  // We need to scale up by a small amount due to floating point errors.
  if (world_window.Scale(slop).Contains(bounds_->Bounds())) {
    return world_window;
  }

  Rect target_rect = world_window;

  Camera target_camera = camera;
  target_camera.SetWorldWindow(world_window);
  bool zoomed_in_too_far = world_window.Width() < bounds_->MinCameraWidth();
  SLOG(SLOG_CAMERA, "zoomed in too far: $0", zoomed_in_too_far);
  if (zoomed_in_too_far) {
    // Rescale while maintaining the center.
    target_rect =
        target_rect * (bounds_->MinCameraWidth() / target_rect.Width());
    target_rect.SetCenter(world_window.Center());
    target_camera.SetWorldWindow(target_rect);
  }
  glm::vec2 translate_screen_amount =
      TranslateScreenAmount(bounds, target_camera);

  // Due to floating point precision issues, we might accidentally keep trying
  // to animate but fail to move anywhere. If we are less than 1/10th of a
  // pixel from the target, don't consider that as a move.
  if (glm::length(translate_screen_amount) > 0.1f || zoomed_in_too_far) {
    // Get the adjustment back into world coordinates and apply it as our
    // target camera.
    glm::vec2 translate_world_amount = target_camera.ConvertVector(
        translate_screen_amount, CoordType::kScreen, CoordType::kWorld);
    target_rect = target_rect + translate_world_amount;
    SLOG(SLOG_CAMERA, "looking too far off page, animating from $0 to $1",
         world_window, zoom_out_bounds);
    return target_rect;
  }
  return world_window;
}

Rect CameraConstraints::GetVerticalMultipageConstrainedBounds(
    const Camera& camera, Rect candidate) const {
  const Rect& bounds = bounds_->Bounds();

  auto mean_page_height =
      bounds.Height() / static_cast<float>(page_manager_->GetNumPages());

  // Don't zoom out too far.
  const auto max_world_window_height =
      mean_page_height * camera_constraints::kMaxPagesShownInWindow;
  if (candidate.Height() > max_world_window_height) {
    candidate = candidate.Scale(max_world_window_height / candidate.Height());
  }

  // Don't zoom in too far.
  const auto min_world_window_height =
      mean_page_height * camera_constraints::kMinPagesShownInWindow;
  if (candidate.Height() < min_world_window_height) {
    candidate = candidate.Scale(min_world_window_height / candidate.Height());
  }

  const auto vertical_margin_world =
      camera.ConvertDistance(camera_constraints::kMultiPageMarginCm,
                             DistanceType::kCm, DistanceType::kWorld);
  // Don't scroll too high.
  if (candidate.Top() > bounds.Top() + vertical_margin_world) {
    candidate.SetTop(bounds.Top() + vertical_margin_world);
  }
  // Don't scroll too low.
  if (candidate.Bottom() < bounds.Bottom() - vertical_margin_world) {
    candidate.SetBottom(bounds.Bottom() - vertical_margin_world);
  }

  // If you are zoomed out as far as you're allowed to go, what are the left
  // and right boundaries?
  const auto max_world_window_width =
      max_world_window_height / bounds.AspectRatio();
  const auto min_left =
      std::ceil(bounds.Center().x - (max_world_window_width / 2.0));
  const auto max_right =
      std::floor(bounds.Center().x + (max_world_window_width / 2.0));
  // Don't scroll too far left.
  if (candidate.Left() < min_left) {
    candidate.SetLeft(min_left);
  }
  // Don't scroll too far right.
  if (candidate.Right() > max_right) {
    candidate.SetRight(max_right);
  }
  return candidate;
}

Rect CameraConstraints::CalculateTargetBoundsNoMargin(
    Rect world_window, const Camera& camera,
    glm::vec2 scale_center_world) const {
  const Rect bounds = bounds_->Bounds();

  glm::vec2 max_size =
      bounds.InteriorRectWithAspectRatio(camera.WorldWindow().AspectRatio())
          .Dim();

  if (world_window.Width() > max_size.x) {
    world_window = geometry::Transform(
        world_window,
        matrix_utils::ScaleAboutPoint(max_size.x / world_window.Width(),
                                      scale_center_world));
  }
  if (world_window.Height() > max_size.y) {
    world_window = geometry::Transform(
        world_window,
        matrix_utils::ScaleAboutPoint(max_size.y / world_window.Height(),
                                      scale_center_world));
  }

  // Note Rect::Set* move the rect without changing the size.
  if (world_window.Top() > bounds.Top()) {
    world_window.SetTop(bounds.Top());
  }
  if (world_window.Right() > bounds.Right()) {
    world_window.SetRight(bounds.Right());
  }
  if (world_window.Bottom() < bounds.Bottom()) {
    world_window.SetBottom(bounds.Bottom());
  }
  if (world_window.Left() < bounds.Left()) {
    world_window.SetLeft(bounds.Left());
  }
  return world_window;
}

Rect CameraConstraints::ZoomOutBounds(const Rect& page_bounds,
                                      glm::ivec2 screen_dim) const {
  Rect zoom_out_bounds = page_bounds.Scale(1 + fraction_padding_zoomed_out_);
  SLOG(SLOG_CAMERA, "page bounds: $0, zoomOutBounds: $1", page_bounds,
       zoom_out_bounds);
  if (!zoom_bounds_margin_px_.IsEmpty()) {
    Margin fractional_margin = zoom_bounds_margin_px_.AsFractionOf(screen_dim);
    fractional_margin.Clamp0N(.4f);
    Rect screen_sized_rect(0, 0, screen_dim.x, screen_dim.y);
    float target_aspect_ratio =
        fractional_margin.MultiplicativeInset(screen_sized_rect).AspectRatio();
    zoom_out_bounds =
        zoom_out_bounds.ContainingRectWithAspectRatio(target_aspect_ratio);
    zoom_out_bounds = fractional_margin.MultiplicativeOutset(zoom_out_bounds);
    SLOG(SLOG_CAMERA, "zoom bounds after margin correction: $0",
         zoom_out_bounds);
  }

  return zoom_out_bounds;
}

glm::vec2 CameraConstraints::TranslateScreenAmount(
    const Rect& page_bounds, const Camera& target_camera) const {
  // If any of the edges are leaving more than the allowed fraction of the
  // screen empty, reign it in.
  // Find out where the page bounds are in screen pixels.
  Rect bounds_screen =
      geometry::Transform(page_bounds, target_camera.WorldToScreen());
  glm::ivec2 screen_dim = target_camera.ScreenDim();

  glm::vec2 translate_screen_amount(0.f, 0.f);
  {
    // We need to specially handle the case where the normal gray border
    // bounds are necessarily broken (eg if fractionPaddingZoomedOut_ = 0.3
    // and the full width of the page when zoomed out is 0.3 * screen). To
    // handle this, take the width of the bounds on the screen and allow at
    // least enough for the page to be centered on the screen.
    float page_screen_coverage_x = bounds_screen.Width() / screen_dim.x;
    // The fraction of the screen that we should allow to be empty.
    float allowed_empty_x = std::max((1 - page_screen_coverage_x) / 2,
                                     fraction_padding_zoomed_out_);

    float right_adj =
        (screen_dim.x * (1 - allowed_empty_x)) - bounds_screen.Right();
    if (right_adj > 0) {
      translate_screen_amount.x -= right_adj;
    }
    float left_adj = bounds_screen.Left() - (screen_dim.x * allowed_empty_x);
    if (left_adj > 0) {
      translate_screen_amount.x += left_adj;
    }
    ASSERT(right_adj < 0.01 || left_adj < 0.01);
  }
  {
    // Same for Y axis as we did for X above.
    float page_screen_coverage_y = bounds_screen.Height() / screen_dim.y;
    float allowed_empty_y = std::max((1 - page_screen_coverage_y) / 2,
                                     fraction_padding_zoomed_out_);
    float top_adj =
        (screen_dim.y * (1 - allowed_empty_y)) - bounds_screen.Top();
    if (top_adj > 0) {
      translate_screen_amount.y -= top_adj;
    }
    float bottom_adj =
        bounds_screen.Bottom() - (screen_dim.y * allowed_empty_y);
    if (bottom_adj > 0) {
      translate_screen_amount.y += bottom_adj;
    }
    ASSERT(top_adj < 0.01 || bottom_adj < 0.01);
  }
  return translate_screen_amount;
}

}  // namespace ink
