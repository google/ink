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

  // The bounding Rect of the entire indexed object.
  virtual Rect Mbr(const glm::mat4& object_to_world) const = 0;

  virtual Mesh DebugMesh() const = 0;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_SPATIAL_INDEX_H_
