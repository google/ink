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

#ifndef INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONSTRAINTS_H_
#define INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONSTRAINTS_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/margin.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"

namespace ink {

// For zooming in a multipage document.
namespace camera_constraints {
extern const float kMaxPagesShownInWindow;
extern const float kMinPagesShownInWindow;
extern const float kMultiPageMarginCm;

// Permit measurements of the current bounds and the desired bounds to differ
// by this much to avoid oscillating between zooms that differ only by some
// floating point epsilon.
extern const float kBoundsComparisonSlopFactor;
}  // namespace camera_constraints

// CameraConstraints holds camera/page margin information and provides
// methods for computing how those constraints should be applied to potential
// changes to the Camera.
class CameraConstraints : public settings::FlagListener {
 public:
  enum class TargetBoundsPolicy { kAllowSlop, kStrict };

  using SharedDeps =
      service::Dependencies<PageManager, PageBounds, settings::Flags>;

  CameraConstraints(std::shared_ptr<PageManager> page_manager,
                    std::shared_ptr<PageBounds> bounds,
                    std::shared_ptr<settings::Flags> flags);

  ~CameraConstraints() override {}

  // CameraConstraints is neither copyable nor movable.
  CameraConstraints(const CameraConstraints&) = delete;
  CameraConstraints& operator=(const CameraConstraints&) = delete;

  void SetZoomBoundsMarginPx(Margin margin);
  void SetFractionPaddingZoomedOut(float fraction_padding_zoomed_out);

  // Given the world window that the Camera thinks it wants to look at, uses
  // heuristics to calculate the window that it should actually be looking at.
  Rect CalculateTargetBounds(Rect world_window, TargetBoundsPolicy policy,
                             const Camera& camera) const;

  // Given the world window that the Camera thinks it wants to look at, uses
  // heuristics to calculate the window that it should actually be looking at.
  // If the window needs to be scaled down to fit within the constraints, scale
  // about scale_center_world.
  Rect CalculateTargetBounds(Rect world_window, TargetBoundsPolicy policy,
                             const Camera& camera,
                             glm::vec2 scale_center_world) const;

  void OnFlagChanged(settings::Flag which, bool new_value) override {
    if (which == settings::Flag::StrictNoMargins) {
      never_show_margins_ = new_value;
    }
  }

 private:
  // Returns the world bounds of the most-zoomed-out camera world view allowed
  // given the margin and zoom fraction constraints.
  Rect ZoomOutBounds(const Rect& page_bounds, glm::ivec2 screen_dim) const;
  // Return the distance that the given camera should be translated such that
  // the given page bounds adhere to the margin/coverage constraints.
  glm::vec2 TranslateScreenAmount(const Rect& page_bounds,
                                  const Camera& target_camera) const;
  Rect CalculateTargetBoundsWithMargin(Rect world_window,
                                       TargetBoundsPolicy policy,
                                       const Camera& camera) const;
  // Same as CalculateTargetBounds() but enforces a "no margins" invariant.
  Rect CalculateTargetBoundsNoMargin(Rect world_window, const Camera& camera,
                                     glm::vec2 scale_center_world) const;

  // Calculates target bounds for a multipage document with vertical layout.
  Rect GetVerticalMultipageConstrainedBounds(const Camera& camera,
                                             Rect candidate) const;

  // When very zoomed out and moving to fit the page (plus padding) on the
  // screen, exclude this margin from the target region on the screen.
  Margin zoom_bounds_margin_px_;
  // The fraction of the screen that is allowed to be empty when we are zoomed
  // out beyond the document bounds
  float fraction_padding_zoomed_out_;

  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<PageBounds> bounds_;
  bool never_show_margins_;
};

}  // namespace ink

#endif  // INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONSTRAINTS_H_
