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

#ifndef INK_ENGINE_SCENE_GRAPH_SCENE_CHANGE_NOTIFIER_H_
#define INK_ENGINE_SCENE_GRAPH_SCENE_CHANGE_NOTIFIER_H_

#include <memory>
#include "ink/engine/public/host/iscene_change_listener.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/types/element_metadata.h"

namespace ink {

// SceneChangeNotifier listens to the SceneGraph for scene changes and
// notifies ISceneChangeListener so that the host can react appropriately.
class SceneChangeNotifier : public SceneGraphListener,
                            public IActiveLayerListener {
 public:
  SceneChangeNotifier(std::shared_ptr<ISceneChangeListener> listener,
                      std::shared_ptr<LayerManager> layer_manager);

  void OnElementAdded(SceneGraph* graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override;

  void ActiveLayerChanged(const UUID& uuid,
                          const proto::SourceDetails& sourceDetails) override;

  // When the SceneChangeNotifier is disabled, it ignores all events. This is
  // meant to help keep quiet during document loading.
  void SetEnabled(bool enabled) { enabled_ = enabled; }

  // SceneChangeNotifier is neither copyable nor movable.
  SceneChangeNotifier(const SceneChangeNotifier&) = delete;
  SceneChangeNotifier& operator=(const SceneChangeNotifier&) = delete;

 private:
  // Return the UUID of the layer above the given layer (kInvalidUUID if it's on
  // top)
  UUID LayerAbove(GroupId id, SceneGraph* graph);

  std::shared_ptr<ISceneChangeListener> dispatch_;
  std::shared_ptr<LayerManager> layer_manager_;
  bool enabled_ = true;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_SCENE_CHANGE_NOTIFIER_H_
