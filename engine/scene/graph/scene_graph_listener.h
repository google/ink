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

#ifndef INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_LISTENER_H_
#define INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_LISTENER_H_

#include <vector>

#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/event_dispatch.h"

namespace ink {

struct SceneGraphRemoval {
  ElementId id;
  UUID uuid;
  UUID parent;

  bool operator==(const SceneGraphRemoval& r) const {
    return r.id == id && r.uuid == uuid && r.parent == parent;
  }

  bool operator!=(const SceneGraphRemoval& r) const { return !(*this == r); }
};

class SceneGraph;

class SceneGraphListener : public EventListener<SceneGraphListener> {
 public:
  virtual void OnElementAdded(SceneGraph* graph, ElementId id) = 0;
  virtual void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) = 0;

  virtual void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) = 0;

  // Longform support. Can be removed only iff Longform stops using it.
  virtual void PreElementAdded(ProcessedElement* line, glm::mat4 obj_to_world) {
  }
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_LISTENER_H_
