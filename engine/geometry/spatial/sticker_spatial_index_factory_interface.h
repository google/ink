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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_INTERFACE_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_INTERFACE_H_

#include <memory>

#include "ink/engine/geometry/spatial/spatial_index.h"

namespace ink {

class SceneGraph;
struct ProcessedElement;

namespace spatial {

// This interface is meant to be used for generating spatial indexes for meshes
// that represent stickers. For selectable stickers, the spatial index created
// should respect transparencies in the sticker texture.
class StickerSpatialIndexFactoryInterface {
 public:
  virtual ~StickerSpatialIndexFactoryInterface() {}

  // The scene graph, which holds a strong reference to this, is responsible for
  // setting this to itself upon construction, and then setting it to nullptr
  // upon destruction.
  virtual void SetSceneGraph(SceneGraph *scene_graph) = 0;

  virtual std::shared_ptr<SpatialIndex> CreateSpatialIndex(
      const ProcessedElement &line) = 0;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_INTERFACE_H_
