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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_DIRECT_RENDERER_H_
#define INK_ENGINE_RENDERING_COMPOSITING_DIRECT_RENDERER_H_

#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/element_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"

namespace ink {

// Draws directly to the bound buffer (e.g. the screen).
class DirectRenderer : public SceneGraphRenderer, public SceneGraphListener {
 public:
  using SharedDeps = service::Dependencies<SceneGraph, FrameState,
                                           GLResourceManager, LayerManager>;

  DirectRenderer(std::shared_ptr<SceneGraph> scene_graph,
                 std::shared_ptr<FrameState> frame_state,
                 std::shared_ptr<GLResourceManager> gl_resources,
                 std::shared_ptr<LayerManager> layer_manager);
  ~DirectRenderer() override;

  void Update(const Timer& timer, const Camera& cam,
              FrameTimeS draw_time) override {}

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void DrawAfterTool(const Camera& cam, FrameTimeS draw_time) const override;

  void Resize(glm::ivec2 size) override { size_ = size; }
  glm::ivec2 RenderingSize() const override { return size_; }

  void Invalidate() override {}

  void Synchronize(FrameTimeS draw_time) override {}

 private:
  void DrawRange(const Camera& cam, FrameTimeS draw_time,
                 SceneGraph::GroupedElementsList::const_iterator begin,
                 SceneGraph::GroupedElementsList::const_iterator end) const;

  void OnElementAdded(SceneGraph* graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override;

  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<LayerManager> layer_manager_;
  ElementRenderer element_renderer_;
  glm::ivec2 size_{0, 0};

  // Elements organized by group to be rendered either before or after the tool.
  // See post_tool_iterator.
  mutable SceneGraph::GroupedElementsList elements_on_screen_;

  // Divides elements_on_screen_ into a partition. Everything before this
  // should be rendered pre tool rendering. Everything after should be rendered
  // post tool rendering. If elements_on_screen_ changes, this value is
  // invalidated.
  mutable SceneGraph::GroupedElementsList::const_iterator post_tool_iterator_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_COMPOSITING_DIRECT_RENDERER_H_
