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

#include "ink/engine/geometry/mesh/mesh_test_helpers.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/math_defines.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

Mesh MakeTriangleStrip(std::vector<Vertex> vertices) {
  EXPECT(vertices.size() >= 3);
  Mesh m;
  m.verts = std::move(vertices);
  m.idx.reserve(3 * (m.verts.size() - 2));
  for (int i = 0; i < m.verts.size() - 2; ++i) {
    m.idx.emplace_back(i);
    m.idx.emplace_back((i + 1) % m.verts.size());
    m.idx.emplace_back((i + 2) % m.verts.size());
  }
  return m;
}

Mesh MakeRingMesh(glm::vec2 center, float inner_radius, float outer_radius,
                  int subdivisions) {
  std::vector<Vertex> vertices;
  vertices.reserve(2 * (subdivisions + 1));
  float angle_increment = M_TAU / subdivisions;
  for (int i = 0; i <= subdivisions; ++i) {
    vertices.emplace_back(
        PointOnCircle(angle_increment * i, inner_radius, center));
    vertices.emplace_back(
        PointOnCircle(angle_increment * i, outer_radius, center));
  }
  return MakeTriangleStrip(std::move(vertices));
}

Mesh MakeSineWaveMesh(glm::vec2 start, float amplitude, float frequency,
                      float length, float width, int subdivisions) {
  std::vector<Vertex> vertices;
  vertices.reserve(2 * (subdivisions + 1));
  float length_increment = length / subdivisions;
  glm::vec2 offset{0, .5 * width};
  for (int i = 0; i <= subdivisions; ++i) {
    glm::vec2 center =
        start + glm::vec2{length_increment * i,
                          amplitude * std::sin(M_TAU * frequency *
                                               length_increment * i)};
    vertices.emplace_back(center + offset);
    vertices.emplace_back(center - offset);
  }
  return MakeTriangleStrip(std::move(vertices));
}

Mesh FlattenObjectMatrix(const Mesh& mesh) {
  Mesh ans = mesh;
  for (Vertex& vert : ans.verts) {
    vert.position = geometry::Transform(vert.position, mesh.object_matrix);
  }
  ans.object_matrix = glm::mat4{1};  // the identity
  return ans;
}

}  // namespace ink
