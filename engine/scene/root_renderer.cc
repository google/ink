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

#include "ink/engine/scene/root_renderer.h"

#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/gl.h"
#include "ink/engine/realtime/line_tool.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

using std::shared_ptr;

RootRendererImpl::RootRendererImpl(
    shared_ptr<GLResourceManager> gl_resources, shared_ptr<Camera> camera,
    shared_ptr<IPlatform> platform, shared_ptr<PageBounds> page_bounds,
    shared_ptr<ToolController> tools, shared_ptr<LiveRenderer> graph_renderer,
    shared_ptr<GridManager> grid_manager,
    shared_ptr<ParticleManager> particle_manager,
    shared_ptr<PageBorder> page_border, shared_ptr<IDbgHelper> dbg_helper,
    shared_ptr<FrameState> frame_state, shared_ptr<settings::Flags> flags)
    : gl_resources_(std::move(gl_resources)),
      gl_(gl_resources_->gl),
      camera_(std::move(camera)),
      platform_(std::move(platform)),
      page_bounds_(std::move(page_bounds)),
      tools_(std::move(tools)),
      graph_renderer_(std::move(graph_renderer)),
      grid_manager_(std::move(grid_manager)),
      particle_manager_(std::move(particle_manager)),
      page_border_(std::move(page_border)),
      dbg_helper_(std::move(dbg_helper)),
      frame_state_(std::move(frame_state)),
      flags_(std::move(flags)),
      drawable_dispatch_(std::make_shared<EventDispatch<DrawListener>>()),
      background_renderer_(gl_resources_, page_bounds) {
  flags_->AddListener(this);
}

void RootRendererImpl::BindScreen() {
  SLOG(SLOG_GL_STATE, "RootRendererImpl binding read/write to screen fbo");
  platform_->BindScreen();

  auto screen_dim = camera_->ScreenDim();
  SLOG(SLOG_GL_STATE, "RootRendererImpl setting glViewport to $0", screen_dim);
  gl_->Viewport(0, 0, screen_dim.x, screen_dim.y);

  GLASSERT_NO_ERROR(gl_);
}

// The specific order of draw operations in the following draw methods should be
// kept in sync with ImageExporter::Render.
//
void RootRendererImpl::Draw(FrameTimeS draw_time) {
  SLOG(SLOG_DRAWING, "draw to screen started");

  gl_->Enable(GL_BLEND);
  gl_->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // If we're double-buffering, use the back buffer instead of the screen to
  // respect the rotation.
  if (back_buffer_) {
    back_buffer_->BindBack();
  } else {
    BindScreen();
  }

  bool only_drawing_line_tool = OnlyDrawingLineTool();

  if (!only_drawing_line_tool) {
    const auto& oob = gl_resources_->background_state->GetOutOfBoundsColor();
    gl_->ClearColor(oob.r, oob.g, oob.b, oob.a);
    gl_->Clear(GL_COLOR_BUFFER_BIT);

    DrawDrawables(draw_time, RenderOrder::Start);
  }

  DrawPageContents(draw_time);

  dbg_helper_->Draw(*camera_, draw_time);

  if (!only_drawing_line_tool) {
    page_border_->Draw(*camera_, draw_time);

    const Tool* tool = tools_->EnabledTool();
    if (tool) {
      tool->AfterSceneDrawn(*camera_, draw_time);
    }

    DrawDrawables(draw_time, RenderOrder::End);
  }

  // If we drew into the back buffer, rotate and respect the screen orientation
  // and dimension mismatch with respect to the world.
  //
  // See rotationHandler in
  // //ink/public/js/sketchology_engine_wrapper.js
  if (back_buffer_) {
    Rect box({0, 0}, camera_->ScreenDim());
    if (only_drawing_line_tool) {
      OptRect updated_region = GetDrawingBounds();
      if (!updated_region.has_value()) {
        return;  // Nothing to draw.
      }
      box = updated_region.value();
    }
    back_buffer_->BlitBackToFront(box);
    BindScreen();
    glm::ivec2 screen_dim = camera_->ScreenDim();
    if (camera_->ScreenRotation() % 180 == 90) {
      gl_->Viewport(0, 0, screen_dim.y, screen_dim.x);
    } else {
      gl_->Viewport(0, 0, screen_dim.x, screen_dim.y);
    }
    Camera flip_camera;
    flip_camera.SetScreenDim(screen_dim);
    glm::vec2 size{back_buffer_->GetSize()};
    flip_camera.SetPosition({size.x / 2, size.y / 2}, size, 0);
    flip_camera.RotateWorldToDevice(
        static_cast<float>(camera_->ScreenRotation()));
    back_buffer_->DrawFront(flip_camera, blit_attrs::Blit(), RotRect(box),
                            RotRect(box));
  }

  if (only_drawing_line_tool) {
    LineTool* line_tool;
    if (tools_->GetTool(Tools::ToolType::Line, &line_tool)) {
      line_tool->ResetUpdatedRegion();
    }
  }
}

void RootRendererImpl::DrawPageContents(FrameTimeS draw_time) {
  Scissor scissor_state(gl_, Scissor::Parent::IGNORE);
  OptRect scissor_box = GetDrawingBounds();
  if (scissor_box.has_value()) {
    if (dbg_helper_->PartialDrawRectsEnabled() && OnlyDrawingLineTool()) {
      dbg_helper_->AddRect(
          geometry::Transform(scissor_box.value(), camera_->ScreenToWorld()),
          {1, 0, 0, 0.5},  // Color
          false,           // Fill
          72998);          // Unique ID chosen at random
    }
    scissor_state.SetScissor(*camera_, scissor_box.value(), CoordType::kScreen);
  } else {
    Scissor::SetScissorEnabled(gl_, false);
  }

  DrawDrawables(draw_time, RenderOrder::PreBackground);

  background_renderer_.Draw(*camera_, draw_time);
  grid_manager_->Draw(*camera_, draw_time);

  const Tool* tool = tools_->EnabledTool();
  if (tool) {
    tool->BeforeSceneDrawn(*camera_, draw_time);
  }

  DrawDrawables(draw_time, RenderOrder::PreScene);

  graph_renderer_->Draw(*camera_, draw_time);

  DrawDrawables(draw_time, RenderOrder::PreTool);

  if (tool) {
    SLOG(SLOG_DRAWING, "drawing tool");
    tool->Draw(*camera_, draw_time);
  }

  graph_renderer_->DrawAfterTool(*camera_, draw_time);

  DrawDrawables(draw_time, RenderOrder::PostTool);
  particle_manager_->Draw(*camera_, draw_time);
}

void RootRendererImpl::DrawDrawables(FrameTimeS draw_time,
                                     RenderOrder which) const {
  drawable_dispatch_->Send(&DrawListener::Draw, which, *camera_, draw_time);
}

void RootRendererImpl::AddDrawable(DrawListener* drawable) {
  ASSERT(drawable != nullptr);
  SLOG(SLOG_DATA_FLOW, "root adding drawable $0", AddressStr(drawable));
  drawable->RegisterOnDispatch(drawable_dispatch_);
}

void RootRendererImpl::RemoveDrawable(DrawListener* drawable) {
  ASSERT(drawable);
  SLOG(SLOG_DATA_FLOW, "removing drawable $0", AddressStr(drawable));
  drawable->Unregister(drawable_dispatch_);
}

void RootRendererImpl::Resize(glm::ivec2 new_size, int rotation_deg) {
  // Always use a back buffer for partial draw for anti-aliasing support.
  // Otherwise only allocate a back buffer if the screen is rotated.
  //
  // We have to disable anti-aliasing on the screen context when doing partial
  // updates.  See crbug/919909 for details.
  bool needs_back_buffer = partial_draw_enabled_ || rotation_deg > 0;

  if (needs_back_buffer && !back_buffer_) {
    back_buffer_ = absl::make_unique<DBRenderTarget>(
        std::make_shared<WallClock>(), gl_resources_);
    back_buffer_->Resize(camera_->ScreenDim());
  } else if (!needs_back_buffer && back_buffer_) {
    back_buffer_.reset();
  }

  if (back_buffer_) {
    back_buffer_->Resize(new_size);
  }
}

void RootRendererImpl::OnFlagChanged(settings::Flag which, bool new_value) {
  if (which == settings::Flag::EnablePartialDraw) {
    partial_draw_enabled_ = new_value;
    // Allocate or de-allocate a back buffer if needed.
    Resize(camera_->ScreenDim(), camera_->ScreenRotation());
  }
}

bool RootRendererImpl::OnlyDrawingLineTool() const {
  LineTool* line_tool;
  if (!tools_->GetTool(Tools::ToolType::Line, &line_tool)) {
    return false;
  }
  return partial_draw_enabled_ &&  // The partial draw flag must be set.
                                   // The line tool must be the active tool.
         tools_->ChosenToolType() == Tools::ToolType::Line &&
         // The line tool must have an updated region to draw.
         line_tool->UpdatedRegion().has_value() &&
         // If there are more frame locks held than just the one in input
         // dispatch, we must update the entire screen because other geometry
         // might be changing, e.g. animations or stroke finalization.
         frame_state_->FrameLockCount() == 1;
}

OptRect RootRendererImpl::GetDrawingBounds() const {
  if (OnlyDrawingLineTool()) {
    LineTool* line_tool;
    if (!tools_->GetTool(Tools::ToolType::Line, &line_tool)) {
      return absl::nullopt;
    }
    OptRect updated_region = line_tool->UpdatedRegion();
    if (!updated_region.has_value()) {
      return absl::nullopt;
    }
    Rect box = updated_region.value();
    box = box.Inset(-3);  // Capture any anti-aliasing artifacts.
    if (page_bounds_->HasBounds()) {
      Rect screen_bounds =
          geometry::Transform(page_bounds_->Bounds(), camera_->WorldToScreen());
      // Don't write over anti-aliasing artifacts from the page border.
      screen_bounds = screen_bounds.Inset(3);
      geometry::Intersection(box, screen_bounds, &box);
    }
    return box;
  } else if (page_bounds_->HasBounds()) {
    return geometry::Transform(page_bounds_->Bounds(),
                               camera_->WorldToScreen());
  } else {
    return absl::nullopt;
  }
}

}  // namespace ink
