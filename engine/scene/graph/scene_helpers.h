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

#ifndef INK_ENGINE_SCENE_GRAPH_SCENE_HELPERS_H_
#define INK_ENGINE_SCENE_GRAPH_SCENE_HELPERS_H_

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {
// Add the given rectangles as solid-colored meshes to the scene, parenting each
// rect to the page it intersects, if any.
void AddRectsToSceneGraph(const std::vector<Rect>& rects, glm::vec4 color,
                          PageManager* page_manager, SceneGraph* graph);
}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_SCENE_HELPERS_H_
