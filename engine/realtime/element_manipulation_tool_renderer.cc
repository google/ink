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

ElementManipulationToolRenderer::ElementManipulationToolRenderer(
    const service::UncheckedRegistry& registry)
    : scene_graph_(registry.GetShared<SceneGraph>()),
      frame_state_(registry.GetShared<FrameState>()),
      renderer_(registry.GetShared<LiveRenderer>()),
      wall_clock_(registry.GetShared<WallClockInterface>()),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      outline_(ShapeGeometry::Type::Rectangle),
      outline_glow_(ShapeGeometry::Type::Rectangle),
      shape_renderer_(registry),
      partition_renderer_(registry),
      mesh_renderer_(registry) {
  outline_.SetFillVisible(false);
  outline_.SetBorderColor(
      glm::vec4(kGoogleBlue500.r, kGoogleBlue500.g, kGoogleBlue500.b, 0.3f));
  outline_glow_.SetFillVisible(false);
  outline_glow_.SetBorderColor(kGoogleBlue200);
}

void ElementManipulationToolRenderer::Draw(const Camera& cam,
                                           FrameTimeS draw_time,
                                           glm::mat4 transform) const {
  mesh_renderer_.Draw(cam, draw_time, bg_overlay_);
  shape_renderer_.Draw(cam, draw_time, outline_glow_);
  shape_renderer_.Draw(cam, draw_time, outline_);

  // Draw the selected elements, with the transform applied
  partition_renderer_.Draw(cam, draw_time, *scene_graph_, blit_attrs::Blit(),
                           transform);
}

void ElementManipulationToolRenderer::SetOutlinePosition(const Camera& cam,
                                                         Rect region) {
  glm::vec2 inner_size{
      cam.ConvertDistance(1, DistanceType::kDp, DistanceType::kWorld)};
  glm::vec2 outer_size{
      cam.ConvertDistance(4, DistanceType::kDp, DistanceType::kWorld)};
  outline_.SetSizeAndPosition(region, inner_size, true);

  // set the outer position as centered on the inner position
  outline_glow_.SetSizeAndPosition(
      region.Inset(-(outer_size - inner_size) * 0.5f), outer_size, true);
}

void ElementManipulationToolRenderer::Update(const Camera& cam,
                                             FrameTimeS draw_time,
                                             Rect element_mbr,
                                             glm::mat4 transform) {
  UpdateWithTimer(cam, draw_time, element_mbr, transform,
                  Timer(wall_clock_, 0.004));
}

void ElementManipulationToolRenderer::UpdateWithTimer(const Camera& cam,
                                                      FrameTimeS draw_time,
                                                      Rect element_mbr,
                                                      glm::mat4 transform,
                                                      Timer timer) {
  SetOutlinePosition(cam, geometry::Transform(element_mbr, transform));

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
  gl_resources_->mesh_vbo_provider->ReplaceVBO(&bg_overlay_, GL_DYNAMIC_DRAW);
}

void ElementManipulationToolRenderer::Enable(bool enabled) {
  outline_.SetVisible(enabled);
  outline_glow_.SetVisible(enabled);
  if (enabled) {
    partition_renderer_.EnableFramerateLocks(frame_state_);
  } else {
    partition_renderer_.DisableFramerateLocks();
  }
}

void ElementManipulationToolRenderer::Synchronize(void) {
  renderer_->Synchronize(frame_state_->GetLastFrameTime());
}

void ElementManipulationToolRenderer::SetElements(
    const Camera& cam, const std::vector<ElementId>& elements,
    Rect element_mbr) {
  if (partition_renderer_.RenderingSize() != cam.ScreenDim()) {
    partition_renderer_.Resize(cam.ScreenDim());
  }
  partition_renderer_.AssignPartitionData(
      PartitionData(partition_renderer_.CurrentPartition() + 1,
                    scene_graph_->GroupifyElements(elements)));

  Timer timer(wall_clock_, 2);
  UpdateWithTimer(cam, frame_state_->GetFrameTime(), element_mbr, glm::mat4{1},
                  timer);
  if (timer.Expired()) {
    SLOG(SLOG_WARNING, "time expired while attempting to select elements");
  }
  outline_glow_.SetVisible(true);
  outline_.SetVisible(true);

  if (!elements.empty()) {
    renderer_->Synchronize(frame_state_->GetFrameTime());
  }
}

}  // namespace tools
}  // namespace ink
