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

#ifndef INK_ENGINE_GEOMETRY_MESH_SHAPE_HELPERS_H_
#define INK_ENGINE_GEOMETRY_MESH_SHAPE_HELPERS_H_

#include <cstddef>
#include <vector>

#include "third_party/absl/strings/string_view.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"

namespace ink {

// Clears the mesh's vertex and face information, and repopulates it with the
// given object-coordinate rectangle. Each vertex's color will be set to the
// given one, and its texture-coordinates will be calculated by applying the
// object-to-uv transformation to the position.
// Note: the mesh's texture info, shader metadata, and object-to-world matrix
// are not affected by this function.
void MakeRectangleMesh(Mesh* mesh, const RotRect& object_rectangle,
                       const glm::vec4 color = {1, 1, 1, 1},
                       const glm::mat4 object_to_uv = glm::mat4{0});

inline void MakeRectangleMesh(Mesh* mesh, const Rect& object_rectangle,
                              const glm::vec4 color = {1, 1, 1, 1},
                              const glm::mat4 object_to_uv = glm::mat4{0}) {
  return MakeRectangleMesh(mesh, RotRect(object_rectangle), color,
                           object_to_uv);
}

// Make a rectangle with the given display coords with a bitmap texture
// starting at the given Rect.
inline void MakeImageRectMesh(Mesh* mesh, const RotRect& display_rect,
                              const RotRect& first_instance_rect,
                              absl::string_view texture_uri) {
  MakeRectangleMesh(
      mesh, display_rect, glm::vec4{1},
      first_instance_rect.CalcTransformTo(RotRect({.5, .5}, {1, -1}, 0)));
  mesh->texture = absl::make_unique<TextureInfo>(texture_uri);
}
inline void MakeImageRectMesh(Mesh* mesh, const Rect& display_rect,
                              const Rect& first_instance_rect,
                              absl::string_view texture_uri) {
  MakeImageRectMesh(mesh, RotRect(display_rect), RotRect(first_instance_rect),
                    texture_uri);
}

// Returns a list of 6n vertices drawing n dashes, where each dash is
// triangulated quad.
std::vector<Vertex> MakeDashedLine(glm::vec2 from, glm::vec2 to,
                                   glm::vec4 color, float width,
                                   float dash_length);

// Sets the contents of the passed in Mesh to be a dashed outline.
void MakeDashedRectangle(Mesh* mesh, Rect r, glm::vec4 color, float width,
                         float dash_length);

// Returns six vertices triangulating the rectange with midline from "from" to
// "to" and extending "width"/2 in both directions from the midline.  If
// "from"=="to", returns a rectangle with every vertex at "from".
std::vector<Vertex> MakeLine(glm::vec2 from, glm::vec2 to, glm::vec4 color,
                             float width);

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_SHAPE_HELPERS_H_
