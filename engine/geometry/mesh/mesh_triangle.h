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

#ifndef INK_ENGINE_GEOMETRY_MESH_MESH_TRIANGLE_H_
#define INK_ENGINE_GEOMETRY_MESH_MESH_TRIANGLE_H_

#include <cstddef>
#include <string>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

struct MeshTriSegment {
  Mesh::IndexType idx[2];

  MeshTriSegment();
  MeshTriSegment(Mesh::IndexType f, Mesh::IndexType t);

  Mesh::IndexType LowIdx() const;
  Mesh::IndexType HighIdx() const;

  bool operator==(const MeshTriSegment& other) const;
  bool operator<(const MeshTriSegment& other) const;

  std::string ToString() const;
};

struct MeshTriSegmentHasher {
  size_t operator()(const ink::MeshTriSegment& s) const {
    return std::hash<Mesh::IndexType>()(s.idx[0] + s.idx[1]);
  }
};

////////////////////////////////////////////////////////////////////////////////

struct MeshTriangle {
  Mesh::IndexType idx[3];

  bool Valid() const;
  bool HasIdx(Mesh::IndexType target, Mesh::IndexType* interior_idx) const;
  MeshTriSegment Segment(int n) const;

  std::string ToString() const;

  bool operator==(const MeshTriangle& other) const;

  // returns true if this has the same vertices as other
  // (winding order doesn't matter)
  bool RoughEquals(const MeshTriangle& other) const;

  // returns if this is the same triangle (start offset doesn't
  // (matter, winding order does matter)
  bool StrictEquals(const MeshTriangle& other) const;
};

////////////////////////////////////////////////////////////////////////////////

struct MeshTriVert {
  MeshTriangle* tri;
  Mesh::IndexType interior_idx;

  MeshTriVert(MeshTriangle* t, Mesh::IndexType idx);
  Mesh::IndexType Idx() const;
  MeshTriVert Advance() const;
};

////////////////////////////////////////////////////////////////////////////////

struct MeshTetrahedron {
  MeshTriangle *t1, *t2;

  MeshTetrahedron();
  MeshTetrahedron(MeshTriangle* t1, MeshTriangle* t2);
  MeshTriVert Advance(MeshTriVert v) const;
  bool IsShared(MeshTriVert v) const;
  bool Valid() const;
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_MESH_MESH_TRIANGLE_H_
