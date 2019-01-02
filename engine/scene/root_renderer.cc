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
    shared_ptr<PageBorder> page_border, shared_ptr<IDbgHelper> dbg_helper)
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
      drawable_dispatch_(std::make_shared<EventDispatch<DrawListener>>()),
      background_renderer_(gl_resources_, page_bounds) {}

void RootRendererImpl::BindScreen() {
  SLOG(SLOG_GL_STATE, "RootRendererImpl binding read/write to screen fbo");
  platform_->BindScreen();

  auto screen_dim = camera_->ScreenDim();
  SLOG(SLOG_GL_STATE, "RootRendererImpl setting glViewport to $0", screen_dim);
  gl_->Viewport(0, 0, screen_dim.x, screen_dim.y);

  GLASSERT_NO_ERROR(gl_);
}

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

  const auto& oob = gl_resources_->background_state->GetOutOfBoundsColor();
  gl_->ClearColor(oob.r, oob.g, oob.b, oob.a);
  gl_->Clear(GL_COLOR_BUFFER_BIT);

  DrawDrawables(draw_time, RenderOrder::Start);

  DrawPageContents(draw_time);

  dbg_helper_->Draw(*camera_, draw_time);
  page_border_->Draw(*camera_, draw_time);

  const Tool* tool = tools_->EnabledTool();
  if (tool) {
    tool->AfterSceneDrawn(*camera_, draw_time);
  }

  DrawDrawables(draw_time, RenderOrder::End);

  // If we drew into the back buffer, rotate and respect the screen orientation
  // and dimension mismatch with respect to the world.
  //
  // See rotationHandler in
  // //ink/public/js/sketchology_engine_wrapper.js
  if (back_buffer_) {
    back_buffer_->BlitBackToFront();
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
    back_buffer_->DrawFront(flip_camera, blit_attrs::Blit());
  }
}

void RootRendererImpl::DrawPageContents(FrameTimeS draw_time) {
  Scissor scissor_state(gl_, Scissor::Parent::IGNORE);
  if (page_bounds_->HasBounds()) {
    scissor_state.SetScissor(
        *camera_,
        geometry::Transform(page_bounds_->Bounds(), camera_->WorldToScreen()),
        CoordType::kScreen);
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
  if (rotation_deg > 0 && !back_buffer_) {
    back_buffer_ = absl::make_unique<DBRenderTarget>(
        std::make_shared<WallClock>(), gl_resources_);
  } else if (rotation_deg == 0 && back_buffer_) {
    back_buffer_.release();
  }
  if (back_buffer_) {
    back_buffer_->Resize(new_size);
  }
}

}  // namespace ink
