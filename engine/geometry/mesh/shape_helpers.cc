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

#include "ink/engine/geometry/mesh/shape_helpers.h"

#include <cstdint>
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

using glm::mat4;
using glm::vec2;
using glm::vec4;

void MakeRectangleMesh(Mesh* mesh, const RotRect& object_rectangle,
                       const glm::vec4 color, const glm::mat4 object_to_uv) {
  mesh->Clear();
  mesh->idx = {0, 1, 3, 1, 2, 3};
  mesh->verts.reserve(4);
  for (auto corner : object_rectangle.Corners()) {
    mesh->verts.emplace_back(corner, color);
    mesh->verts.back().texture_coords =
        geometry::Transform(corner, object_to_uv);
  }
}

std::vector<Vertex> MakeDashedLine(vec2 from, vec2 to, vec4 color, float width,
                                   float dash_length) {
  std::vector<Vertex> res;

  auto len = glm::length(from - to);
  auto dash_margin = dash_length * 0.66f;
  auto ndashes = static_cast<int>(len / (dash_length + dash_margin));

  auto dir = (to - from) / len;
  auto spt = from;

  // center the dashes within the space to fill
  auto dashes_len = ndashes * dash_length + (ndashes - 1) * dash_margin;
  auto remainder = len - dashes_len;
  spt += dir * (remainder / 2);

  for (int i = 0; i < ndashes; i++) {
    auto ept = spt + dir * dash_length;
    auto pts = MakeLine(spt, ept, color, width);
    res.insert(res.end(), pts.begin(), pts.end());
    spt = ept + dir * dash_margin;
  }
  return res;
}

static void SetColor(std::vector<Vertex>* points, vec4 color) {
  for (Vertex& vert : *points) {
    vert.color = color;
  }
}

std::vector<Vertex> MakeLine(vec2 from, vec2 to, vec4 color, float width) {
  std::vector<Vertex> pts(6);
  auto hwidth = width / 2.0f;

  auto v = to - from;
  if (glm::length(v) > 0) {
    auto cross = vec2(-v.y, v.x);
    cross = glm::normalize(cross) * hwidth;

    pts[0] = Vertex(from - cross);
    pts[1] = Vertex(to - cross);
    pts[2] = Vertex(to + cross);

    pts[3] = Vertex(from + cross);
    pts[4] = Vertex(to + cross);
    pts[5] = Vertex(from - cross);
  } else {
    for (size_t i = 0; i < pts.size(); i++) {
      pts[i] = Vertex(from);
    }
  }
  SetColor(&pts, color);
  return pts;
}

void MakeDashedRectangle(Mesh* mesh, Rect r, glm::vec4 color, float width,
                         float dash_length) {

  mesh->Clear();

  std::vector<Vertex>* out = &mesh->verts;

  std::vector<Vertex> top =
      MakeDashedLine(r.Lefttop(), r.Righttop(), color, width, dash_length);
  out->insert(out->end(), top.begin(), top.end());

  std::vector<Vertex> right =
      MakeDashedLine(r.Righttop(), r.Rightbottom(), color, width, dash_length);
  out->insert(out->end(), right.begin(), right.end());

  std::vector<Vertex> bottom = MakeDashedLine(r.Rightbottom(), r.Leftbottom(),
                                              color, width, dash_length);
  out->insert(out->end(), bottom.begin(), bottom.end());

  std::vector<Vertex> left =
      MakeDashedLine(r.Leftbottom(), r.Lefttop(), color, width, dash_length);
  out->insert(out->end(), left.begin(), left.end());

  mesh->GenIndex();
}
}  // namespace ink
