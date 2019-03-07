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

#include "ink/engine/geometry/mesh/mesh_triangle.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {

MeshTriSegment::MeshTriSegment() {}
MeshTriSegment::MeshTriSegment(Mesh::IndexType f, Mesh::IndexType t) {
  idx[0] = f;
  idx[1] = t;
  ASSERT(f != t);
}

Mesh::IndexType MeshTriSegment::LowIdx() const {
  return std::min(idx[0], idx[1]);
}

Mesh::IndexType MeshTriSegment::HighIdx() const {
  return std::max(idx[0], idx[1]);
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

////////////////////////////////////////////////////////////////////////////////

MeshTriSegment MeshTriangle::Segment(int n) const {
  Mesh::IndexType f = n % 3;
  Mesh::IndexType t = (n + 1) % 3;
  return MeshTriSegment(idx[f], idx[t]);
}

bool MeshTriangle::Valid() const {
  return idx[0] != idx[1] && idx[0] != idx[2] && idx[1] != idx[2];
}

bool MeshTriangle::HasIdx(Mesh::IndexType target,
                          Mesh::IndexType* interior_idx) const {
  bool found = false;
  Mesh::IndexType i = 0;
  for (i = 0; i < 3; i++) {
    if (idx[i] == target) {
      found = true;
      break;
    }
  }
  *interior_idx = i;
  return found;
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
  Mesh::IndexType startidx = 0;
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

////////////////////////////////////////////////////////////////////////////////

MeshTriVert::MeshTriVert(MeshTriangle* t, Mesh::IndexType idx)
    : tri(t), interior_idx(idx % 3) {}

Mesh::IndexType MeshTriVert::Idx() const { return tri->idx[interior_idx]; }

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

  Mesh::IndexType other_interior_idx;
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

  Mesh::IndexType other_interior_idx;
  return other->HasIdx(v.Idx(), &other_interior_idx);
}
}  // namespace ink
