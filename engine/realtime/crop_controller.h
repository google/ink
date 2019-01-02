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

#ifndef INK_ENGINE_REALTIME_CROP_CONTROLLER_H_
#define INK_ENGINE_REALTIME_CROP_CONTROLLER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/realtime/pan_handler.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/animation/animated_value.h"

namespace ink {
namespace tools {

namespace crop {
static constexpr int kTop = 1;
static constexpr int kLeft = 1 << 1;
static constexpr int kBottom = 1 << 2;
static constexpr int kRight = 1 << 3;
}  // namespace crop

// Bounds are stored in both screen and world coordinates. Usually the screen
// coordinates are authoratative, but when the screen rotates, the world
// coordinates must be used in order to re-derive the screen coordiantes.
class Bounds {
 public:
  static Bounds FromScreen(const Rect& screen, const Camera& cam) {
    Bounds b;
    b.SetByScreen(screen, cam);
    return b;
  }
  // Set the bounds to the given screen coords.
  void SetByScreen(const Rect& screen, const Camera& cam) {
    screen_ = screen;
    world_ = geometry::Transform(screen, cam.ScreenToWorld());
  }
  // Set the bounds to the given world coords.
  void SetByWorld(const Rect& world, const Camera& cam) {
    world_ = world;
    screen_ = geometry::Transform(world, cam.WorldToScreen());
  }
  Rect Screen() const { return screen_; }
  Rect World() const { return world_; }

 private:
  Rect screen_;
  Rect world_;
};

// A CropAction keeps track of a single drag/release gesture. It knows how
// to constrain a drag, and mutates the bounds Rect as it's updated.
class CropAction {
 public:
  // constraint: which coordinate(s) can change in this gesture, expressed as a
  //     bitmask. A 0 constraint means "moving the whole crop Rect".
  // start_world: gesture start point in world coordinates.
  // bounds: address of world-space rectangle to manipulate during gesture.
  // drag_limit_world: world space within which the drag point is permitted to
  //     move, or total allowed area when dragging the entire crop Rect.
  CropAction(int constraint, glm::vec2 start_screen, Bounds* bounds,
             Rect drag_limit_screen);
  void HandleInput(const input::InputData& data, const Camera& camera);
  int Constraint() { return constraint_; }
  Rect DragLimitScreen() { return drag_limit_screen_; }

 private:
  std::string ConstraintString() const;

  int constraint_;
  glm::vec2 start_screen_{0, 0};
  Bounds* bounds_;
  Rect drag_limit_screen_;
  Rect start_bounds_;
};

// CropTool is used to change the scene's page bounds.
// It first disables the engine's default pan/zoom handling, then zooms out to
// reveal the entire scene.
//
// When active, the controller draws crop boundaries, the edges of which can
// be dragged to modify. The InteriorInputPolicy can be set to control whether
// drag events within the crop bounds are ignored or cause the entire crop
// rectangle to be moved.
class CropController : public IDrawable, public input::IInputHandler {
 public:
  // This is how long camera animations take when zooming in and out of crop
  // operations.
  static const DurationS kCameraAnimationDuration;

  // This is how long it takes for the rule-of-threes box to fade in or out.
  static const DurationS kRuleOfThreesFadeAnimationDuration;

 public:
  using SharedDeps = service::Dependencies<CameraController, Camera, PageBounds,
                                           GLResourceManager, IEngineListener,
                                           AnimationController, PanHandler>;

  CropController(std::shared_ptr<CameraController> camera_controller,
                 std::shared_ptr<Camera> camera,
                 std::shared_ptr<PageBounds> page_bounds,
                 std::shared_ptr<GLResourceManager> gl_resource_manager,
                 std::shared_ptr<IEngineListener> engine_listener,
                 std::shared_ptr<AnimationController> animation_controller,
                 std::shared_ptr<PanHandler> pan_handler);
  ~CropController() override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void Update(const Camera& cam);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  void Enable(bool enabled);
  void Commit();

  // Set the crop area to the given rectangle in world coordinates.
  //
  // This method is only meaningful if CropController is enabled. Invalid,
  // empty or out-of-page-bounds rectangles will result in an error logged and
  // no change in crop state.
  void SetCrop(const Rect& crop_rect);

  enum class InteriorInputPolicy {
    // Input events within the crop Rect cause the Rect to be moved.
    kMove,
    // Input events within the crop Rect are ignored by CropController.
    kPassthrough,
  };

  void SetInteriorInputPolicy(InteriorInputPolicy policy) {
    interior_input_policy_ = policy;
  }

  bool RefuseAllNewInput() override { return false; }
  input::Priority InputPriority() const override {
    return input::Priority::Crop;
  }

  // With the given input data and boundaries, populate the given out param
  // to indicate the beginning of a crop gesture. For example, if the given
  // touch is near a corner handle, permit dragging both of the incident axes
  // of that handle, and permit it to be dragged to the limits of the uncropped
  // scene in those directions, but not too near the other axes. If the touch is
  // inside the bounds rectangle, permit moving the crop area.
  // The given hit radius determines what's "near" a control, and the given min
  // size determines how close a dragged control can get to the other side of
  // the crop region.
  void DetectResizeGesture(const input::InputData& data, Rect drag_limit_screen,
                           float hit_radius_px, float min_size_px,
                           Bounds* bounds, std::unique_ptr<CropAction>* action);

  inline std::string ToString() const override { return "<CropController>"; }

 private:
  // The current cropping bounds.
  Bounds bounds_;

  // Same as above, but using internal constants for hit radius and min size.
  void DetectResizeGesture(const input::InputData& data,
                           const Rect& drag_limit_screen, Bounds* bounds_screen,
                           std::unique_ptr<CropAction>* action);

  // See if the given InputData hits a drag target in this tool. If so, then
  // crop_action_ is populated; otherwise, it remains nullptr.
  void MaybeBeginCropAction(const input::InputData& data);

  // Called when crop mode is enabled.
  void EnterCropMode();

  // Called when crop mode is disabled.
  void ExitCropMode();

  // We fix pan and zoom for the entire interaction.
  std::shared_ptr<CameraController> camera_controller_;

  // We may need to know what the camera is looking at when the tool is enabled.
  std::shared_ptr<Camera> camera_;

  // This is where we get and set the page bounds.
  std::shared_ptr<PageBounds> page_bounds_;

  // This is where we get the background image first instance Rect, if any.
  std::shared_ptr<GLResourceManager> gl_resource_manager_;

  // For notifying the host when we enter and exit crop mode.
  std::shared_ptr<IEngineListener> engine_listener_;

  std::shared_ptr<PanHandler> pan_handler_;

  // This draws the UI elements.
  ShapeRenderer shape_renderer_;

  // This helps us fade in/out the rule-of-threes boxes.
  AnimatedValue<float> rule_of_threes_alpha_;

  // The following shapes are used to stamp various UI elements; they're mutable
  // because we move them around the screen while stamping them.

  // Filled Rect with no border.
  mutable Shape solid_rect_;
  // Bordered Rect with no fill.
  mutable Shape bordered_rect_;
  // Translucent gray Rect for stamping onto the cropped-out regions.
  mutable Shape cropped_area_;

  // The uncropped_world_ is the maximal Rect that can be revealed by the crop
  // tool, i.e., the position of the background image. This is populated when
  // the tool is enabled.
  Rect uncropped_world_;

  // Screen dimensions last observed. If this changes, a rotation may have
  // happened
  glm::ivec2 previous_screen_dim_{0, 0};

  // Non-nullptr when tracking a gesture.
  std::unique_ptr<CropAction> crop_action_;

  InteriorInputPolicy interior_input_policy_ = InteriorInputPolicy::kMove;

  bool enabled_;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_CROP_CONTROLLER_H_
