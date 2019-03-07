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

#ifndef INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONTROLLER_H_
#define INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONTROLLER_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_constraints.h"
#include "ink/engine/geometry/primitives/margin.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/animation/animated_fn.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/security.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

/*
 * For most use cases, if you want to tell the Camera to look at something, you
 * should probably do it through this service.
 *
 * If you use LookAt or AnimateTo, CameraController corrects the world
 * rectangle you've specified to make sure it meets margin and camera
 * constraints.
 *
 * CameraController observes touch input, and, when the input is done,
 * animates the camera to the nearest correct position, if any constraints are
 * violated.
 */
class CameraController : public input::InputHandler,
                         public settings::FlagListener {
 public:
  using SharedDeps =
      service::Dependencies<Camera, PageBounds, FrameState,
                            input::InputDispatch, AnimationController,
                            CameraConstraints, settings::Flags>;

  CameraController(std::shared_ptr<Camera> camera,
                   std::shared_ptr<PageBounds> bounds,
                   std::shared_ptr<FrameState> frame_state,
                   std::shared_ptr<input::InputDispatch> input_dispatch,
                   std::shared_ptr<AnimationController> anim_controller,
                   std::shared_ptr<CameraConstraints> constraints,
                   std::shared_ptr<settings::Flags> flags);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  // Exposed for testing.
  // Returns true if the animation is currently running.
  S_WARN_UNUSED_RESULT bool GetCurrentAnimationTarget(Rect* out);
  DurationS GetAnimationDurationSecs();
  virtual void StopAnimation();

  virtual void LookAt(const Rect& world_window);
  void LookAt(glm::vec2 world_center, glm::vec2 world_dim) {
    LookAt(Rect::CreateAtPoint(world_center, world_dim.x, world_dim.y));
  }

  virtual void SetInputProcessingEnabled(bool enabled);

  // Scrolls the scene 1/20th of a page down, or less if the resulting position
  // is illegal.
  void ScrollDown();

  // Scrolls the scene 1/20th of a page up, or less if the resulting position
  // is illegal.
  void ScrollUp();

  // Scrolls the scene so that the current bottom world coordinate is just above
  // the camera, or less if the resulting position is illegal.
  void PageDown();

  // Scrolls the scene so that the current top world coordinate is just below
  // the camera, or less if the resulting position is illegal.
  void PageUp();

  inline std::string ToString() const override { return "<CameraController>"; }

  // settings::FlagListener
  void OnFlagChanged(settings::Flag which, bool new_value) override;

 protected:
  // For faking in tests.
  CameraController()
      : rect_anim_(nullptr, []() { return Rect(); }, [](const Rect& r) {}) {}

 private:
  void MaybeStartAnimating();
  void StartAnimation(Rect to_rect);

  std::shared_ptr<Camera> camera_;  // The actual camera to move.
  std::shared_ptr<PageBounds> bounds_;

  std::shared_ptr<FrameState> frame_state_;

  AnimatedFn<Rect> rect_anim_;
  bool input_processing_enabled_;
  std::shared_ptr<CameraConstraints> constraints_;

  // This bool mirrors the state of the EnableHostCameraControl flag. When that
  // flag is true, this object does not apply any constraints to LookAt(), and
  // does not perform any view-correction animations.
  bool constraints_active_;
};

}  // namespace ink

#endif  // INK_ENGINE_CAMERA_CONTROLLER_CAMERA_CONTROLLER_H_
