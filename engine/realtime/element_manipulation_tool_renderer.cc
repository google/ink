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

#include "ink/engine/realtime/element_manipulation_tool_renderer.h"

#include "third_party/absl/types/span.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

static const ElementManipulationToolHandle kAllHandles[] = {
    ElementManipulationToolHandle::NONE,
    ElementManipulationToolHandle::RIGHT,
    ElementManipulationToolHandle::TOP,
    ElementManipulationToolHandle::LEFT,
    ElementManipulationToolHandle::BOTTOM,
    ElementManipulationToolHandle::RIGHTTOP,
    ElementManipulationToolHandle::LEFTTOP,
    ElementManipulationToolHandle::LEFTBOTTOM,
    ElementManipulationToolHandle::RIGHTBOTTOM,
    ElementManipulationToolHandle::ROTATION,
};
const absl::Span<const ElementManipulationToolHandle>
    kAllElementManipulationToolHandles(kAllHandles, sizeof(kAllHandles) /
                                                        sizeof(kAllHandles[0]));

static glm::vec2 HandlePositionInternal(ElementManipulationToolHandle handle,
                                        const Camera* camera,
                                        RotRect world_rect) {
  const float angle_cos = std::cos(world_rect.Rotation());
  const float angle_sin = std::sin(world_rect.Rotation());
  glm::vec2 horz{0.5 * world_rect.Width() * angle_cos,
                 0.5 * world_rect.Width() * angle_sin};
  glm::vec2 vert{-0.5 * world_rect.Height() * angle_sin,
                 0.5 * world_rect.Height() * angle_cos};
  switch (handle) {
    case ElementManipulationToolHandle::NONE:
      return world_rect.Center();
    case ElementManipulationToolHandle::RIGHT:
      return world_rect.Center() + horz;
    case ElementManipulationToolHandle::RIGHTTOP:
      return world_rect.Center() + horz + vert;
    case ElementManipulationToolHandle::TOP:
      return world_rect.Center() + vert;
    case ElementManipulationToolHandle::LEFTTOP:
      return world_rect.Center() - horz + vert;
    case ElementManipulationToolHandle::LEFT:
      return world_rect.Center() - horz;
    case ElementManipulationToolHandle::LEFTBOTTOM:
      return world_rect.Center() - horz - vert;
    case ElementManipulationToolHandle::BOTTOM:
      return world_rect.Center() - vert;
    case ElementManipulationToolHandle::RIGHTBOTTOM:
      return world_rect.Center() + horz - vert;
    case ElementManipulationToolHandle::ROTATION: {
      ASSERT(camera);
      float dist = camera->ConvertDistance(0.5f, DistanceType::kCm,
                                           DistanceType::kWorld);
      return world_rect.Center() + vert +
             glm::vec2{-dist * angle_sin, dist * angle_cos};
    }
  }
  SLOG(SLOG_WARNING, "invalid ElementManipulationToolHandle value: $0",
       static_cast<int>(handle));
  return world_rect.Center();
}

static ElementManipulationToolHandle OppositeHandle(
    ElementManipulationToolHandle handle) {
  switch (handle) {
    case ElementManipulationToolHandle::NONE:
      return ElementManipulationToolHandle::NONE;
    case ElementManipulationToolHandle::RIGHT:
      return ElementManipulationToolHandle::LEFT;
    case ElementManipulationToolHandle::RIGHTTOP:
      return ElementManipulationToolHandle::LEFTBOTTOM;
    case ElementManipulationToolHandle::TOP:
      return ElementManipulationToolHandle::BOTTOM;
    case ElementManipulationToolHandle::LEFTTOP:
      return ElementManipulationToolHandle::RIGHTBOTTOM;
    case ElementManipulationToolHandle::LEFT:
      return ElementManipulationToolHandle::RIGHT;
    case ElementManipulationToolHandle::LEFTBOTTOM:
      return ElementManipulationToolHandle::RIGHTTOP;
    case ElementManipulationToolHandle::BOTTOM:
      return ElementManipulationToolHandle::TOP;
    case ElementManipulationToolHandle::RIGHTBOTTOM:
      return ElementManipulationToolHandle::LEFTTOP;
    case ElementManipulationToolHandle::ROTATION:
      return ElementManipulationToolHandle::NONE;
  }
  SLOG(SLOG_WARNING, "invalid ElementManipulationToolHandle value: $0",
       static_cast<int>(handle));
  return ElementManipulationToolHandle::NONE;
}

glm::vec2 ElementManipulationToolHandlePosition(
    ElementManipulationToolHandle handle, const Camera& camera,
    RotRect world_rect) {
  return HandlePositionInternal(handle, &camera, world_rect);
}

glm::vec2 ElementManipulationToolHandleAnchor(
    ElementManipulationToolHandle handle, RotRect world_rect) {
  // Only the ROTATION handle needs a non-null camera, and OppositeHandle never
  // returns ROTATION, so we can pass nullptr for camera here.
  return HandlePositionInternal(OppositeHandle(handle), nullptr, world_rect);
}

ElementManipulationToolRenderer::ElementManipulationToolRenderer(
    const service::UncheckedRegistry& registry)
    : scene_graph_(registry.GetShared<SceneGraph>()),
      frame_state_(registry.GetShared<FrameState>()),
      renderer_(registry.GetShared<LiveRenderer>()),
      wall_clock_(registry.GetShared<WallClockInterface>()),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      outline_(ShapeGeometry::Type::Rectangle),
      outline_glow_(ShapeGeometry::Type::Rectangle),
      rotation_bar_(ShapeGeometry::Type::Rectangle),
      shape_renderer_(registry),
      partition_renderer_(registry),
      mesh_renderer_(registry),
      flags_(registry.GetShared<settings::Flags>()) {
  outline_.SetFillVisible(false);
  outline_.SetBorderColor(kGoogleBlue500);
  outline_glow_.SetFillVisible(false);
  outline_glow_.SetBorderColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  rotation_bar_.SetFillColor(kGoogleBlue500);
  rotation_bar_.SetBorderColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  for (auto handle : kAllElementManipulationToolHandles) {
    if (handle == ElementManipulationToolHandle::NONE) continue;
    auto shape = absl::make_unique<Shape>(
        handle == ElementManipulationToolHandle::ROTATION
            ? ShapeGeometry::Type::Circle
            : ShapeGeometry::Type::Rectangle);
    shape->SetFillColor(kGoogleBlue500);
    shape->SetBorderColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    handle_shapes_[handle] = std::move(shape);
  }
}

void ElementManipulationToolRenderer::Draw(const Camera& cam,
                                           FrameTimeS draw_time,
                                           glm::mat4 transform) const {
  mesh_renderer_.Draw(cam, draw_time, bg_overlay_);

  // Draw the selected elements, with the transform applied
  partition_renderer_.Draw(cam, draw_time, *scene_graph_, blit_attrs::Blit(),
                           transform);

  shape_renderer_.Draw(cam, draw_time, rotation_bar_);
  for (const auto& handle_shape : handle_shapes_) {
    shape_renderer_.Draw(cam, draw_time, *handle_shape.second);
  }

  shape_renderer_.Draw(cam, draw_time, outline_glow_);
  shape_renderer_.Draw(cam, draw_time, outline_);
  if (flags_->GetFlag(settings::Flag::EnableSelectionBoxHandles)) {
    for (const auto& handle_shape : handle_shapes_) {
      shape_renderer_.Draw(cam, draw_time, *handle_shape.second);
    }
  }
}

void ElementManipulationToolRenderer::SetOutlinePosition(const Camera& cam,
                                                         RotRect region) {
  glm::vec2 inner_size{
      cam.ConvertDistance(2, DistanceType::kDp, DistanceType::kWorld)};
  glm::vec2 outer_size{
      cam.ConvertDistance(4, DistanceType::kDp, DistanceType::kWorld)};
  outline_.SetSizeAndPosition(region.Inset(inner_size * 0.5f), inner_size,
                              false);
  outline_glow_.SetSizeAndPosition(region.Inset(outer_size * 0.5f), outer_size,
                                   false);

  if (flags_->GetFlag(settings::Flag::EnableSelectionBoxHandles)) {
    glm::vec2 handle_border_size = (outer_size - inner_size) * 0.5f;
    glm::vec2 handle_rect_size{
        cam.ConvertDistance(10, DistanceType::kDp, DistanceType::kWorld)};
    for (const auto& handle_shape : handle_shapes_) {
      ElementManipulationToolHandle handle = handle_shape.first;
      glm::vec2 position =
          ElementManipulationToolHandlePosition(handle, cam, region);
      float size_mult =
          handle == ElementManipulationToolHandle::ROTATION ? 1.3f : 1.0f;
      handle_shape.second->SetSizeAndPosition(
          RotRect(position, handle_rect_size * size_mult, region.Rotation()),
          handle_border_size, false);
    }

    if (flags_->GetFlag(settings::Flag::EnableRotation)) {
      glm::vec2 pt1 = ElementManipulationToolHandlePosition(
          ElementManipulationToolHandle::ROTATION, cam, region);
      glm::vec2 pt2 = ElementManipulationToolHandlePosition(
          ElementManipulationToolHandle::TOP, cam, region);
      glm::vec2 center = (pt1 + pt2) / 2.0f;
      float width = inner_size.x;
      float height = glm::distance(pt1, pt2);
      rotation_bar_.SetSizeAndPosition(
          RotRect(center, {width, height}, region.Rotation()),
          (outer_size - inner_size) / 2.0f, false);
    }
  }
}

void ElementManipulationToolRenderer::SetOutlineVisible(bool visible) {
  outline_.SetVisible(visible);
  outline_glow_.SetVisible(visible);
  const bool handles_visible =
      visible && flags_->GetFlag(settings::Flag::EnableSelectionBoxHandles);
  const bool rotation_visible =
      handles_visible && flags_->GetFlag(settings::Flag::EnableRotation);
  rotation_bar_.SetVisible(rotation_visible);
  for (const auto& handle_shape : handle_shapes_) {
    handle_shape.second->SetVisible(
        handle_shape.first == ElementManipulationToolHandle::ROTATION
            ? rotation_visible
            : handles_visible);
  }
}

void ElementManipulationToolRenderer::Update(const Camera& cam,
                                             FrameTimeS draw_time,
                                             Rect element_mbr, RotRect region,
                                             glm::mat4 transform) {
  UpdateWithTimer(cam, draw_time, element_mbr, region, transform,
                  Timer(wall_clock_, 0.004));
}

void ElementManipulationToolRenderer::UpdateWithTimer(
    const Camera& cam, FrameTimeS draw_time, Rect element_mbr, RotRect region,
    glm::mat4 transform, Timer timer) {
  SetOutlinePosition(cam, geometry::Transform(region, transform));

  // Apply the inverse transformation to the camera to find the visible portion
  // of the elements, relative to the elements' original position.
  Rect inverted_window =
      geometry::Transform(cam.WorldWindow(), glm::inverse(transform))
          .ContainingRectWithAspectRatio(cam.WorldWindow().AspectRatio());
  Rect intersection;
  if (geometry::Intersection(inverted_window, element_mbr, &intersection) &&
      intersection.Area() > 0) {
    // This will always contain at least the portion of the elements that is
    // visible on screen, and possibly some amount of "buffer" area if more of
    // the element could fit on screen (i.e. if inverted_window doesn't lie
    // entirely within element_mbr) -- this prevents us from needlessly
    // restarting the partition back buffer.
    Rect partition_window = element_mbr.ClosestInteriorRect(inverted_window);
    if (partition_window == element_mbr) {
      // The entirety of the transformed elements would fit on screen. If the
      // camera window transformed by just the inverse scale (no rotation or
      // translation) contains the elements, center that on the elements and use
      // it for the partition camera. This ensures that pixel density of the
      // transformed partition buffer is roughly the same as the pixel density
      // of the screen.
      Rect scaled_window = cam.WorldWindow().Scale(
          1 / matrix_utils::GetAverageAbsScale(transform));
      if (scaled_window.Width() >= element_mbr.Width() &&
          scaled_window.Height() >= element_mbr.Height()) {
        scaled_window.SetCenter(element_mbr.Center());
        partition_window = scaled_window;
      }
    }

    // Note because the resolution is different than the main renderer
    // resolution, there will be visible aliasing differences between selected
    // and non-selected elements.
    auto partition_camera = cam;
    partition_camera.SetWorldWindow(partition_window);
    partition_renderer_.Update(timer, partition_camera, draw_time,
                               *scene_graph_);
  }

  // Draw a background overlay over non-selected elements
  glm::vec4 bg_color(0.9, 0.9, 0.9, 0.5);
  bg_color = RGBtoRGBPremultiplied(bg_color);
  MakeRectangleMesh(&bg_overlay_, cam.WorldWindow(), bg_color);
  gl_resources_->mesh_vbo_provider->ReplaceVBOs(&bg_overlay_, GL_DYNAMIC_DRAW);
}

void ElementManipulationToolRenderer::Enable(bool enabled) {
  SetOutlineVisible(enabled);
  if (enabled) {
    partition_renderer_.EnableFramerateLocks(frame_state_);
  } else {
    partition_renderer_.DisableFramerateLocks();
  }
}

void ElementManipulationToolRenderer::Synchronize() {
  renderer_->Synchronize(frame_state_->GetLastFrameTime());
}

void ElementManipulationToolRenderer::SetElements(
    const Camera& cam, const std::vector<ElementId>& elements, Rect element_mbr,
    RotRect region) {
  if (partition_renderer_.RenderingSize() != cam.ScreenDim()) {
    partition_renderer_.Resize(cam.ScreenDim());
  }
  partition_renderer_.AssignPartitionData(
      PartitionData(partition_renderer_.CurrentPartition() + 1,
                    scene_graph_->GroupifyElements(elements)));

  Timer timer(wall_clock_, 2);
  UpdateWithTimer(cam, frame_state_->GetFrameTime(), element_mbr, region,
                  glm::mat4{1}, timer);
  if (timer.Expired()) {
    SLOG(SLOG_WARNING, "time expired while attempting to select elements");
  }
  SetOutlineVisible(true);

  if (!elements.empty()) {
    renderer_->Synchronize(frame_state_->GetFrameTime());
  }
}

}  // namespace tools
}  // namespace ink
