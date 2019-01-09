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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_MOCK_SPATIAL_INDEX_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_MOCK_SPATIAL_INDEX_H_

#include "testing/base/public/gmock.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/spatial/spatial_index.h"

namespace ink {
namespace spatial {

class MockSpatialIndex : public SpatialIndex {
 public:
  MOCK_CONST_METHOD2(Intersects, bool(const Rect& region,
                                      const glm::mat4& region_to_object));
  MOCK_CONST_METHOD2(Intersection,
                     absl::optional<Rect>(const Rect& region,
                                          const glm::mat4& object_to_world));
  MOCK_CONST_METHOD1(Mbr, Rect(const glm::mat4& object_to_world));
  MOCK_CONST_METHOD0(DebugMesh, Mesh());
};

}  // namespace spatial
}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_SPATIAL_MOCK_SPATIAL_INDEX_H_
