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

#include "ink/engine/scene/graph/scene_helpers.h"

#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

void AddRectsToSceneGraph(const std::vector<Rect>& rects, glm::vec4 color,
                          PageManager* page_manager, SceneGraph* graph) {
  std::vector<SceneGraph::ElementAdd> elements;
  elements.reserve(rects.size());
  for (auto& rect : rects) {
    glm::mat4 world_to_group_transform{1};
    GroupId group_id = kInvalidElementId;
    auto group_relative_rect = rect;
    if (page_manager && page_manager->MultiPageEnabled()) {
      group_id = page_manager->GetPageGroupForRect(rect);
      if (group_id == kInvalidElementId) {
        SLOG(SLOG_WARNING,
             "Given rect $0 does not intersect any page, but multi-page is "
             "enabled. Discarding.",
             rect);
        continue;
      }
      ElementMetadata meta = graph->GetElementMetadata(group_id);
      ASSERT(meta.id == group_id);
      world_to_group_transform = glm::inverse(meta.world_transform);
      group_relative_rect = geometry::Transform(rect, world_to_group_transform);
    }

    UUID uuid = graph->GenerateUUID();
    ElementId id;
    graph->GetNextPolyId(uuid, &id);

    Mesh m;
    MakeRectangleMesh(&m, group_relative_rect, color);

    auto pe = absl::make_unique<ProcessedElement>(
        id, m, ShaderType::SingleColorShader);
    pe->group = group_id;

    // Invert that matrix to get object-local outline coordinates.
    glm::mat4 page_to_object_transform = glm::inverse(pe->obj_to_group);
    pe->outline.reserve(m.verts.size());
    for (const auto& v : m.verts) {
      pe->outline.emplace_back(
          geometry::Transform(v.position, page_to_object_transform));
    }

    auto se = absl::make_unique<SerializedElement>(
        uuid, graph->UUIDFromElementId(group_id), SourceDetails::FromEngine(),
        CallbackFlags::All());
    se->Serialize(*pe);

    elements.emplace_back(std::move(pe), std::move(se));
  }
  graph->AddStrokes(std::move(elements));
}
}  // namespace ink
