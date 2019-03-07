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

#include "ink/engine/geometry/spatial/spatial_index.h"

namespace ink {
namespace spatial {

bool SpatialIndex::IntersectsSpatialIndex(
    const SpatialIndex& other, const glm::mat4& this_to_other) const {
  if (!GetTriRTree() || !other.GetTriRTree()) {
    return false;
  }
  return GetTriRTree()->Intersects(*other.GetTriRTree(), this_to_other);
}

}  // namespace spatial
}  // namespace ink
