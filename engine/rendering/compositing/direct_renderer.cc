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

#include "ink/engine/rendering/compositing/direct_renderer.h"

#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

DirectRenderer::DirectRenderer(std::shared_ptr<SceneGraph> scene_graph,
                               std::shared_ptr<FrameState> frame_state,
                               std::shared_ptr<GLResourceManager> gl_resources,
                               std::shared_ptr<LayerManager> layer_manager)
    : scene_graph_(std::move(scene_graph)),
      frame_state_(std::move(frame_state)),
      gl_resources_(gl_resources),
      layer_manager_(layer_manager),
      element_renderer_(gl_resources) {
  scene_graph_->AddListener(this);
}

DirectRenderer::~DirectRenderer() { scene_graph_->RemoveListener(this); }

void DirectRenderer::Draw(const Camera& cam, FrameTimeS draw_time) const {
  RegionQuery query = RegionQuery::MakeCameraQuery(cam);
  elements_on_screen_ = scene_graph_->ElementsInRegionByGroup(query);

  auto begin = elements_on_screen_.begin();
  auto end = elements_on_screen_.end();

  // Set the post_tool_iterator to end, assuming that everything needs to be
  // rendered pre-tool.
  post_tool_iterator_ = end;

  // If the layer manager is active, then set the post tool iterator to the
  // first layer (group) after the active layer.
  if (layer_manager_->IsActive()) {
    auto active_layer_or = layer_manager_->IndexOfActiveLayer();
    if (active_layer_or.ok()) {
      size_t active_layer = active_layer_or.ValueOrDie();
      post_tool_iterator_ = std::stable_partition(
          begin, end,
          [this, active_layer](const SceneGraph::GroupedElements& item) {
            auto index_or =
                layer_manager_->IndexForLayerWithGroupId(item.group_id);
            if (index_or.ok()) {
              return index_or.ValueOrDie() <= active_layer;
            } else {
              SLOG(SLOG_ERROR,
                   "SceneGraph rendering layer unknown to LayerManager, $0",
                   item.group_id);
            }
            return true;
          });
    }
  }

  DrawRange(cam, draw_time, begin, post_tool_iterator_);

  for (auto& d : scene_graph_->GetDrawables()) {
    d->Draw(cam, draw_time);
  }
}

void DirectRenderer::DrawAfterTool(const Camera& cam,
                                   FrameTimeS draw_time) const {
  if (post_tool_iterator_ != elements_on_screen_.end()) {
    DrawRange(cam, draw_time, post_tool_iterator_, elements_on_screen_.end());
  }
}

void DirectRenderer::DrawRange(
    const Camera& cam, FrameTimeS draw_time,
    SceneGraph::GroupedElementsList::const_iterator begin,
    SceneGraph::GroupedElementsList::const_iterator end) const {
  // Assume that the range is sorted such that elements are in the
  // appropriate z-order and any GROUPs found will force a scissor.
  for (auto element_group = begin; element_group < end; element_group++) {
    SLOG(SLOG_DRAWING, "Drawing group $0", element_group->group_id);
    std::unique_ptr<Scissor> scissor;
    if (element_group->bounds.Area() != 0) {
      SLOG(SLOG_DRAWING, "  Scissoring to $0", element_group->bounds);
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(cam, element_group->bounds, CoordType::kWorld);
    }
    for (ElementId poly_id : element_group->poly_ids) {
      SLOG(SLOG_DRAWING, "    Drawing element $0", poly_id);
      if (!element_renderer_.Draw(poly_id, *scene_graph_, cam, draw_time)) {
        SLOG(SLOG_WARNING, "    FAILED to draw element $0", poly_id);
      }
    }
  }
}

void DirectRenderer::OnElementAdded(SceneGraph* graph, ElementId id) {
  frame_state_->RequestFrame();
}

void DirectRenderer::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  frame_state_->RequestFrame();
}

void DirectRenderer::OnElementsMutated(
    SceneGraph* graph, const std::vector<ElementMutationData>& mutation_data) {
  frame_state_->RequestFrame();
}

}  // namespace ink
