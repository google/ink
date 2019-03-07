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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_SPATIAL_INDEX_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_SPATIAL_INDEX_H_

#include "third_party/absl/types/optional.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/geometry/spatial/rtree.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace spatial {

class SpatialIndex {
 public:
  virtual ~SpatialIndex() {}

  // Returns true if the indexed object intersects the given region. The
  // region_to_object matrix contains the transformation from region-coordinates
  // to object-coordinates.
  // Note that while the region is often defined in world-coordinates, this is
  // not required to be the case. As such, even if your implementation is aware
  // of world-coordinates, it is not safe to ignore the transform.
  virtual bool Intersects(const Rect& region,
                          const glm::mat4& region_to_object) const = 0;

  // Returns true if this spatial index intersects other's spatial index. The
  // this_to_other matrix contains the transformation from *this object
  // coordinates to other's object coordinates.
  // Implementation note: This will be using the private function:
  //   GetTriRTree()
  // If an RTree is not present, this will return false.
  bool IntersectsSpatialIndex(const SpatialIndex& other,
                              const glm::mat4& this_to_other) const;

  // Returns the intersection rect between the elements in this spatial index
  // and the region provided. Note that if Intersection() would return
  // absl::nullopt, then Intersects() should return false.
  // The rect returned here may be smaller than the rect returned by
  // intersection this region with the Mbr() of the spatial index.
  // The returned rect is in object space.
  // Warning: This does not preserve area if the incoming
  // region * region_to_object is not axis aligned.
  virtual absl::optional<ink::Rect> Intersection(
      const Rect& region, const glm::mat4& region_to_object) const = 0;

  // The bounding Rect of the entire indexed object after the given
  // object-to-world transform has been applied.
  virtual Rect Mbr(const glm::mat4& object_to_world) const = 0;

  // Return the axis aligned bounding Rect in object coordinates. This is
  // equivalent to calling Mbr(glm::mat4{1}), but may be faster.
  virtual Rect ObjectMbr() const = 0;

  virtual Mesh DebugMesh() const = 0;

 private:
  // For now, we expect all subclasses to work against an RTree of
  // triangles. In the future, a subclass may have a different type. We
  // should add an accessor for the raw RTree here and use it in the
  // IntersectsSpatialIndex implementation.
  virtual const RTree<geometry::Triangle>* const GetTriRTree() const = 0;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_SPATIAL_INDEX_H_
