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

#include "ink/engine/realtime/pan_handler.h"

#include "ink/engine/settings/flags.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

namespace {
// Parameters for fling behavior.

// A sentinel meaning "we don't have a valid start time."
constexpr InputTimeS kInvalidTime(0);

// How long does it take for fling to decay?
constexpr DurationS kFlingDuration(1.6);
}  // namespace

DefaultPanHandler::DefaultPanHandler(
    std::shared_ptr<input::InputDispatch> input, std::shared_ptr<Camera> camera,
    std::shared_ptr<settings::Flags> flags,
    std::shared_ptr<CameraController> camera_controller,
    std::shared_ptr<PageBounds> page_bounds,
    std::shared_ptr<CameraConstraints> constraints,
    std::shared_ptr<AnimationController> animation_controller,
    std::shared_ptr<WallClockInterface> wall_clock,
    std::shared_ptr<PageManager> page_manager,
    std::shared_ptr<IEngineListener> engine_listener)
    : input::InputHandler(input::Priority::Pan),
      allow_one_finger_pan_(false),
      camera_(std::move(camera)),
      camera_controller_(std::move(camera_controller)),
      flags_(std::move(flags)),
      page_bounds_(std::move(page_bounds)),
      rect_drag_modifier_(camera_, page_bounds_),
      constraints_(std::move(constraints)),
      animation_controller_(std::move(animation_controller)),
      wall_clock_(std::move(wall_clock)),
      page_manager_(std::move(page_manager)),
      engine_listener_(std::move(engine_listener)),
      strict_camera_constraints_(
          flags_->GetFlag(settings::Flag::StrictNoMargins)),
      drag_anim_(
          animation_controller_, [this]() { return drag_per_second_; },
          [this](glm::vec2 drag) { SetDragPerSecond(drag); }) {
  flags_->AddListener(this);
  RegisterForInput(input);
}

void DefaultPanHandler::MaybeFling() {
  if (flags_->GetFlag(settings::Flag::EnableFling) &&
      glm::length(drag_per_second_) > 0) {
    last_anim_frame_time_ = wall_clock_->CurrentTime();
    // If our current viewport violates resting viewport constraints, don't
    // bother animating.
    const auto current_view = camera_->WorldWindow();
    auto target_bounds = constraints_->CalculateTargetBounds(
        current_view, CameraConstraints::TargetBoundsPolicy::kStrict, *camera_);
    if (current_view == target_bounds) {
      drag_anim_.AnimateTo(glm::vec2{0}, kFlingDuration,
                           absl::make_unique<CubicBezierAnimationCurve>(
                               glm::vec2(0, 0), glm::vec2(.2, 1)),
                           DefaultInterpolator<glm::vec2>());
      MaybeNotifyCameraMovementStateChange(true);
    }
  } else {
    MaybeNotifyCameraMovementStateChange(false);
  }
}

input::CaptureResult DefaultPanHandler::OnInput(const input::InputData& data,
                                                const Camera& camera) {
  if (flags_->GetFlag(settings::Flag::EnableHostCameraControl) ||
      !flags_->GetFlag(settings::Flag::EnablePanZoom)) {
    is_dragging_ = false;
    drag_anim_.StopAnimation();
    drag_reco_.Reset();
    return input::CapResRefuse;
  }

  if (data.Get(input::Flag::Cancel)) {
    is_dragging_ = false;
    drag_anim_.StopAnimation();
    drag_reco_.Reset();
    camera_->SetWorldWindow(saved_world_wnd_);
    MaybeNotifyCameraMovementStateChange(false);
    return input::CapResRefuse;
  }

  // On TUp, begin a "fling" animation.
  if (data.Get(input::Flag::TUp) && data.n_down == 0) {
    is_dragging_ = false;
    MaybeFling();
    return input::CapResRefuse;
  }

  auto res = input::CapResObserve;

  // Take over input if the view is invalid
  if (!camera_->WithinPrecisionBounds()) {
    res = input::CapResCapture;
  }

  if (data.Get(input::Flag::TDown) && data.n_down == 1) {
    drag_anim_.StopAnimation();
    drag_per_second_ = {0, 0};

    // Save a copy of the camera viewport on down to support cancellation
    saved_world_wnd_ = camera_->WorldWindow();

    // Update drag settings for right-click or two-finger touch.
    drag_reco_.Reset();
    bool should_pan = ShouldOneTouchPan(data);
    drag_reco_.SetAllowOneFingerPan(should_pan);
    if (should_pan) {
      is_dragging_ = true;
      drag_anim_.StopAnimation();
    }
  }

  input::DragData drag;
  bool has_drag;

  bool strict_constraints = strict_camera_constraints_;

  // Move camera_ to track input
  if (data.Get(input::Flag::Wheel)) {
    drag_anim_.StopAnimation();
    drag = DragDataFromWheelInput(data);
    has_drag = true;
    // Scroll wheel always enforces strict constraints, as we don't want to
    // rubberband back after a wheel event.
    strict_constraints = true;
  } else {
    res = drag_reco_.OnInput(data, *camera_);
    has_drag = drag_reco_.GetDrag(&drag);
    is_dragging_ |= has_drag;
  }

  if (has_drag) {
    rect_drag_modifier_.ConstrainDragEvent(&drag);
    camera_->Scale(1.0f / drag.scale, drag.world_scale_center);
    camera_->Translate(-drag.world_drag);

    if (page_manager_->MultiPageEnabled() &&
        page_manager_->GetFullBounds().AspectRatio() < 1) {
      // Don't allow rubber-band pan with PDFs.
      camera_controller_->LookAt(constraints_->CalculateTargetBounds(
          camera.WorldWindow(), CameraConstraints::TargetBoundsPolicy::kStrict,
          *camera_));
    } else if (strict_constraints) {
      // If camera constraints wouldn't allow the desired world window,
      // accept whatever window the constraints allow.
      Rect target = constraints_->CalculateTargetBounds(
          camera.WorldWindow(), CameraConstraints::TargetBoundsPolicy::kStrict,
          *camera_, drag.world_scale_center);
      if (target != camera_->WorldWindow()) {
        camera_->SetWorldWindow(target);
      }
    }

    NoteDragEvent(data, -drag.world_drag);
  }
  MaybeNotifyCameraMovementStateChange(has_drag);

  return res;
}

void DefaultPanHandler::NoteDragEvent(const input::InputData& data,
                                      glm::vec2 world_drag) {
  drag_anim_.StopAnimation();
  if (data.last_time != kInvalidTime) {
    // Input can feature 0 inter-event time intervals, which would give infinite
    // drag speed per second. We ignore events with zero deltas.
    auto interval = data.time - data.last_time;
    if (interval > 0) {
      drag_per_second_ = world_drag / static_cast<float>(interval);
    }
  }
}

absl::optional<input::Cursor> DefaultPanHandler::CurrentCursor(
    const Camera& camera) const {
  if (is_dragging_) {
    return input::Cursor(input::CursorType::MOVE);
  }
  return absl::nullopt;
}

void DefaultPanHandler::SetAllowOneFingerPan(bool enable_one_finger_pan) {
  SLOG(SLOG_TOOLS, "enabling one finger pan: $0", enable_one_finger_pan);
  allow_one_finger_pan_ = enable_one_finger_pan;
}

// This function is called by the fling animation, as drag per second goes to 0.
void DefaultPanHandler::SetDragPerSecond(glm::vec2 drag_per_second) {
  drag_per_second_ = drag_per_second;
  auto now = wall_clock_->CurrentTime();
  auto interval = now - last_anim_frame_time_;
  last_anim_frame_time_ = now;

  Camera new_camera = *camera_;
  auto instantaneous_drag = drag_per_second * static_cast<float>(interval);
  new_camera.Translate(instantaneous_drag);

  // LookAt() uses strict constraints, so we can't go off the rails.
  camera_controller_->LookAt(new_camera.WorldWindow());
  MaybeNotifyCameraMovementStateChange(glm::length(drag_per_second_) > 0);
}

input::DragData DefaultPanHandler::DragDataFromWheelInput(
    const input::InputData& input_data) {
  input::DragData drag;
  const bool& is_ctrl = input_data.Get(input::Flag::Control);
  // If mouse_wheel_scrolls_, then we're only zooming if ctrl is pressed.
  // Conversely, if mouse wheel zooms, only zoom when ctrl is NOT pressed.
  const bool& zooming =
      ((mousewheel_policy_ == MousewheelPolicy::SCROLLS) == is_ctrl);

  const auto dx = input_data.wheel_delta_x;
  const auto dy = input_data.wheel_delta_y;
  Camera destination = *camera_;

  if (zooming) {
    float scale_magnitude =
        std::min(kMouseWheelZoomFactor * std::abs(dy), 0.99);
    drag.scale = dy <= 0 ? 1 - scale_magnitude : 1 / (1 - scale_magnitude);
    drag.world_scale_center = input_data.world_pos;
  } else {
    // Scrolling.
    // Heavily weight the dominant direction.
    auto scroll_x = dx;
    if (std::abs(dy) > std::abs(dx)) {
      scroll_x *= kTrackpadSquashFactor;
    }
    auto scroll_y = dy;
    if (std::abs(dx) > std::abs(dy)) {
      scroll_y *= kTrackpadSquashFactor;
    }
    drag.world_drag = destination.ConvertVector(
        glm::vec2(scroll_x, -scroll_y), CoordType::kScreen, CoordType::kWorld);
  }
  return drag;
}

bool DefaultPanHandler::ShouldOneTouchPan(const input::InputData& data) {
  if (allow_one_finger_pan_) {
    return true;
  }
  if (data.Get(input::Flag::Right)) {
    return true;
  }
  if (data.type == input::InputType::Touch &&
      flags_->GetFlag(settings::Flag::EnablePenMode)) {
    return true;
  }
  if (page_bounds_->HasBounds() &&
      !page_bounds_->Bounds().Contains(data.world_pos)) {
    return true;
  }
  if (page_manager_->MultiPageEnabled() &&
      page_manager_->GetPageGroupForRect(
          Rect::CreateAtPoint(data.world_pos, 5, 5)) == kInvalidElementId) {
    return true;
  }
  return false;
}

void DefaultPanHandler::MaybeNotifyCameraMovementStateChange(bool is_moving) {
  if (is_moving != camera_is_moving_) {
    camera_is_moving_ = is_moving;
    engine_listener_->CameraMovementStateChanged(camera_is_moving_);
  }
}

}  // namespace ink
