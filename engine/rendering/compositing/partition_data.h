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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_PARTITION_DATA_H_
#define INK_ENGINE_RENDERING_COMPOSITING_PARTITION_DATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include "third_party/absl/strings/substitute.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

// Partition describing a set of scene elements
struct PartitionData {
  typedef uint32_t ParId;
  static const ParId invalid_partition = 0;

  const ParId id;
  const SceneGraph::GroupedElementsList elements;

  PartitionData() : id(invalid_partition) {}
  PartitionData(ParId id, SceneGraph::GroupedElementsList elements)
      : id(id), elements(elements) {}
  PartitionData& operator=(const PartitionData& other) {
    *const_cast<ParId*>(&id) = other.id;
    *const_cast<SceneGraph::GroupedElementsList*>(&elements) = other.elements;
    return *this;
  }

  std::string ToString() const {
    return absl::Substitute("id: $0, size: $1", id, elements.size());
  }
};

enum class PartitionCacheState {
  // The cache is unable to render any version of the assigned partition.
  // In this state draw calls are noops.
  Incomplete,

  // The partition is ready to draw.
  // The cache is known to represent a different camera than whatever the
  // latest call had provided.
  OutOfDate,

  // The partition is ready to draw.
  // The cache is known to represent the exact camera requested in the last
  // call.
  Complete
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_COMPOSITING_PARTITION_DATA_H_
