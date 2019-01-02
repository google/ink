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

#include "ink/engine/rendering/renderers/rectangles_renderer.h"

#include <vector>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"

namespace ink {

RectanglesRenderer::RectanglesRenderer(
    const std::shared_ptr<GLResourceManager>& gl_resources)
    : gl_resources_(gl_resources) {
  rect_mesh_bounds_ = Rect(0, 0, 1, 1);
  Mesh m;
  MakeRectangleMesh(&m, rect_mesh_bounds_, glm::vec4(0));
  rect_mesh_ = absl::make_unique<OptimizedMesh>(SingleColorShader, m);
  gl_resources_->mesh_vbo_provider->EnsureOnlyInVBO(rect_mesh_.get(),
                                                    GL_STATIC_DRAW);
  renderer_ = absl::make_unique<MeshRenderer>(gl_resources_);
}
void RectanglesRenderer::DrawRectangles(const std::vector<Rect>& rects,
                                        const glm::vec4& color,
                                        const Camera& cam,
                                        FrameTimeS draw_time) const {
  for (auto rect : rects) {
    rect_mesh_->add_color_modifier = color;
    auto mat = rect_mesh_->object_matrix;
    rect_mesh_->object_matrix = rect_mesh_bounds_.CalcTransformTo(rect) * mat;
    renderer_->Draw(cam, draw_time, *rect_mesh_);
    rect_mesh_->object_matrix = mat;
  }
}

}  // namespace ink
