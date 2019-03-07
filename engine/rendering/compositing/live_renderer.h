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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_LIVE_RENDERER_H_
#define INK_ENGINE_RENDERING_COMPOSITING_LIVE_RENDERER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/includes/glm/third_party/glm/glm/detail/type_vec.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/rendering/compositing/direct_renderer.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/compositing/triple_buffered_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/strategy/rendering_strategy.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

// The LiveRenderer is the SceneGraphRenderer used by SEngine during
// interaction, i.e., the renderer seen by the user, as opposed to special
// purpose renderers such as those used by the bitmap export process or other
// texture manipulation use cases. It delegates to either a DirectRenderer or a
// TripleBufferedRenderer, switchable via the
//   Use(RenderingStrategy rendering_strategy)
// method. It defaults to TripleBufferedRenderer.
class LiveRenderer : public SceneGraphRenderer {
 public:
  using SharedDeps =
      service::Dependencies<SceneGraph, FrameState, GLResourceManager,
                            LayerManager, input::InputDispatch,
                            WallClockInterface, PageManager, settings::Flags>;

  LiveRenderer(std::shared_ptr<SceneGraph> scene_graph,
               std::shared_ptr<FrameState> frame_state,
               std::shared_ptr<GLResourceManager> gl_resources,
               std::shared_ptr<LayerManager> layer_manager,
               std::shared_ptr<input::InputDispatch> input_dispatch,
               std::shared_ptr<WallClockInterface> wall_clock,
               std::shared_ptr<PageManager> page_manager,
               std::shared_ptr<settings::Flags> flags);
  ~LiveRenderer() override;

  void Use(RenderingStrategy rendering_strategy);

  // SceneGraphRenderer
  void Update(const Timer& timer, const Camera& cam,
              FrameTimeS draw_time) override;
  void Resize(glm::ivec2 size) override;
  glm::ivec2 RenderingSize() const override;
  void DrawAfterTool(const Camera& cam, FrameTimeS draw_time) const override;
  void Invalidate() override;
  void Synchronize(FrameTimeS draw_time) override;

  // IDrawable
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

  // LiveRenderer is neither copyable nor movable.
  LiveRenderer(const LiveRenderer&) = delete;
  LiveRenderer& operator=(const LiveRenderer&) = delete;

 private:
  SceneGraphRenderer* delegate() const;

  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<LayerManager> layer_manager_;
  std::shared_ptr<input::InputDispatch> input_dispatch_;
  std::shared_ptr<WallClockInterface> wall_clock_;
  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<settings::Flags> flags_;

  mutable std::unique_ptr<SceneGraphRenderer> delegate_;
  RenderingStrategy strategy_{RenderingStrategy::kBufferedRenderer};
  glm::vec2 cached_size_{0, 0};
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_COMPOSITING_LIVE_RENDERER_H_
