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

#include "ink/engine/rendering/renderers/shape_renderer.h"

#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

ShapeRenderer::ShapeRenderer(const service::UncheckedRegistry& registry)
    : renderer_(registry),
      gl_resources_(registry.GetShared<GLResourceManager>()) {}

ShapeRenderer::ShapeRenderer(
    std::shared_ptr<GLResourceManager> gl_resource_manager)
    : renderer_(gl_resource_manager), gl_resources_(gl_resource_manager) {}

void ShapeRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                         const Shape& shape) const {
  if (!shape.visible()) return;

  const Mesh* m;
  if (shape.fillVisible() && shape.GetFillMesh(*gl_resources_, &m)) {
    renderer_.Draw(cam, draw_time, *m);
  }
  if (shape.borderVisible() && shape.GetBorderMesh(*gl_resources_, &m)) {
    renderer_.Draw(cam, draw_time, *m);
  }
}
}  // namespace ink
