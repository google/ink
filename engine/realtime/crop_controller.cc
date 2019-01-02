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

#include "ink/engine/realtime/crop_controller.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

using glm::vec2;
using glm::vec4;

namespace ink {
namespace tools {

using crop::kBottom;
using crop::kLeft;
using crop::kRight;
using crop::kTop;

// You hit a drag handle if you touch within this many centimeters of it.
static constexpr float kHitRadiusTouchCm = .4;
static constexpr float kHitRadiusNonTouchCm = .3;

// The visual thickness of the white outline around the cropped area.
static constexpr float kCropAreaBorderThicknessCm = 0.03;

// The distance a handle extends.
static constexpr float kHandleLengthCm = 0.3;
// The width (thickness) of a handle.
static constexpr float kHandleThicknessCm = 0.08;

// The component-wise size of the crop box must be at least this many times the
// size of kHandleLengthCm
static constexpr float kMinSizeHandleMultiplier = 4;

// Scale the full scene Rect by this much and set the camera to it.
static constexpr float kViewScale = 1.05;

// The maximum alpha for the rule-of-threes lines.
static constexpr float kRuleOfThreesAlpha = 0.6;

static constexpr float kBorderHandleGray = 0.259;

// Don't let the Camera scale things down below this value.
static constexpr float kMinimumScale = 0.7;

CropAction::CropAction(int constraint, vec2 start_screen, Bounds* bounds,
                       Rect drag_limit_screen)
    : constraint_(constraint),
      start_screen_(start_screen),
      bounds_(bounds),
      drag_limit_screen_(drag_limit_screen),
      start_bounds_(bounds->Screen()) {}

// Useful during debugging.
std::string CropAction::ConstraintString() const {
  std::string s;
  if (constraint_ & kLeft) {
    s.append("kLeft");
  } else if (constraint_ & kRight) {
    s.append("kRight");
  }
  if (constraint_ & kTop) {
    if (!s.empty()) s.append("|");
    s.append("kTop");
  } else if (constraint_ & kBottom) {
    if (!s.empty()) s.append("|");
    s.append("kBottom");
  }
  return s;
}

void CropAction::HandleInput(const input::InputData& data,
                             const Camera& camera) {
  vec2 delta = data.screen_pos - start_screen_;
  const auto& lim = drag_limit_screen_;

  Rect bounds_screen(bounds_->Screen());
  if (constraint_) {
    // Resizing the crop.
    if (constraint_ & kTop) {
      bounds_screen.to.y =
          glm::clamp(start_bounds_.to.y + delta.y, lim.Bottom(), lim.Top());
    } else if (constraint_ & kBottom) {
      bounds_screen.from.y =
          glm::clamp(start_bounds_.from.y + delta.y, lim.Bottom(), lim.Top());
    }
    if (constraint_ & kLeft) {
      bounds_screen.from.x =
          glm::clamp(start_bounds_.from.x + delta.x, lim.Left(), lim.Right());
    } else if (constraint_ & kRight) {
      bounds_screen.to.x =
          glm::clamp(start_bounds_.to.x + delta.x, lim.Left(), lim.Right());
    }
  } else {
    // Moving the crop.
    bounds_screen.from = start_bounds_.from + delta;
    bounds_screen.to = start_bounds_.to + delta;
    bounds_screen.SetLeft(std::max(bounds_screen.Left(), lim.Left()));
    bounds_screen.SetBottom(std::max(bounds_screen.Bottom(), lim.Bottom()));
    bounds_screen.SetTop(std::min(bounds_screen.Top(), lim.Top()));
    bounds_screen.SetRight(std::min(bounds_screen.Right(), lim.Right()));
  }
  bounds_->SetByScreen(bounds_screen, camera);
}

const DurationS CropController::kCameraAnimationDuration = DurationS(0.5);

const DurationS CropController::kRuleOfThreesFadeAnimationDuration =
    DurationS(0.25);

CropController::CropController(
    std::shared_ptr<CameraController> camera_controller,
    std::shared_ptr<Camera> camera, std::shared_ptr<PageBounds> page_bounds,
    std::shared_ptr<GLResourceManager> gl_resource_manager,
    std::shared_ptr<IEngineListener> engine_listener,
    std::shared_ptr<AnimationController> animation_controller,
    std::shared_ptr<PanHandler> pan_handler)
    : camera_controller_(camera_controller),
      camera_(camera),
      page_bounds_(page_bounds),
      gl_resource_manager_(gl_resource_manager),
      engine_listener_(engine_listener),
      pan_handler_(pan_handler),
      shape_renderer_(gl_resource_manager),
      rule_of_threes_alpha_(0.0, animation_controller),
      solid_rect_(ShapeGeometry::Type::Rectangle),
      bordered_rect_(ShapeGeometry::Type::Rectangle),
      cropped_area_(ShapeGeometry::Type::Rectangle),
      enabled_(false) {
  bordered_rect_.SetBorderColor(
      vec4(kBorderHandleGray, kBorderHandleGray, kBorderHandleGray, 1.0));
  bordered_rect_.SetFillColor(vec4(0.0, 0.0, 0.0, 0.0));
  cropped_area_.SetFillColor(vec4(0.0, 0.0, 0.0, 0.2));
}

CropController::~CropController() {
  SLOG(SLOG_OBJ_LIFETIME, "CropController dtor");
}

void CropController::DetectResizeGesture(const input::InputData& data,
                                         const Rect& drag_limit_screen,
                                         Bounds* bounds,
                                         std::unique_ptr<CropAction>* action) {
  const float hitRadiusCm = data.type == input::InputType::Touch
                                ? kHitRadiusTouchCm
                                : kHitRadiusNonTouchCm;
  const float hitRadiusPixels = camera_->ConvertDistance(
      hitRadiusCm, DistanceType::kCm, DistanceType::kScreen);

  float min_size = kMinSizeHandleMultiplier *
                   camera_->ConvertDistance(kHandleLengthCm, DistanceType::kCm,
                                            DistanceType::kScreen);
  DetectResizeGesture(data, drag_limit_screen, hitRadiusPixels, min_size,
                      bounds, action);
}

void CropController::DetectResizeGesture(const input::InputData& data,
                                         Rect drag_limit_screen,
                                         float hit_radius_px, float min_size_px,
                                         Bounds* bounds,
                                         std::unique_ptr<CropAction>* action) {
  action->reset();
  int constraint = 0;

  Rect bounds_screen = bounds->Screen();
  const vec2& p = data.screen_pos;
  if (geometry::Distance(p, bounds_screen.Lefttop()) < hit_radius_px) {
    constraint = kLeft | kTop;
  } else if (geometry::Distance(p, bounds_screen.Righttop()) < hit_radius_px) {
    constraint = kRight | kTop;
  } else if (geometry::Distance(p, bounds_screen.Leftbottom()) <
             hit_radius_px) {
    constraint = kLeft | kBottom;
  } else if (geometry::Distance(p, bounds_screen.Rightbottom()) <
             hit_radius_px) {
    constraint = kRight | kBottom;
  } else if (geometry::Distance(p, bounds_screen.TopSegment()) <
             hit_radius_px) {
    constraint = kTop;
  } else if (geometry::Distance(p, bounds_screen.RightSegment()) <
             hit_radius_px) {
    constraint = kRight;
  } else if (geometry::Distance(p, bounds_screen.LeftSegment()) <
             hit_radius_px) {
    constraint = kLeft;
  } else if (geometry::Distance(p, bounds_screen.BottomSegment()) <
             hit_radius_px) {
    constraint = kBottom;
  }

  if (constraint & kTop) {
    drag_limit_screen.from.y = bounds_screen.Bottom() + min_size_px;
  } else if (constraint & kBottom) {
    drag_limit_screen.to.y = bounds_screen.Top() - min_size_px;
  }

  if (constraint & kLeft) {
    drag_limit_screen.to.x = bounds_screen.Right() - min_size_px;
  } else if (constraint & kRight) {
    drag_limit_screen.from.x = bounds_screen.Left() + min_size_px;
  }

  if (constraint) {
    // A control was hit: resize the crop Rect.
    *action = absl::make_unique<CropAction>(constraint, data.screen_pos, bounds,
                                            drag_limit_screen);
  } else if (bounds_screen.Contains(data.screen_pos) &&
             interior_input_policy_ == InteriorInputPolicy::kMove) {
    // The crop interior was hit: move the crop Rect.
    *action = absl::make_unique<CropAction>(constraint, data.screen_pos, bounds,
                                            drag_limit_screen);
  }
}

void CropController::MaybeBeginCropAction(const input::InputData& data) {
  // Inset the camera's world window by the handle thickness (we prohibit the
  // crop box from getting too close to the edge and stuck).
  const float thickness_world = camera_->ConvertDistance(
      kHandleThicknessCm, DistanceType::kCm, DistanceType::kWorld);
  auto world_window = camera_->WorldWindow();
  world_window =
      world_window.Inset(glm::vec2(thickness_world, thickness_world));

  // The drag limit is the intersection of this inset world window and the
  // uncropped bounds of the document.
  Rect drag_limit_world;
  geometry::Intersection(uncropped_world_, world_window, &drag_limit_world);

  Rect drag_limit_screen =
      geometry::Transform(drag_limit_world, camera_->WorldToScreen());

  DetectResizeGesture(data, drag_limit_screen, &bounds_, &crop_action_);
}

input::CaptureResult CropController::OnInput(const input::InputData& data,
                                             const Camera& camera) {
  if (data.Get(input::Flag::Cancel)) {
    return input::CapResRefuse;
  }

  if (data.Get(input::Flag::InContact) &&
      (data.Get(input::Flag::Primary) || data.n_down == 1)) {
    if (crop_action_) {
      crop_action_->HandleInput(data, camera);
    } else {
      MaybeBeginCropAction(data);
      if (crop_action_) {
        rule_of_threes_alpha_.AnimateTo(kRuleOfThreesAlpha,
                                        kRuleOfThreesFadeAnimationDuration);
      } else {
        // Not a crop action, don't handle this input.
        return input::CapResRefuse;
      }
    }
  } else if (crop_action_) {
    // Modification of the crop box is complete.
    crop_action_.reset();
    // Ensure the new crop box is enforce during subsequent panning/zooming.
    pan_handler_->EnforceMovementConstraint(bounds_.Screen(), kMinimumScale);
    rule_of_threes_alpha_.AnimateTo(0.0, kRuleOfThreesFadeAnimationDuration);
  }

  return input::CapResCapture;
}

void CropController::Update(const Camera& cam) {
  if (!enabled_) return;

  if (previous_screen_dim_ != cam.ScreenDim()) {
    // Viewport has changed, use the world coordinates to update the screen
    // coordinates.
    previous_screen_dim_ = cam.ScreenDim();
    bounds_.SetByWorld(bounds_.World(), cam);
    pan_handler_->EnforceMovementConstraint(bounds_.Screen(), kMinimumScale);
  } else {
    // Update the world bounds (in case camera has changed).
    bounds_.SetByScreen(bounds_.Screen(), cam);
  }
}

void CropController::Draw(const Camera& cam, FrameTimeS draw_time) const {
  if (!enabled_) return;

  Rect w;
  // Define the "outside current crop" area to be the area outside of the crop
  // but within the page and camera.
  geometry::Intersection(cam.WorldWindow(), page_bounds_->Bounds(), &w);

  const auto b = bounds_.World();

  // Stamp the "outside current crop" area as four rectangles.
  cropped_area_.SetSizeAndPosition(Rect(w.Left(), b.Top(), w.Right(), w.Top()));
  shape_renderer_.Draw(cam, draw_time, cropped_area_);
  cropped_area_.SetSizeAndPosition(
      Rect(w.Left(), b.Bottom(), b.Left(), b.Top()));
  shape_renderer_.Draw(cam, draw_time, cropped_area_);
  cropped_area_.SetSizeAndPosition(
      Rect(w.Left(), w.Bottom(), w.Right(), b.Bottom()));
  shape_renderer_.Draw(cam, draw_time, cropped_area_);
  cropped_area_.SetSizeAndPosition(
      Rect(b.Right(), b.Bottom(), w.Right(), b.Top()));
  shape_renderer_.Draw(cam, draw_time, cropped_area_);

  // Draw the white box around the cropped area.
  const float border_size = cam.ConvertDistance(
      kCropAreaBorderThicknessCm, DistanceType::kCm, DistanceType::kWorld);
  bordered_rect_.SetSizeAndPosition(b, vec2(border_size), false);
  shape_renderer_.Draw(cam, draw_time, bordered_rect_);

  // Stamp the handles.
  const float thickness_world = cam.ConvertDistance(
      kHandleThicknessCm, DistanceType::kCm, DistanceType::kWorld);
  const float length_world = cam.ConvertDistance(
      kHandleLengthCm, DistanceType::kCm, DistanceType::kWorld);
  solid_rect_.SetFillColor(
      vec4(kBorderHandleGray, kBorderHandleGray, kBorderHandleGray, 1.0));

  // Upper left handle
  solid_rect_.SetSizeAndPosition(Rect(
      b.Lefttop() - vec2(thickness_world, 0),
      b.Lefttop() + vec2(length_world - thickness_world, thickness_world)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);
  solid_rect_.SetSizeAndPosition(
      Rect(b.Lefttop() - vec2(thickness_world, length_world - thickness_world),
           b.Lefttop()));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);

  // Upper right handle
  solid_rect_.SetSizeAndPosition(
      Rect(b.Righttop() - vec2(length_world - thickness_world, 0),
           b.Righttop() + vec2(thickness_world)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);
  solid_rect_.SetSizeAndPosition(
      Rect(b.Righttop() - vec2(0, length_world - thickness_world),
           b.Righttop() + vec2(thickness_world, 0)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);

  // Lower left handle.
  solid_rect_.SetSizeAndPosition(
      Rect(b.Leftbottom() - vec2(thickness_world),
           b.Leftbottom() + vec2(length_world - thickness_world, 0)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);
  solid_rect_.SetSizeAndPosition(
      Rect(b.Leftbottom() - vec2(thickness_world, 0),
           b.Leftbottom() + vec2(0, length_world - thickness_world)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);

  // Lower right handle.
  solid_rect_.SetSizeAndPosition(Rect(
      b.Rightbottom() - vec2(length_world - thickness_world, thickness_world),
      b.Rightbottom() + vec2(thickness_world, 0)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);
  solid_rect_.SetSizeAndPosition(Rect(
      b.Rightbottom(),
      b.Rightbottom() + vec2(thickness_world, length_world - thickness_world)));
  shape_renderer_.Draw(cam, draw_time, solid_rect_);

  // Draw the rule-of-threes box.
  if (rule_of_threes_alpha_.Value() > 0.0) {
    const float w3 = b.Width() / 3.0;
    const float h3 = b.Height() / 3.0;
    const float b2 = border_size / 2.0;
    solid_rect_.SetFillColor(
        vec4(1.0, 1.0, 1.0, rule_of_threes_alpha_.Value()));

    solid_rect_.SetSizeAndPosition(
        Rect(b.Left() + w3 - b2, b.Bottom(), b.Left() + w3 + b2, b.Top()));
    shape_renderer_.Draw(cam, draw_time, solid_rect_);
    solid_rect_.SetSizeAndPosition(
        Rect(b.Right() - w3 - b2, b.Bottom(), b.Right() - w3 + b2, b.Top()));
    shape_renderer_.Draw(cam, draw_time, solid_rect_);
    solid_rect_.SetSizeAndPosition(
        Rect(b.Left(), b.Bottom() + h3 - b2, b.Right(), b.Bottom() + h3 + b2));
    shape_renderer_.Draw(cam, draw_time, solid_rect_);
    solid_rect_.SetSizeAndPosition(
        Rect(b.Left(), b.Top() - h3 - b2, b.Right(), b.Top() - h3 + b2));
    shape_renderer_.Draw(cam, draw_time, solid_rect_);
  }
}

void CropController::Enable(bool enabled) {
  enabled_ = enabled;
  // Only enable camera controller when crop controller is not enabled...
  // panning and zooming are only constrained by PanHandler's required Rect
  // when cropping.
  camera_controller_->SetInputProcessingEnabled(!enabled);
  if (enabled) {
    EnterCropMode();
  } else {
    ExitCropMode();
  }
}

void CropController::EnterCropMode() {
  uncropped_world_ = page_bounds_->Bounds();
  ImageBackgroundState* bg_image;
  if (gl_resource_manager_->background_state->GetImage(&bg_image)) {
    if (bg_image->HasFirstInstanceWorldCoords()) {
      uncropped_world_ = bg_image->FirstInstanceWorldCoords();
    } else {
      SLOG(SLOG_ERROR,
           "unexpected background image without first instance world coords");
    }
  }
  if (page_bounds_->Bounds().Area() == 0 || uncropped_world_.Area() == 0) {
    uncropped_world_ = camera_->WorldWindow();
  }

  camera_controller_->LookAt(uncropped_world_.Scale(kViewScale));
  bounds_.SetByWorld(page_bounds_->Bounds(), *camera_);
  pan_handler_->EnforceMovementConstraint(bounds_.Screen(), kMinimumScale);

  page_bounds_->SetWorkingBounds(uncropped_world_);

  previous_screen_dim_ = camera_->ScreenDim();
}

void CropController::ExitCropMode() {
  pan_handler_->StopMovementConstraintEnforcement();
  page_bounds_->ClearWorkingBounds();
  if (page_bounds_->HasBounds()) {
    camera_controller_->LookAt(page_bounds_->Bounds());
  }
}

void CropController::Commit() {
  if (!enabled_) {
    SLOG(SLOG_ERROR, "Attempted to commit crop while it wasn't enabled!");
    return;
  }
  page_bounds_->SetBounds(bounds_.World(), SourceDetails::FromEngine());
}

void CropController::SetCrop(const Rect& crop_rect) {
  if (!crop_rect.IsValid() || crop_rect.Width() <= 0 ||
      crop_rect.Height() <= 0) {
    SLOG(SLOG_ERROR, "Attempted to set invalid crop bounds ($0)", crop_rect);
    return;
  }

  if (page_bounds_->HasBounds() &&
      !page_bounds_->Bounds().Contains(crop_rect)) {
    SLOG(SLOG_ERROR, "Crop bounds ($0) should be within page bounds ($1)",
         crop_rect, page_bounds_->Bounds());
    return;
  }

  if (!enabled_) {
    SLOG(SLOG_ERROR, "Attempted to set crop bounds while crop is disabled.");
    return;
  }

  bounds_.SetByWorld(crop_rect, *camera_);
}

}  // namespace tools
}  // namespace ink
