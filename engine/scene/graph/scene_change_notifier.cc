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

#include "ink/engine/scene/graph/scene_change_notifier.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

SceneChangeNotifier::SceneChangeNotifier(
    std::shared_ptr<ISceneChangeListener> listener,
    std::shared_ptr<LayerManager> layer_manager) {
  dispatch_ = listener;
  layer_manager_ = layer_manager;
}

void SceneChangeNotifier::OnElementAdded(SceneGraph* graph, ElementId id) {
  if (!enabled_) return;
  proto::scene_change::SceneChangeEvent event;
  UUID uuid = graph->UUIDFromElementId(id);
  if (id.Type() == GROUP && layer_manager_->IsActive()) {
    int opacity = graph->Opacity(id);
    bool visible = graph->Visible(id);

    event.mutable_layer_added()->set_uuid(uuid);
    event.mutable_layer_added()->set_opacity(opacity);
    event.mutable_layer_added()->set_visible(visible);
    event.mutable_layer_added()->set_below_uuid(LayerAbove(id, graph));
  } else {
    event.mutable_element_added()->set_uuid(uuid);
    const auto parent_id = graph->GetParentGroupId(id);
    if (parent_id != kInvalidElementId) {
      UUID parent = graph->UUIDFromElementId(parent_id);
      event.mutable_element_added()->set_layer_uuid(parent);
    }
  }
  dispatch_->SceneChanged(event);
}

void SceneChangeNotifier::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  if (!enabled_) return;
  for (auto removed : removed_elements) {
    proto::scene_change::SceneChangeEvent event;
    if (layer_manager_->IsActive() && removed.id.Type() == GROUP) {
      event.mutable_layer_removed()->set_uuid(removed.uuid);
    } else {
      event.mutable_element_removed()->set_uuid(removed.uuid);
      event.mutable_element_removed()->set_layer_uuid(removed.parent);
    }
    dispatch_->SceneChanged(event);
  }
}

void SceneChangeNotifier::OnElementsMutated(
    SceneGraph* graph, const std::vector<ElementMutationData>& mutation_data) {
  if (!enabled_) return;
  for (auto i : mutation_data) {
    if (layer_manager_->IsActive() &&
        i.original_element_data.id.Type() == GROUP) {
      if (i.mutation_type == ElementMutationType::kVisibilityMutation) {
        proto::scene_change::SceneChangeEvent event;
        event.mutable_visibility_updated()->set_uuid(
            i.original_element_data.uuid);
        event.mutable_visibility_updated()->set_visible(
            i.modified_element_data.visible);
        dispatch_->SceneChanged(event);
      } else if (i.mutation_type == ElementMutationType::kOpacityMutation) {
        proto::scene_change::SceneChangeEvent event;
        event.mutable_opacity_updated()->set_uuid(i.original_element_data.uuid);
        event.mutable_opacity_updated()->set_opacity(
            i.modified_element_data.opacity);
        dispatch_->SceneChanged(event);
      } else if (i.mutation_type == ElementMutationType::kZOrderMutation) {
        proto::scene_change::SceneChangeEvent event;
        event.mutable_order_updated()->set_uuid(i.original_element_data.uuid);
        event.mutable_order_updated()->set_below_uuid(
            LayerAbove(i.original_element_data.id, graph));
        dispatch_->SceneChanged(event);
      }
    } else {
      // Only care about transform changes for now.
      if (i.mutation_type == ElementMutationType::kTransformMutation) {
        proto::scene_change::SceneChangeEvent event;
        event.mutable_element_modified()->set_uuid(
            i.original_element_data.uuid);
        if (i.original_element_data.group_id != kInvalidElementId) {
          UUID parent =
              graph->UUIDFromElementId(i.original_element_data.group_id);
          event.mutable_element_modified()->set_layer_uuid(parent);
        }
        dispatch_->SceneChanged(event);
      }
    }
  }
}

void SceneChangeNotifier::ActiveLayerChanged(
    const UUID& uuid, const proto::SourceDetails& sourceDetails) {
  if (!enabled_) return;
  proto::scene_change::SceneChangeEvent event;
  event.mutable_active_layer_updated()->set_active_uuid(uuid);
  dispatch_->SceneChanged(event);
}

UUID SceneChangeNotifier::LayerAbove(ink::GroupId id, SceneGraph* graph) {
  auto index_or = layer_manager_->IndexForLayerWithGroupId(id);
  if (!index_or.ok()) return kInvalidUUID;

  auto below_group_or =
      layer_manager_->GroupIdForLayerAtIndex(index_or.ValueOrDie() + 1);
  if (!below_group_or.ok()) return kInvalidUUID;

  return graph->UUIDFromElementId(below_group_or.ValueOrDie());
}

}  // namespace ink
