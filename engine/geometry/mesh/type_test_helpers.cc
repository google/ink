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

#include "ink/engine/geometry/mesh/type_test_helpers.h"

namespace ink {
namespace funcs {

static std::vector<Vertex> ExtractFlatVertices(const Mesh& mesh) {
  Mesh copy_mesh(mesh);
  copy_mesh.Deindex();
  return copy_mesh.verts;
}

bool IsValidRectangleTriangulation(const Mesh& mesh,
                                   const Rect& rect_expected) {
  return IsValidRectangleTriangulation(ExtractFlatVertices(mesh),
                                       rect_expected);
}

void CheckTextureCoors(const Mesh& mesh, const Rect& texture_coors,
                       const Rect& quad_coors) {
  CheckTextureCoors(ExtractFlatVertices(mesh), texture_coors, quad_coors);
}

void CheckTextureCoorsInverted(const Mesh& mesh, const Rect& texture_coors,
                               const Rect& quad_coors) {
  CheckTextureCoorsInverted(ExtractFlatVertices(mesh), texture_coors,
                            quad_coors);
}

}  // namespace funcs
}  // namespace ink
