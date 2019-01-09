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

#ifndef INK_ENGINE_GEOMETRY_TESS_TESSELLATOR_H_
#define INK_ENGINE_GEOMETRY_TESS_TESSELLATOR_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/flat_hash_set.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/gl.h"
#include "ink/engine/util/security.h"

namespace ink {

// Converts an outline to a Tessellation.
//
// An example:
//
//   bool MakeAMesh(const FatLine &fat_line, Mesh *mesh) {
//     Tessellator tess;
//     if (tess->Tessellate(fat_line, true) && tess->HasMesh()) {
//       *mesh = tess->mesh_;
//       return true;
//     }
//     return false;
//   }
//
// This class is mainly a C++ wrapper to gluTess* which necessitates
// making certain internals public. Do not use those internals.
//
class Tessellator {
 public:
  Tessellator();

  // Disallow copy and assign.
  Tessellator(const Tessellator&) = delete;
  Tessellator& operator=(const Tessellator&) = delete;

  ~Tessellator();

  // Tessellates the poly specified by the points in the pts vector.
  // The points are injected in order.
  // Returns true iff the tessellation was successful.
  // The resulting mesh can be found in tess->mesh_.
  S_WARN_UNUSED_RESULT bool Tessellate(const std::vector<Vertex>& pts);

  // Tessellates the poly specified by the edges in the edges vector.
  // The points in each edge are injected in order, and the edges are visited
  // in order.
  // Returns true iff the tessellation was successful.
  // The resulting mesh can be found in tess->mesh_.
  S_WARN_UNUSED_RESULT bool Tessellate(
      const std::vector<std::vector<Vertex>>& edges);

  // Convenience overloads for geometry::Polygon.
  S_WARN_UNUSED_RESULT bool Tessellate(const geometry::Polygon& polygon);
  S_WARN_UNUSED_RESULT bool Tessellate(
      const std::vector<geometry::Polygon>& polygons);

  // Tessellates the poly described by line. This includes the start cap,
  // the forward_line, the backward_line, and (if end_cap == true) the end_cap.
  S_WARN_UNUSED_RESULT bool Tessellate(const FatLine& line, bool end_cap);

  // Clears out the vertex data of the result mesh and any working data
  // structures. Clear() should be called in between calls to Tessellate.
  //
  // Note in particular that this does not reset the mesh transformation matrix
  // or shader metadata.
  void ClearGeometry();

  // Returns true iff the result mesh contains a non-empty mesh.
  //
  bool HasMesh() { return !mesh_.idx.empty(); }

 private:
  void Setup();
  template <typename T>
  void Inject(T from, T to);

 public:
  bool tess_mid_pts_;

  // The result mesh from a call to Tessellate().
  // Always check HasMesh() before using this value.
  Mesh mesh_;

 public:
  // Not really public, but we use these fields from free functions in
  // tessellation.cc (which have to be free functions because GLUtesselator is a
  // C API).
  //
  // DO NOT USE any of these outside of tessellator.cc
  void SetErrorFlag() { did_error_ = true; }
  std::vector<std::unique_ptr<Vertex>> temp_verts_;
  absl::flat_hash_map<glm::vec2, uint32_t, Vec2Hasher> pt_to_idx_;
  absl::flat_hash_set<glm::vec2, Vec2Hasher> combined_verts_;

 private:
  GLUtesselator* glu_tess_;
  bool did_error_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_TESS_TESSELLATOR_H_
