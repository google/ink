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

#ifndef INK_ENGINE_REALTIME_PAN_HANDLER_H_
#define INK_ENGINE_REALTIME_PAN_HANDLER_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/input/drag_reco.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/realtime/required_rect_drag_modifier.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/animation/animated_fn.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

enum class MousewheelPolicy {
  ZOOMS,   // Mousewheel zooms, ctrl-mousewheel scrolls.
  SCROLLS  // Mousewheel pans, ctrl-mousewheel zooms.
};

class PanHandler {
 public:
  virtual ~PanHandler() {}
  virtual void SetAllowOneFingerPan(bool enable_one_finger_pan) {}
  virtual void EnforceMovementConstraint(const Rect& required,
                                         float min_scale) {}
  virtual void StopMovementConstraintEnforcement() {}
  virtual void SetMousewheelPolicy(MousewheelPolicy mousewheel_policy) {}
};

// DefaultPanHandler listens for input data and modifies the Camera to
// translate/scale based upon user gestures.
class DefaultPanHandler : public PanHandler,
                          public input::InputHandler,
                          public settings::FlagListener {
 public:
  // Multiply mouse wheel values by this value when zooming.
  static constexpr float kMouseWheelZoomFactor = 0.0005f;
  // When scrolling via trackpad gestures, one direction of movement is likely
  // to be dominant. Multiply the non-dominant direction by this factor, which
  // was arrived at by fiddling with a trackpad.
  static constexpr float kTrackpadSquashFactor = 0.2f;

  using SharedDeps =
      service::Dependencies<input::InputDispatch, Camera, settings::Flags,
                            CameraController, PageBounds, CameraConstraints,
                            AnimationController, WallClockInterface,
                            PageManager>;

  DefaultPanHandler(std::shared_ptr<input::InputDispatch> input,
                    std::shared_ptr<Camera> camera,
                    std::shared_ptr<settings::Flags> flags,
                    std::shared_ptr<CameraController> camera_controller,
                    std::shared_ptr<PageBounds> page_bounds,
                    std::shared_ptr<CameraConstraints> constraints,
                    std::shared_ptr<AnimationController> animation_controller,
                    std::shared_ptr<WallClockInterface> wall_clock,
                    std::shared_ptr<PageManager> page_manager);
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  void SetAllowOneFingerPan(bool enable_one_finger_pan) override;

  // Ensure that the given screen-coords Rect is kept within the page bounds and
  // the camera's scale does not go below minimum_scale. This applies to figure
  // pans and zooms: if the pan or zoom would result in a camera configuration
  // such that the page no longer includes these screen coordinates or would
  // have too low a scale, the pan/zoom will either be rejected or will be
  // modified to ensure the required Rect is still within the page.
  // So if the Rect ((10, 10), (20, 20)) is provided here, only pans and zooms
  // that result in that rectangle being within the page will be accepted.
  void EnforceMovementConstraint(const Rect& required,
                                 float min_scale) override {
    rect_drag_modifier_.StartEnforcement(required, min_scale);
  };

  // Stop enforcing required Rect.
  void StopMovementConstraintEnforcement() override {
    rect_drag_modifier_.StopEnforcement();
  }

  void SetMousewheelPolicy(MousewheelPolicy mousewheel_policy) override {
    mousewheel_policy_ = mousewheel_policy;
  }

  void OnFlagChanged(settings::Flag which, bool new_value) override {
    if (which == settings::Flag::StrictNoMargins) {
      strict_camera_constraints_ = new_value;
    }
  }

  inline std::string ToString() const override { return "<PanHandler>"; }

 private:
  // Calculate the current drag rate per second.
  void NoteDragEvent(const input::InputData& data, glm::vec2 world_drag);

  // Called by momentum animation.
  void SetDragPerSecond(glm::vec2 drag_per_second);

  // Start a fling animation if fling is enabled.
  void MaybeFling();

  // Construct fake drag data from mouse wheel input.
  input::DragData DragDataFromWheelInput(const input::InputData& input_data);

  // Returns true if one contact point should pan.  This is true if:
  //  - SetAllowOneFingerPan was explicitly enabled.
  //  - It's a right-click.
  //  - It's a touch and pen mode is enabled.
  //  - It's on the out of bounds area.
  //  - It's between pages in a multi-page document.
  bool ShouldOneTouchPan(const input::InputData& data);

  bool allow_one_finger_pan_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<CameraController> camera_controller_;
  input::DragReco drag_reco_;
  Rect saved_world_wnd_;
  std::shared_ptr<settings::Flags> flags_;
  RequiredRectDragModifier rect_drag_modifier_;
  std::shared_ptr<CameraConstraints> constraints_;
  std::shared_ptr<AnimationController> animation_controller_;
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<WallClockInterface> wall_clock_;
  std::shared_ptr<PageManager> page_manager_;

  MousewheelPolicy mousewheel_policy_ = MousewheelPolicy::ZOOMS;
  bool strict_camera_constraints_ = false;

  // These members are concerned with "fling" behavior.
  InputTimeS last_drag_time_{0};
  WallTimeS last_anim_frame_time_{0};
  glm::vec2 drag_per_second_{0};
  AnimatedFn<glm::vec2> drag_anim_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_PAN_HANDLER_H_
