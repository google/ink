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

#ifndef INK_ENGINE_SCENE_ROOT_RENDERER_H_
#define INK_ENGINE_SCENE_ROOT_RENDERER_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/realtime/tool_controller.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/rendering/renderers/background_renderer.h"
#include "ink/engine/scene/grid_manager.h"
#include "ink/engine/scene/page/page_border.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/particle_manager.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg_helper.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class RootRenderer {
 public:
  enum class RenderOrder {
    Start,
    PreBackground,
    PreScene,
    PreTool,
    PostTool,
    End,
  };

  class DrawListener : public EventListener<DrawListener> {
   public:
    // This will be called n times per frame, where n == number of classes in
    // Render Order. Each order will get exactly one call to draw per frame.
    // You probably only want to draw in response to one of these calls!
    //
    // Example:
    // Frame 1
    //    Draw(RenderOrder::Start, cam, t1);
    //    Draw(RenderOrder::PreBackground, cam, t1);
    //    Draw(RenderOrder::PreScene, cam, t1);
    //    Draw(RenderOrder::PreTool, cam, t1);
    //    Draw(RenderOrder::PostTool, cam, t1);
    //    Draw(RenderOrder::End, cam, t1);
    // Frame 2
    //    Draw(RenderOrder::Start, cam, time_2);
    //    Draw(RenderOrder::PreBackground, cam, time_2);
    //    Draw(RenderOrder::PreScene, cam, time_2);
    //    Draw(RenderOrder::PreTool, cam, time_2);
    //    Draw(RenderOrder::PostTool, cam, time_2);
    //    Draw(RenderOrder::End, cam, time_2);
    virtual void Draw(RenderOrder at_order, const Camera& cam,
                      FrameTimeS t) const = 0;
  };

  virtual ~RootRenderer() {}
  virtual void BindScreen() = 0;
  virtual void Draw(FrameTimeS draw_time) = 0;

  virtual void AddDrawable(DrawListener* drawable) = 0;
  virtual void RemoveDrawable(DrawListener* drawable) = 0;

  virtual void Resize(glm::ivec2 new_size, int rotation_deg) = 0;
};

class RootRendererImpl : public RootRenderer {
 public:
  using SharedDeps =
      service::Dependencies<GLResourceManager, Camera, IPlatform, PageBounds,
                            ToolController, LiveRenderer, GridManager,
                            ParticleManager, PageBorder, IDbgHelper>;

  RootRendererImpl(std::shared_ptr<GLResourceManager> gl_resources,
                   std::shared_ptr<Camera> camera,
                   std::shared_ptr<IPlatform> platform,
                   std::shared_ptr<PageBounds> page_bounds,
                   std::shared_ptr<ToolController> tools,
                   std::shared_ptr<LiveRenderer> graph_renderer,
                   std::shared_ptr<GridManager> grid_manager,
                   std::shared_ptr<ParticleManager> particle_manager,
                   std::shared_ptr<PageBorder> page_border,
                   std::shared_ptr<IDbgHelper> dbg_helper);

  // Disallow copy and assign.
  RootRendererImpl(const RootRendererImpl&) = delete;
  RootRendererImpl& operator=(const RootRendererImpl&) = delete;

  void BindScreen() override;
  void Draw(FrameTimeS draw_time) override;

  void AddDrawable(DrawListener* drawable) override;
  void RemoveDrawable(DrawListener* drawable) override;

  void Resize(glm::ivec2 new_size, int rotation_deg) override;

 private:
  void DrawPageContents(FrameTimeS draw_time);
  void DrawDrawables(FrameTimeS draw_time, RenderOrder which) const;
  std::shared_ptr<GLResourceManager> gl_resources_;
  ion::gfx::GraphicsManagerPtr gl_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<ToolController> tools_;
  std::shared_ptr<LiveRenderer> graph_renderer_;
  std::shared_ptr<GridManager> grid_manager_;
  std::shared_ptr<ParticleManager> particle_manager_;
  std::shared_ptr<PageBorder> page_border_;
  std::shared_ptr<IDbgHelper> dbg_helper_;

  std::shared_ptr<EventDispatch<DrawListener>> drawable_dispatch_;
  BackgroundRenderer background_renderer_;

  std::unique_ptr<DBRenderTarget> back_buffer_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_ROOT_RENDERER_H_
