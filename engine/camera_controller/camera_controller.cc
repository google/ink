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

#include "ink/engine/camera_controller/camera_controller.h"

#include <algorithm>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// The animation is currently a 200ms linear animation.
static constexpr DurationS kAnimationDurationSecs(0.2);

CameraController::CameraController(
    std::shared_ptr<Camera> camera, std::shared_ptr<PageBounds> bounds,
    std::shared_ptr<FrameState> frame_state,
    std::shared_ptr<input::InputDispatch> input_dispatch,
    std::shared_ptr<AnimationController> anim_controller,
    std::shared_ptr<CameraConstraints> constraints,
    std::shared_ptr<settings::Flags> flags)
    : input::InputHandler(input::Priority::ObserveOnly),
      camera_(std::move(camera)),
      bounds_(bounds),
      frame_state_(frame_state),
      rect_anim_(anim_controller, [this]() { return camera_->WorldWindow(); },
                 [this](const Rect& r) { return camera_->SetWorldWindow(r); }),
      input_processing_enabled_(true),
      constraints_(constraints),
      constraints_active_(
          !flags->GetFlag(settings::Flag::EnableHostCameraControl)) {
  RegisterForInput(input_dispatch);
  flags->AddListener(this);
}

DurationS CameraController::GetAnimationDurationSecs() {
  return kAnimationDurationSecs;
}

S_WARN_UNUSED_RESULT bool CameraController::GetCurrentAnimationTarget(
    Rect* out) {
  // Only fill in the animation target if we are currently in the middle of
  // animating.
  if (!rect_anim_.IsAnimating()) return false;
  return rect_anim_.GetTarget(out);
}

input::CaptureResult CameraController::OnInput(const input::InputData& data,
                                               const Camera& camera) {
  if (constraints_active_ && input_processing_enabled_) {
    if (bounds_->HasBounds() && data.n_down == 0) {
      MaybeStartAnimating();
    } else {
      StopAnimation();
    }
  }
  return input::CapResObserve;
}

// Calculate if we should animate the camera, and call StartAnimation() with the
// from/to camera positions if so.
void CameraController::MaybeStartAnimating() {
  // Do nothing if we are already animating.
  if (rect_anim_.IsAnimating()) return;

  // Do nothing if we're not constraining.
  if (!constraints_active_) return;

  Rect current = camera_->WorldWindow();
  Rect target = constraints_->CalculateTargetBounds(
      camera_->WorldWindow(), CameraConstraints::TargetBoundsPolicy::kAllowSlop,
      *camera_);
  if (current.Scale(camera_constraints::kBoundsComparisonSlopFactor)
          .Contains(target) &&
      target.Scale(camera_constraints::kBoundsComparisonSlopFactor)
          .Contains(current)) {
    return;
  }
  StartAnimation(target);
}

void CameraController::StartAnimation(Rect to_rect) {
  rect_anim_.AnimateTo(to_rect, kAnimationDurationSecs);
}

void CameraController::StopAnimation() { rect_anim_.StopAnimation(); }

void CameraController::SetInputProcessingEnabled(bool enabled) {
  input_processing_enabled_ = enabled;
  if (enabled) {
    MaybeStartAnimating();
    frame_state_->RequestFrame();
  }
}

void CameraController::LookAt(const Rect& world_window) {
  // Stop the current animations, if any.
  rect_anim_.StopAnimation();

  // Jump the camera to the target.
  if (constraints_active_) {
    camera_->SetWorldWindow(constraints_->CalculateTargetBounds(
        world_window, CameraConstraints::TargetBoundsPolicy::kStrict,
        *camera_));
  } else {
    camera_->SetWorldWindow(world_window);
  }
  frame_state_->RequestFrame();
}

void CameraController::PageDown() {
  auto ww = camera_->WorldWindow();
  LookAt(ww - glm::vec2(0, ww.Height()));
}

void CameraController::PageUp() {
  auto ww = camera_->WorldWindow();
  LookAt(ww + glm::vec2(0, ww.Height()));
}

static constexpr float kScrollWorldWindowRatio = .2;

void CameraController::ScrollDown() {
  auto ww = camera_->WorldWindow();
  LookAt(ww - glm::vec2(0, ww.Height() * kScrollWorldWindowRatio));
}

void CameraController::ScrollUp() {
  auto ww = camera_->WorldWindow();
  LookAt(ww + glm::vec2(0, ww.Height() * kScrollWorldWindowRatio));
}

void CameraController::OnFlagChanged(settings::Flag which, bool new_value) {
  if (which == settings::Flag::EnableHostCameraControl) {
    constraints_active_ = !new_value;
  }
  if (constraints_active_) {
    LookAt(camera_->WorldWindow());
  } else {
    rect_anim_.StopAnimation();
  }
}

}  // namespace ink
