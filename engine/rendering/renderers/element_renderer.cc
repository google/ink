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

#include "ink/engine/rendering/renderers/element_renderer.h"

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

ElementRenderer::ElementRenderer(const service::UncheckedRegistry& registry)
    : ElementRenderer(registry.GetShared<GLResourceManager>()) {}

ElementRenderer::ElementRenderer(
    std::shared_ptr<GLResourceManager> gl_resources)
    : mesh_renderer_(gl_resources), zoomable_rect_renderer_(gl_resources) {}

bool ElementRenderer::Draw(ElementId element, const SceneGraph& graph,
                           const Camera& camera, FrameTimeS draw_time,
                           const glm::mat4& transform) const {
  switch (element.Type()) {
    case POLY: {
      OptimizedMesh* m;
      if (graph.GetMesh(element, &m)) {
        m->object_matrix = transform * m->object_matrix;

        const auto& metadata = graph.GetElementMetadata(element);
        if (metadata.attributes.is_zoomable) {
          zoomable_rect_renderer_.Draw(camera, draw_time, m->WorldBounds(),
                                       m->texture->uri);
        } else {
          mesh_renderer_.Draw(camera, draw_time, *m);
        }
        return true;
      }
    } break;
    default:
      UNHANDLED_ELEMENT_TYPE(element);
  }

  return false;
}
}  // namespace ink
