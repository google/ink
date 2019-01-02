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

#include "ink/engine/geometry/tess/tessellator.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace {
using geometry::Polygon;

std::vector<Vertex> PolygonToVertices(const Polygon& polygon) {
  std::vector<Vertex> vertices;
  vertices.reserve(polygon.Size());
  for (const auto& p : polygon.Points()) vertices.emplace_back(p);
  return vertices;
}
}  // namespace

extern "C" void TessBegin(GLenum prim, void* poly_data) {
  // prim is always GL_TRIANGLES because we set edge flag callback
}
extern "C" void TessEdgeFlagData(GLboolean flag, void* poly_data) {}

extern "C" void TessAddVert(void* vert_data, void* poly_data) {
  Tessellator* tess = static_cast<Tessellator*>(poly_data);
  auto vert = reinterpret_cast<Vertex const*>(vert_data);

  uint16_t idx;
  auto ptidx = tess->pt_to_idx_.find(vert->position);
  if (ptidx != tess->pt_to_idx_.end()) {
    idx = ptidx->second;
  } else {
    idx = tess->mesh_.verts.size();
    tess->mesh_.verts.push_back(*vert);
    tess->pt_to_idx_.insert(
        std::pair<glm::vec2, uint32_t>(vert->position, idx));

    if (tess->combined_verts_.find(vert->position) !=
        tess->combined_verts_.end()) {
      tess->mesh_.combined_idx.push_back(idx);
    }
  }

  tess->mesh_.idx.push_back(idx);
}

extern "C" void TessEnd(void* poly_data) {
  Tessellator* tess = static_cast<Tessellator*>(poly_data);
  tess->temp_verts_.clear();
}

extern "C" void TessCombine(GLdouble intersection[3], void* neighbors[4],
                            GLfloat weights[4], void** vert_data,
                            void* poly_data) {
  Tessellator* tess = static_cast<Tessellator*>(poly_data);
  glm::vec2 pos(intersection[0], intersection[1]);
  auto v = absl::make_unique<Vertex>(pos);

  Vertex vertex_neighbors[4];
  size_t count = 0;
  for (size_t i = 0; i < 4; i++) {
    auto vn = static_cast<Vertex*>(neighbors[i]);
    if (vn != nullptr) {
      ASSERT(count == i);
      vertex_neighbors[i] = *vn;
      count++;
    }
  }
  *v = Vertex::Mix(vertex_neighbors, weights, count);

  tess->combined_verts_.insert(pos);

  *vert_data = v.get();
  tess->temp_verts_.push_back(std::move(v));
}

extern "C" void TessError(GLenum err, void* poly_data) {
  SLOG(SLOG_ERROR, "tessellation error: $0", HexStr(err));
  (static_cast<Tessellator*>(poly_data))->SetErrorFlag();
}

Tessellator::Tessellator()
    : tess_mid_pts_(false), glu_tess_(nullptr), did_error_(false) {
  Setup();
}

void Tessellator::Setup() {
  glu_tess_ = gluNewTess();
  EXPECT(glu_tess_ != nullptr);
  gluTessProperty(glu_tess_, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
  gluTessNormal(glu_tess_, 0.0, 0.0, 1.0);

  gluTessCallback(glu_tess_, GLU_TESS_BEGIN_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessBegin));
  gluTessCallback(glu_tess_, GLU_TESS_EDGE_FLAG_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessEdgeFlagData));
  gluTessCallback(glu_tess_, GLU_TESS_VERTEX_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessAddVert));
  gluTessCallback(glu_tess_, GLU_TESS_COMBINE_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessCombine));
  gluTessCallback(glu_tess_, GLU_TESS_END_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessEnd));
  gluTessCallback(glu_tess_, GLU_TESS_ERROR_DATA,
                  reinterpret_cast<_GLUfuncptr>(TessError));
  gluTessProperty(glu_tess_, GLU_TESS_TOLERANCE, 0.002);
}

Tessellator::~Tessellator() {
  if (glu_tess_ != nullptr) gluDeleteTess(glu_tess_);
  glu_tess_ = nullptr;
}

template <typename T>
void Tessellator::Inject(const T from, const T to) {
  for (auto ai = from; ai != to; ai++) {
    double vals[3];
    auto& vert = *ai;
    vals[0] = vert.position.x;
    vals[1] = vert.position.y;
    vals[2] = 0;

    // Explicit casts to ensure the type. We cast from
    // Vertex const * -> void * here, and
    // void * -> Vertex const * in tessAddVert
    // (tessAddVert is the callback from gluTessVertex)
    void* vert_data = const_cast<Vertex*>(&vert);
    gluTessVertex(glu_tess_, vals, vert_data);
  }
}

bool Tessellator::Tessellate(const FatLine& line, bool end_cap) {
  did_error_ = false;
  gluTessBeginPolygon(glu_tess_, this);
  gluTessBeginContour(glu_tess_);

  Inject(line.StartCap().begin(), line.StartCap().end());
  Inject(line.ForwardLine().begin(), line.ForwardLine().end());
  if (end_cap) Inject(line.EndCap().begin(), line.EndCap().end());
  if (tess_mid_pts_) {
    // The midpoints contain only the screen coordinates, but we need vertices.
    std::vector<Vertex> midpoints;
    midpoints.reserve(line.MidPoints().size());
    for (const auto& m : line.MidPoints())
      midpoints.push_back(Vertex(m.screen_position));

    Inject(midpoints.rbegin(), midpoints.rend());
    Inject(midpoints.begin(), midpoints.end());
  }
  Inject(line.BackwardLine().rbegin(), line.BackwardLine().rend());

  gluTessEndContour(glu_tess_);
  gluTessEndPolygon(glu_tess_);
  return !did_error_;
}

bool Tessellator::Tessellate(const std::vector<Vertex>& pts) {
  did_error_ = false;
  gluTessBeginPolygon(glu_tess_, this);
  gluTessBeginContour(glu_tess_);

  Inject(pts.begin(), pts.end());

  gluTessEndContour(glu_tess_);
  gluTessEndPolygon(glu_tess_);
  return !did_error_;
}

void Tessellator::Clear() {
  mesh_.Clear();
  temp_verts_.clear();
  pt_to_idx_.clear();
  combined_verts_.clear();
}

bool Tessellator::Tessellate(const std::vector<std::vector<Vertex>>& edges) {
  did_error_ = false;
  gluTessProperty(glu_tess_, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
  gluTessBeginPolygon(glu_tess_, this);

  for (auto aedge = edges.begin(); aedge != edges.end(); aedge++) {
    gluTessBeginContour(glu_tess_);
    Inject(aedge->begin(), aedge->end());
    gluTessEndContour(glu_tess_);
  }

  gluTessEndPolygon(glu_tess_);
  return !did_error_;
}

bool Tessellator::Tessellate(const geometry::Polygon& polygon) {
  return Tessellate(PolygonToVertices(polygon));
}

bool Tessellator::Tessellate(const std::vector<geometry::Polygon>& polygons) {
  std::vector<std::vector<Vertex>> vertices;
  vertices.reserve(polygons.size());
  for (const auto& p : polygons) vertices.emplace_back(PolygonToVertices(p));
  return Tessellate(vertices);
}

}  // namespace ink
