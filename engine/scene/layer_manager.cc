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

#include "ink/engine/scene/layer_manager.h"

#include <algorithm>

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

StatusOr<GroupId> LayerManager::AddLayer(const SourceDetails &source_details) {
  UUID uuid = scene_graph_->GenerateUUID();
  GroupId group_id;
  if (!scene_graph_->GetNextGroupId(uuid, &group_id)) {
    return ErrorStatus(StatusCode::INTERNAL, "Cannot get next group id");
  }

  // Layers are a one-way street. After adding a layer, they are always active.
  bool became_active = false;
  if (!active_) {
    active_ = true;
    became_active = true;
  }

  // Pass an empty Rect when adding the group. This ensures that it
  // will always be rendered (subject to Visibility).
  //
  scene_graph_->AddOrUpdateGroup(group_id, glm::mat4{1}, Rect(0, 0, 0, 0),
                                 false, kLayerGroupType, source_details);

  InvalidateLayerListCache();

  if (became_active) {
    // Layer 0 is always the first active layer.
    SetActiveLayer(0);
  }

  return group_id;
}

Status LayerManager::MoveLayer(size_t from_index, size_t to_index) {
  if (from_index == to_index) {
    return OkStatus();
  }

  const auto &layer_ids = CachedLayerList();

  if (from_index >= layer_ids.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "from_index, $0, out of range",
                       from_index);
  }
  if (to_index >= layer_ids.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "to_index, $0, out of range",
                       to_index);
  }

  // The scene graph refers to groups by id, not layer index.
  GroupId group_to_move = layer_ids[from_index];
  GroupId move_before_group = kInvalidElementId;
  if (to_index < layer_ids.size() - 1) {
    move_before_group = layer_ids[to_index + (from_index < to_index ? 1 : 0)];
  }

  // Remember the active group id
  auto active_group_id = layer_ids[active_layer_index_];

  // We have to also maintain our local list of group ids.
  scene_graph_->MoveGroup(group_to_move, move_before_group);
  InvalidateLayerListCache();

  // Update the active layer if necessary.
  const auto &new_layer_ids = CachedLayerList();
  if (new_layer_ids[active_layer_index_] != active_group_id) {
    SetActiveLayer(FindGroupWithId(new_layer_ids, active_group_id) -
                   new_layer_ids.begin());
  }
  return OkStatus();
}

Status LayerManager::RemoveLayer(size_t index,
                                 const SourceDetails &source_details) {
  const auto &layer_ids = CachedLayerList();

  if (layer_ids.size() == 1) {
    return ErrorStatus(StatusCode::FAILED_PRECONDITION,
                       "You cannot remove the last layer.");
  }

  if (index >= layer_ids.size()) return OkStatus();

  // Update the active layer in case its index changes.
  if (index == active_layer_index_) {
    active_layer_index_ = 0;
  } else if (index < active_layer_index_) {
    active_layer_index_--;
  }

  scene_graph_->RemoveElement(layer_ids[index], source_details);
  InvalidateLayerListCache();
  InformActiveLayerListener(source_details);
  return OkStatus();
}

Status LayerManager::SetActiveLayer(size_t index,
                                    const SourceDetails &source_details) {
  const auto &layer_ids = CachedLayerList();

  if (index >= layer_ids.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "Index, $0, out of range.",
                       index);
  }

  active_layer_index_ = index;
  // Only notify if this came from the engine.
  InformActiveLayerListener(source_details);

  return OkStatus();
}

StatusOr<size_t> LayerManager::IndexOfActiveLayer() const {
  if (!IsActive()) {
    return ErrorStatus(StatusCode::FAILED_PRECONDITION,
                       "There is no active layer.");
  }
  return active_layer_index_;
}

StatusOr<GroupId> LayerManager::GroupIdOfActiveLayer() const {
  INK_ASSIGN_OR_RETURN(auto index, IndexOfActiveLayer());
  return GroupIdForLayerAtIndex(index);
}

bool LayerManager::HasTransparency() const {
  if (!IsActive()) return false;

  if (!cached_has_transparency_.has_value()) {
    auto layer_list = CachedLayerList();
    cached_has_transparency_ = std::any_of(
        layer_list.begin(), layer_list.end(), [this](GroupId group_id) {
          return scene_graph_->Opacity(group_id) != 255;
        });
  }

  return *cached_has_transparency_;
}

StatusOr<GroupId> LayerManager::GroupIdForLayerAtIndex(size_t index) const {
  const auto &layer_ids = CachedLayerList();

  if (index >= layer_ids.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "Index, $0, out of range.",
                       index);
  }

  return layer_ids[index];
}

LayerManager::LayerList::const_iterator LayerManager::FindGroupWithId(
    const LayerList &layer_ids, const GroupId &group_id) const {
  return std::find_if(
      layer_ids.begin(), layer_ids.end(),
      [group_id](const GroupId &gid) { return gid == group_id; });
}

StatusOr<size_t> LayerManager::IndexForLayerWithGroupId(
    GroupId group_id) const {
  const auto &layer_ids = CachedLayerList();

  auto i = FindGroupWithId(layer_ids, group_id);
  if (i == layer_ids.end()) {
    return ErrorStatus(StatusCode::NOT_FOUND,
                       "Index with group_id, $0, not found.", group_id);
  }
  return i - layer_ids.begin();
}

Status LayerManager::SetLayerVisibility(size_t index, bool visible) {
  const auto &layer_ids = CachedLayerList();

  if (index >= layer_ids.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "Index, $0, out of range.",
                       index);
  }

  scene_graph_->SetElementRenderedByMain(layer_ids[index], visible);
  return OkStatus();
}

bool LayerManager::IsActiveLayerTopmost() const {
  if (!IsActive()) {
    return true;  // Active line is always topmost if layers aren't active.
  }
  const auto &layer_ids = CachedLayerList();

  return active_layer_index_ == layer_ids.size() - 1;
}

void LayerManager::Reset() {
  InvalidateLayerListCache();
  last_active_uuid_sent_ = kInvalidUUID;
  active_layer_index_ = std::numeric_limits<size_t>::max();
  active_ = false;
}

void LayerManager::InvalidateLayerListCache() {
  cached_layer_list_.clear();

  // The has transparency cache deps directly on the layer list.
  InvalidateHasTransparencyCache();
}

const LayerManager::LayerList &LayerManager::CachedLayerList() const {
  if (IsActive() && cached_layer_list_.empty()) {
    scene_graph_->GroupChildrenOfRoot(std::back_inserter(cached_layer_list_));
  }
  return cached_layer_list_;
}

//
// SceneGraphListener
//
void LayerManager::OnElementAdded(SceneGraph *graph, ElementId id) {
  if (id.Type() == ElementType::GROUP) {
    // If the layer manager is not active, and a group is added,
    // then it must become active iff that group is a layer.
    if (!IsActive()) {
      auto metadata = scene_graph_->GetElementMetadata(id);
      if (metadata.attributes.group_type == kLayerGroupType) {
        active_ = true;
      }
    }

    if (IsActive()) {
      InvalidateLayerListCache();
    }
  }
}

void LayerManager::OnElementsRemoved(
    SceneGraph *graph, const std::vector<SceneGraphRemoval> &removed_elements) {
  if (IsActive() &&
      std::any_of(removed_elements.begin(), removed_elements.end(),
                  [](const SceneGraphRemoval &removal) {
                    return removal.id.Type() == ElementType::GROUP;
                  })) {
    InvalidateLayerListCache();
  }
}

void LayerManager::OnElementsMutated(
    SceneGraph *graph, const std::vector<ElementMutationData> &mutation_data) {
  if (IsActive()) {
    // Invalidate the layer list if the layer order has changed.
    if (std::any_of(mutation_data.begin(), mutation_data.end(),
                    [](const ElementMutationData &md) {
                      return md.mutation_type ==
                             ElementMutationType::kZOrderMutation;
                    })) {
      InvalidateLayerListCache();
    }

    // Invalidate the has transparency value of some opacity has changed.
    if (std::any_of(mutation_data.begin(), mutation_data.end(),
                    [](const ElementMutationData &md) {
                      return md.mutation_type ==
                             ElementMutationType::kOpacityMutation;
                    })) {
      InvalidateHasTransparencyCache();
    }
  }
}

void LayerManager::AddActiveLayerListener(IActiveLayerListener *listener) {
  listener->RegisterOnDispatch(active_layer_dispatch_);
}

void LayerManager::RemoveActiveLayerListener(IActiveLayerListener *listener) {
  listener->Unregister(active_layer_dispatch_);
}

void LayerManager::InformActiveLayerListener(
    const SourceDetails &source_details) {
  auto group_id_or = GroupIdForLayerAtIndex(active_layer_index_);
  if (!group_id_or.ok()) {
    SLOG(SLOG_ERROR, "Failed to inform active layer listener: $0",
         group_id_or.status().ToString());
    return;
  }

  UUID uuid = scene_graph_->UUIDFromElementId(group_id_or.ValueOrDie());

  if (uuid != last_active_uuid_sent_) {
    proto::SourceDetails source;
    util::WriteToProto(&source, source_details);

    active_layer_dispatch_->Send(&IActiveLayerListener::ActiveLayerChanged,
                                 uuid, source);
    last_active_uuid_sent_ = uuid;
  }
}

void LayerManager::ActiveLayerChanged(
    const UUID &uuid, const proto::SourceDetails &sourceDetails) {
  if (sourceDetails.origin() == proto::SourceDetails::HOST) {
    auto group_id = scene_graph_->GroupIdFromUUID(uuid);
    if (group_id == kInvalidElementId) {
      SLOG(SLOG_ERROR, "uuid '$0' not found while setting active layer", uuid);
      return;
    }

    auto layer_list = CachedLayerList();
    auto iter = FindGroupWithId(layer_list, group_id);
    if (iter == layer_list.end()) {
      SLOG(SLOG_ERROR, "group id '$0' not found while setting active layer",
           group_id);
      return;
    }

    last_active_uuid_sent_ = uuid;
    active_layer_index_ = iter - layer_list.begin();
  }
}

}  // namespace ink
