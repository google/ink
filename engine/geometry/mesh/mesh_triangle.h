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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "third_party/glm/glm/gtx/norm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

struct MeshTriSegment {
  uint16_t idx[2];

  MeshTriSegment();
  MeshTriSegment(uint16_t f, uint16_t t);

  uint16_t LowIdx() const;
  uint16_t HighIdx() const;
  float Length2(const Mesh& mesh) const;

  // Used by longform
  bool HasIdx(uint16_t test_idx) const;
  bool SharedIdx(const MeshTriSegment& other, uint16_t* shared) const;
  uint16_t OtherIdx(uint16_t shared_idx) const;
  Segment PtSegment(const Mesh& mesh) const;
  float Length(const Mesh& mesh) const;
  Vertex Midpt(const Mesh& mesh) const;

  bool operator==(const MeshTriSegment& other) const;
  bool operator<(const MeshTriSegment& other) const;

  std::string ToString() const;
};

struct MeshTriSegmentHasher {
  size_t operator()(const ink::MeshTriSegment& s) const {
    return std::hash<uint16_t>()(s.idx[0] + s.idx[1]);
  }
};

////////////////////////////////////////////////////////////////////////////////

struct MeshTriangle {
  uint16_t idx[3];

  bool Valid() const;
  bool HasIdx(uint16_t target, uint32_t* interior_idx) const;
  MeshTriSegment Segment(int n) const;

  // Used by longform
  float Area(const Mesh& mesh) const;
  Vertex Centroid(const Mesh& mesh) const;
  void AppendToMesh(Mesh* mesh) const;
  bool OtherIdx(const MeshTriSegment& segment, uint16_t* other) const;
  bool ContainsPt(const Mesh& mesh, glm::vec2 point) const;

  std::string ToString() const;

  bool operator==(const MeshTriangle& other) const;

  // returns if this has the same verticies as other
  // (winding order doesn't matter)
  bool RoughEquals(const MeshTriangle& other) const;

  // returns if this is the same triangle (start offset doesn't
  // (matter, winding order does matter)
  bool StrictEquals(const MeshTriangle& other) const;
};

////////////////////////////////////////////////////////////////////////////////

struct MeshTriVert {
  MeshTriangle* tri;
  uint16_t interior_idx;

  MeshTriVert(MeshTriangle* t, uint16_t idx);
  uint16_t Idx() const;
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
