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

#ifndef INK_ENGINE_GEOMETRY_TESS_CDREFINEMENT_H_
#define INK_ENGINE_GEOMETRY_TESS_CDREFINEMENT_H_

#include <cstdint>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/inlined_vector.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/mesh_triangle.h"
namespace ink {

// Constrained Deluany Refinement via the Lawson Flip Algo
// https://www.cise.ufl.edu/~ungor/delaunay/delaunay/node5.html
class CDR {
 public:
  explicit CDR(Mesh* mesh);
  void RefineMesh();

 private:
  void InitData();
  uint32_t GetTrisForSeg(MeshTriSegment seg, MeshTriangle** t1,
                         MeshTriangle** t2);

  uint32_t RemoveTriFromMap(const MeshTriangle* t);
  void AddTriToMap(const MeshTriangle* t, uint32_t idx);

  bool ShouldFlip(MeshTetrahedron trh);
  bool ShouldFlip_Circle(MeshTetrahedron trh);
  // Returns false if flip failed due to bad winding.
  bool Flip(MeshTetrahedron trh);

 private:
  Mesh* mesh_;
  MeshTriangle* tris_;
  uint32_t ntris_;
  absl::flat_hash_map<MeshTriSegment, absl::InlinedVector<uint32_t, 2>,
                      MeshTriSegmentHasher>
      seg_to_tri_;

  // The following are initialized by InitData. It is assumed all segments
  // only appear once in the seg_stack_.
  std::vector<MeshTriSegment> seg_stack_;
  // If the segment is in the stack, in_seg_stack_[seg] should be true.
  // Otherwise, it should be false. This will be used to ensure elements in
  // seg_stack_ are unique as we explore the segment space.
  absl::flat_hash_map<MeshTriSegment, bool, MeshTriSegmentHasher> in_seg_stack_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_TESS_CDREFINEMENT_H_
