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

#include "ink/engine/util/dbg_helper.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <utility>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

DbgHelper::DbgHelper(const service::UncheckedRegistry& registry)
    : DbgHelper(registry.GetShared<GLResourceManager>()) {}

DbgHelper::DbgHelper(const std::shared_ptr<GLResourceManager>& gl_resources)
    : gl_resources_(gl_resources), renderer_(gl_resources_) {}

void DbgHelper::Draw(const Camera& cam, FrameTimeS draw_time) {
  // See if we can avoid drawing altogether.
  if (points_.empty() && lines_.empty() && rects_.empty() &&
      skeletons_.empty() && meshes_.empty()) {
    return;
  }

  for (const auto& key_value_pair : lines_) {
    const auto& dbg_line = key_value_pair.second;
    DrawLine(cam, draw_time, dbg_line.start, dbg_line.end, dbg_line.color,
             dbg_line.size_screen);
  }

  for (const auto& key_value_pair : points_) {
    const auto& dbg_point = key_value_pair.second;
    DrawPoint(cam, draw_time, dbg_point.position, dbg_point.color,
              dbg_point.size);
  }

  for (const auto& key_value_pair : rects_) {
    const auto& dbg_rect = key_value_pair.second;
    DrawRect(cam, draw_time, dbg_rect.r, dbg_rect.color, dbg_rect.fill);
  }

  for (const auto& key_value_pair : skeletons_) {
    const auto& dbg_skeleton = key_value_pair.second;
    DrawSkeleton(cam, draw_time, dbg_skeleton.mesh, dbg_skeleton.edge_color,
                 dbg_skeleton.point_color);
  }

  for (auto& key_value_pair : meshes_) {
    auto& mesh = key_value_pair.second;
    DrawMesh(cam, draw_time, &mesh);
  }
}

void DbgHelper::DrawPoint(const Camera& cam, FrameTimeS draw_time,
                          glm::vec2 position, glm::vec4 color, float size) {
  DrawRect(cam, draw_time, Rect::CreateAtPoint(position, size, size), color,
           true);
}

void DbgHelper::DrawLine(const Camera& cam, FrameTimeS draw_time,
                         glm::vec2 start, glm::vec2 end, glm::vec4 color,
                         float size_screen) {
  auto width_world = cam.ConvertDistance(size_screen, DistanceType::kScreen,
                                         DistanceType::kWorld);
  Mesh mesh(MakeLine(start, end, color, width_world));
  mesh.GenIndex();
  gl_resources_->mesh_vbo_provider->GenVBO(&mesh, GL_STREAM_DRAW);
  renderer_.Draw(cam, draw_time, mesh);
}

void DbgHelper::DrawRect(const Camera& cam, FrameTimeS draw_time, const Rect& r,
                         const glm::vec4& color, bool fill) {
  if (fill) {
    Mesh mesh;
    MakeRectangleMesh(&mesh, r, color);
    DrawMesh(cam, draw_time, &mesh);
  } else {
    float width_screen =
        cam.ConvertDistance(3, DistanceType::kDp, DistanceType::kScreen);
    DrawLine(cam, draw_time, r.Leftbottom(), r.Lefttop(), color, width_screen);
    DrawLine(cam, draw_time, r.Leftbottom(), r.Rightbottom(), color,
             width_screen);
    DrawLine(cam, draw_time, r.Rightbottom(), r.Righttop(), color,
             width_screen);
    DrawLine(cam, draw_time, r.Lefttop(), r.Righttop(), color, width_screen);
  }
}

void DbgHelper::DrawSkeleton(const Camera& cam, FrameTimeS draw_time,
                             const Mesh& mesh, glm::vec4 edge_color,
                             glm::vec4 point_color) {
  if (mesh.verts.empty() || mesh.idx.empty()) return;

  if (edge_color.a > 0) {
    float width_screen =
        cam.ConvertDistance(1.0f, DistanceType::kDp, DistanceType::kScreen);
    for (int i = 0; i < mesh.NumberOfTriangles(); ++i) {
      auto triangle = mesh.GetTriangle(i);
      DrawLine(cam, draw_time, triangle[0], triangle[1], edge_color,
               width_screen);
      DrawLine(cam, draw_time, triangle[1], triangle[2], edge_color,
               width_screen);
      DrawLine(cam, draw_time, triangle[2], triangle[0], edge_color,
               width_screen);
    }
  }

  if (point_color.a > 0) {
    float width_world =
        cam.ConvertDistance(4.0f, DistanceType::kDp, DistanceType::kWorld);
    for (const auto& vertex : mesh.verts) {
      DrawPoint(cam, draw_time, vertex.position, point_color, width_world);
    }
  }
}

void DbgHelper::DrawMesh(const Camera& cam, FrameTimeS draw_time, Mesh* mesh) {
  // Ensure the mesh has a vbo for drawing.
  if (!gl_resources_->mesh_vbo_provider->HasVBO(*mesh)) {
    gl_resources_->mesh_vbo_provider->GenVBO(mesh, GL_STATIC_DRAW);
  }
  renderer_.Draw(cam, draw_time, *mesh);
}

void DbgHelper::Clear() {
  points_.clear();
  lines_.clear();
  rects_.clear();
  skeletons_.clear();
  meshes_.clear();
}

void DbgHelper::AddPoint(Vertex vert, float size, uint32_t id) {
  DbgPoint pt;
  pt.size = size;
  pt.position = vert.position;
  pt.color = vert.color;
  points_.insert(std::pair<uint32_t, DbgPoint>(id, pt));
}

void DbgHelper::AddLine(Vertex from_world, Vertex to_world, float size_screen,
                        uint32_t id) {
  DbgLine ln;
  ln.size_screen = size_screen;
  ln.start = from_world.position;
  ln.end = to_world.position;
  ln.color = from_world.color;
  lines_.insert(std::pair<uint32_t, DbgLine>(id, ln));
}

void DbgHelper::AddRect(Rect r, glm::vec4 color, bool fill, uint32_t id) {
  DbgRect dbgr;
  dbgr.color = color;
  dbgr.r = r;
  dbgr.fill = fill;
  rects_.insert(std::pair<uint32_t, DbgRect>(id, dbgr));
}

void DbgHelper::AddMeshSkeleton(const Mesh& m, glm::vec4 edge_color,
                                glm::vec4 point_color, uint32_t id) {
  DbgMeshSkeleton skeleton;
  skeleton.mesh = m;
  skeleton.edge_color = edge_color;
  skeleton.point_color = point_color;
  for (auto& vertex : skeleton.mesh.verts)
    vertex.position = geometry::Transform(vertex.position, m.object_matrix);
  skeleton.mesh.object_matrix = glm::mat4{1};
  skeletons_.insert(std::make_pair(id, skeleton));
}

void DbgHelper::AddMesh(const Mesh& m, uint32_t id) {
  meshes_.insert(std::pair<uint32_t, Mesh>(id, m));
}

void DbgHelper::Remove(uint32_t id) {
  points_.erase(id);
  lines_.erase(id);
  rects_.erase(id);
  skeletons_.erase(id);
  meshes_.erase(id);
}

void DbgHelper::AddMesh(const Mesh& m, glm::vec4 color, uint32_t id) {
  auto pair = meshes_.emplace(id, Mesh());
  pair->second = m;
  for (auto& v : pair->second.verts) {
    v.color = color;
  }
  gl_resources_->mesh_vbo_provider->GenVBO(&pair->second, GL_DYNAMIC_DRAW);
}
}  // namespace ink
