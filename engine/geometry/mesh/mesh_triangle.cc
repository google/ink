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

#include <cstdint>

#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/mesh/mesh_triangle.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {

MeshTriSegment::MeshTriSegment() {}
MeshTriSegment::MeshTriSegment(uint16_t f, uint16_t t) {
  idx[0] = f;
  idx[1] = t;
  ASSERT(f != t);
}

uint16_t MeshTriSegment::LowIdx() const { return std::min(idx[0], idx[1]); }

uint16_t MeshTriSegment::HighIdx() const { return std::max(idx[0], idx[1]); }

bool MeshTriSegment::HasIdx(uint16_t test_idx) const {
  return idx[0] == test_idx || idx[1] == test_idx;
}

bool MeshTriSegment::SharedIdx(const MeshTriSegment& other,
                               uint16_t* shared) const {
  for (auto i : idx) {
    if (other.idx[0] == i || other.idx[1] == i) {
      *shared = i;
      return true;
    }
  }
  return false;
}

uint16_t MeshTriSegment::OtherIdx(uint16_t shared_idx) const {
  if (shared_idx != idx[0]) return idx[0];
  return idx[1];
}

Segment MeshTriSegment::PtSegment(const Mesh& mesh) const {
  return Segment(mesh.verts[LowIdx()].position, mesh.verts[HighIdx()].position);
}

bool MeshTriSegment::operator==(const MeshTriSegment& other) const {
  return LowIdx() == other.LowIdx() && HighIdx() == other.HighIdx();
}

bool MeshTriSegment::operator<(const MeshTriSegment& other) const {
  if (LowIdx() != other.LowIdx()) {
    return LowIdx() < other.LowIdx();
  } else {
    return HighIdx() < other.HighIdx();
  }
}

std::string MeshTriSegment::ToString() const {
  return Substitute("($0, $1)", LowIdx(), HighIdx());
}

float MeshTriSegment::Length2(const Mesh& mesh) const {
  const auto& x = mesh.verts[idx[0]].position;
  const auto& y = mesh.verts[idx[1]].position;
  glm::vec2 delta = y - x;
  return glm::dot(delta, delta);
}

float MeshTriSegment::Length(const Mesh& mesh) const {
  const auto& x = mesh.verts[idx[0]].position;
  const auto& y = mesh.verts[idx[1]].position;
  return geometry::Distance(x, y);
}

Vertex MeshTriSegment::Midpt(const Mesh& mesh) const {
  auto from = mesh.verts[LowIdx()];
  auto to = mesh.verts[HighIdx()];
  return util::Lerpnc(from, to, 0.5f);
}

////////////////////////////////////////////////////////////////////////////////

MeshTriSegment MeshTriangle::Segment(int n) const {
  uint16_t f = n % 3;
  uint16_t t = (n + 1) % 3;
  return MeshTriSegment(idx[f], idx[t]);
}

bool MeshTriangle::Valid() const {
  return idx[0] != idx[1] && idx[0] != idx[2] && idx[1] != idx[2];
}

bool MeshTriangle::HasIdx(uint16_t target, uint32_t* interior_idx) const {
  bool found = false;
  uint32_t i = 0;
  for (i = 0; i < 3; i++) {
    if (idx[i] == target) {
      found = true;
      break;
    }
  }
  *interior_idx = i;
  return found;
}

float MeshTriangle::Area(const Mesh& mesh) const {
  // Heron's formula
  // This is slow, we could use the cross product instead.
  auto a = Segment(0).Length(mesh);
  auto b = Segment(1).Length(mesh);
  auto c = Segment(2).Length(mesh);
  auto s = (a + b + c) / 2;
  return std::sqrt(s * (s - a) * (s - b) * (s - c));
}

bool MeshTriangle::OtherIdx(const MeshTriSegment& segment,
                            uint16_t* other) const {
  for (auto i : idx) {
    if (i != segment.idx[0] && i != segment.idx[1]) {
      *other = i;
      return true;
    }
  }
  return false;
}

bool MeshTriangle::ContainsPt(const Mesh& mesh, glm::vec2 point) const {
  auto o1 = Orientation(mesh.verts[idx[0]].position,
                        mesh.verts[idx[1]].position, point);
  auto o2 = Orientation(mesh.verts[idx[1]].position,
                        mesh.verts[idx[2]].position, point);
  auto o3 = Orientation(mesh.verts[idx[2]].position,
                        mesh.verts[idx[0]].position, point);
  return o1 == o2 && o2 == o3;
}

void MeshTriangle::AppendToMesh(Mesh* mesh) const {
  for (auto ai : idx) {
    mesh->idx.push_back(ai);
  }
}

std::string MeshTriangle::ToString() const {
  return Substitute("($0, $1, $2)", idx[0], idx[1], idx[2]);
}

bool MeshTriangle::operator==(const MeshTriangle& other) const {
  return RoughEquals(other);
}

// returns if this has the same verticies as other
// (winding order doesn't matter)
bool MeshTriangle::RoughEquals(const MeshTriangle& other) const {
  int found = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (idx[i] == other.idx[j]) {
        found++;
        break;
      }
    }
  }

  return found == 3;
}

// returns if this is the same triangle (start offset doesn't
// (matter, winding order does matter)
bool MeshTriangle::StrictEquals(const MeshTriangle& other) const {
  // find 1st idx in other
  uint32_t startidx = 0;
  bool found = false;
  for (; startidx < 3; startidx++) {
    if (other.idx[startidx] == idx[0]) {
      found = true;
      break;
    }
  }
  if (!found) return false;

  // the other must have every index in the same order
  // (but offsets are ok)
  for (int i = 0; i < 3; i++) {
    if (idx[i] != other.idx[(startidx + i) % 3]) return false;
  }
  return true;
}

Vertex MeshTriangle::Centroid(const Mesh& mesh) const {
  Vertex res;
  for (int i = 0; i < 3; i++) {
    res = res + mesh.verts[idx[i]];
  }
  return res / 3.0f;
}

////////////////////////////////////////////////////////////////////////////////

MeshTriVert::MeshTriVert(MeshTriangle* t, uint16_t idx)
    : tri(t), interior_idx(idx % 3) {}

uint16_t MeshTriVert::Idx() const { return tri->idx[interior_idx]; }

MeshTriVert MeshTriVert::Advance() const {
  MeshTriVert res = *this;
  res.interior_idx = (1 + res.interior_idx) % 3;
  return res;
}

////////////////////////////////////////////////////////////////////////////////

MeshTetrahedron::MeshTetrahedron() : t1(nullptr), t2(nullptr) {}

MeshTetrahedron::MeshTetrahedron(MeshTriangle* t1, MeshTriangle* t2)
    : t1(t1), t2(t2) {
  ASSERT(t1 != t2);
  ASSERT(t1 && t2);
}

bool MeshTetrahedron::Valid() const {
  if (!t1 || !t2) return false;
  if (!t1->Valid() || !t2->Valid()) return false;
  if (*t1 == *t2) return false;
  return true;
}

MeshTriVert MeshTetrahedron::Advance(MeshTriVert v) const {
  MeshTriVert res = v;
  MeshTriangle* other = t1;
  if (other == v.tri) other = t2;

  uint32_t other_interior_idx;
  if (other->HasIdx(v.tri->idx[v.interior_idx], &other_interior_idx)) {
    res.tri = other;
    res.interior_idx = other_interior_idx;
  }
  res = res.Advance();
  return res;
}

bool MeshTetrahedron::IsShared(MeshTriVert v) const {
  auto other = t1;
  if (v.tri == other) other = t2;

  uint32_t other_interior_idx;
  return other->HasIdx(v.Idx(), &other_interior_idx);
}
}  // namespace ink
