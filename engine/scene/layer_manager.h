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

#ifndef INK_ENGINE_SCENE_LAYER_MANAGER_H_
#define INK_ENGINE_SCENE_LAYER_MANAGER_H_

#include <limits>
#include <vector>

#include "ink/engine/public/host/iactive_layer_listener.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/status_or.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

// The LayerManager maintains the state for a list of
// "layers". Conceptually, a layer is a container for a list of
// elements which will be rendered together. Each layer will be
// marked 'visible' by default, but can also be marked as not
// visible. At any time, there will be exactly one 'active' layer.
//
// Layers with lower indices will be rendered before those with higher
// indices.
//
// In general, layers are referred to by their 0-based index in the list,
// but each layer has a GroupId which remains constant if the layers
// are reordered.
//
// The LayerManager starts with no layers. The LayerManager will be
// 'active' at any time that it contains more than zero layers.
class LayerManager : public SceneGraphListener, public IActiveLayerListener {
 public:
  using SharedDeps = service::Dependencies<SceneGraph>;

  // Creates a new empty, inactive LayerManager.
  explicit LayerManager(std::shared_ptr<SceneGraph> scene_graph)
      : active_layer_dispatch_(new EventDispatch<IActiveLayerListener>()),
        scene_graph_(scene_graph) {
    scene_graph_->AddListener(this);
  }

  // Returns the total number of layers in the LayerManager, both visible and
  // invisible layers are included in the count.
  size_t NumLayers() const { return CachedLayerList().size(); }

  // Returns true of there is more than one layer in the LayerManager.
  bool IsActive() const { return active_; }

  // Returns true if any of the layers is transparent.
  // Returns false if the LayerManager is not active.
  bool HasTransparency() const;

  // Make LayerManager inactive and clear out all layer data.
  //
  // NOTE: Only to this if you are also clearing the scene graph; otherwise,
  //       you risk an inconsistency with the LayerManager cache. Setting the
  //       LayerManager to inactive will flush its cached layer data, and
  //       it will not be re-cached until another layer is added (thus making
  //       the LayerManager active again.
  void Reset();

  // Adds and inserts a new layer at the bottom of the current scene
  // graph.
  //
  // Adding a new layer will not affect the active layer.
  //
  // NOTE: this creates a new UUID and Group element and inserts it
  // into the SceneGraph.
  StatusOr<GroupId> AddLayer(const SourceDetails &source_details);

  // Removes the layer at the specified index.
  // If index >= NumLayers(), do nothing.
  // Removing the active layer will reset the active layer to 0.
  Status RemoveLayer(size_t index, const SourceDetails &source_details);

  // Move the layer at from_index to to_index. The active layer will not change,
  // although its index might.
  Status MoveLayer(size_t from_index, size_t to_index);

  // Sets the active layer to the specified index.
  // Does nothing and returns a failure status if index >= NumLayers().
  Status SetActiveLayer(size_t index, const SourceDetails &source_details =
                                          SourceDetails::FromEngine());

  // Returns the index of the active layer.
  // If there are no layers (!layerManager.IsActive()), returns an error
  // status.
  StatusOr<size_t> IndexOfActiveLayer() const;

  // Returns the GroupId of the active alyer.
  // If there are no layers (!layerManager.IsActive()), returns an error status.
  StatusOr<GroupId> GroupIdOfActiveLayer() const;

  // Marks a layer as visible (or invisible).
  // If index is out of range, returns an error status.
  Status SetLayerVisibility(size_t index, bool visible);

  // Finds the GroupId for the layer at a particular index. If index is
  // out of range, returns an error status.
  //
  // NOTE: IndexForLayerWithGroupId runs in O(n) time, so the
  // LayerManager is currently only suitable for small numbers of
  // layers.
  StatusOr<GroupId> GroupIdForLayerAtIndex(size_t index) const;

  // Returns the index for the layer with the requested GroupId. If it
  // is not found, returns an error status.
  StatusOr<size_t> IndexForLayerWithGroupId(GroupId group_id) const;

  // Convenience methods.

  // Returns true if active layer is topmost or layers are not active.
  //
  // If layers aren't active, then all drawing happens in the topmost "layer"
  // and nothing in the scene graph can ever be above the active line.
  bool IsActiveLayerTopmost() const;

  // Returns true if the given group ID is above the active layer and false
  // otherwise or if layers aren't active.
  //
  // If layers aren't active, nothing in the scene graph can ever be above the
  // active line.
  bool IsAboveActiveLayer(const GroupId &group_id) const;

  // SceneGraphListener
  void OnElementAdded(SceneGraph *graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph *graph,
      const std::vector<SceneGraphRemoval> &removed_elements) override;
  void OnElementsMutated(
      SceneGraph *graph,
      const std::vector<ElementMutationData> &mutation_data) override;

  void AddActiveLayerListener(IActiveLayerListener *listener);
  void RemoveActiveLayerListener(IActiveLayerListener *listener);

  // IActiveLayerListener
  void ActiveLayerChanged(const UUID &uuid,
                          const proto::SourceDetails &sourceDetails) override;

 private:
  // Invalidates the internal layer list causing it to be reloaded from the
  // SceneGraph. Call only if you know what you are doing. Bad performance may
  // result from abuse of this method.
  void InvalidateLayerListCache();

  // Invalidates the cached result for HasTransparency().
  void InvalidateHasTransparencyCache() { cached_has_transparency_.reset(); }

  using LayerList = std::vector<GroupId>;

  const LayerList &CachedLayerList() const;

  // We keep a cache of the current layer list in z-order. This list is read
  // directly from the SceneGraph. Operations that would mutate this list
  // (adding/removing/changing z-order) MUST invalidate this cache so that the
  // next access will reload it.
  //
  // If the LayerManager is active and the list is empty, then it needs to be
  // reloaded. (If the LayerManager is not active, then it should always be
  // empty.)
  //
  // Do NOT refer to this directly. Use CachedLayerList() to help ensure
  // cache consistency.
  //
  // Marked "mutable" since const operations that use the layer list may need
  // to re-read it from the SceneGraph.
  mutable LayerList cached_layer_list_;

  // Cached value for the 'has transparency' result.
  // DO NOT refer to this directly. Use HasTransparency() to ensure cache
  // consistency.
  mutable absl::optional<bool> cached_has_transparency_;

  LayerList::const_iterator FindGroupWithId(const LayerList &layer_ids,
                                            const GroupId &group_id) const;

  void InformActiveLayerListener(const SourceDetails &source_details);
  std::shared_ptr<EventDispatch<IActiveLayerListener>> active_layer_dispatch_;

  // The last uuid for which we sent an ActiveLayerChanged event.
  UUID last_active_uuid_sent_ = kInvalidUUID;

  bool active_ = false;

  // The scene_graph that will display our layers. Currently only used
  // directly to get new UUIDs and GroupIds.
  std::shared_ptr<SceneGraph> scene_graph_;

  // The currently active layer.
  // This only has meaning when the LayerManager is active.
  //
  // Initialized to size_t::max() so that the initial layer creation
  // will trigger an layer change event.
  size_t active_layer_index_ = std::numeric_limits<size_t>::max();
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_LAYER_MANAGER_H_
