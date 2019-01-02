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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_MESH_RTREE_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_MESH_RTREE_H_

#include <utility>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/spatial/rtree.h"
#include "ink/engine/geometry/spatial/spatial_index.h"

namespace ink {
namespace spatial {

class MeshRTree : public SpatialIndex {
 public:
  explicit MeshRTree(const OptimizedMesh& mesh);

  // Disallow copy and assign.
  MeshRTree(const MeshRTree&) = delete;
  MeshRTree& operator=(const MeshRTree&) = delete;

  Rect Mbr(const glm::mat4& object_to_world) const override;

  bool Intersects(const Rect& region,
                  const glm::mat4& region_to_object) const override;

  Mesh DebugMesh() const override;

 private:
  std::unique_ptr<RTree<geometry::Triangle>> rtree_;
  std::vector<glm::vec2> convex_hull_;
  float mbr_offset_dist_;

  // Cache the result of the last call to Mbr().
  mutable std::unique_ptr<std::pair<glm::mat4, Rect>> cached_mbr_;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_MESH_RTREE_H_
