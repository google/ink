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

#ifndef INK_ENGINE_SCENE_GRAPH_MOCK_SCENE_GRAPH_LISTENER_H_
#define INK_ENGINE_SCENE_GRAPH_MOCK_SCENE_GRAPH_LISTENER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"

namespace ink {

class MockSceneGraphListener : public SceneGraphListener {
 public:
  MOCK_METHOD2(OnElementAdded, void(SceneGraph* graph, ElementId id));
  MOCK_METHOD2(OnElementsRemoved,
               void(SceneGraph* graph,
                    const std::vector<SceneGraphRemoval>& removal));
  MOCK_METHOD2(OnElementsMutated,
               void(SceneGraph* graph,
                    const std::vector<ElementMutationData>& mutation_data));
  MOCK_METHOD2(PreElementAdded,
               void(ProcessedElement* element, glm::mat4 obj_to_world));
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_MOCK_SCENE_GRAPH_LISTENER_H_
