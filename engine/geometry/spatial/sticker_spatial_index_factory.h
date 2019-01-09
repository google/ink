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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/sticker_spatial_index_factory_interface.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"

namespace ink {
namespace spatial {

class StickerSpatialIndexFactory
    : public StickerSpatialIndexFactoryInterface,
      public TextureListener,
      public std::enable_shared_from_this<StickerSpatialIndexFactory> {
 public:
  using SharedDeps =
      service::Dependencies<settings::Flags, ITaskRunner, GLResourceManager>;

  StickerSpatialIndexFactory(std::shared_ptr<settings::Flags> flags,
                             std::shared_ptr<ITaskRunner> task_runner,
                             std::shared_ptr<GLResourceManager> gl_resources);
  ~StickerSpatialIndexFactory() override;

  void SetSceneGraph(SceneGraph* scene_graph) override {
    scene_graph_ = scene_graph;
  }

  std::shared_ptr<SpatialIndex> CreateSpatialIndex(
      const ProcessedElement& element) override;

  void OnTextureLoaded(const TextureInfo& info) override;
  void OnTextureEvicted(const TextureInfo& info) override;

  void RegisterTextureSpatialIndex(const std::string& texture_uri,
                                   std::shared_ptr<SpatialIndex> index);

 private:
  void ReplaceTextureSpatialIndex(const std::string& texture_uri,
                                  std::shared_ptr<SpatialIndex> index);
  std::shared_ptr<settings::Flags> flags_;
  std::shared_ptr<ITaskRunner> task_runner_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  SceneGraph* scene_graph_;

  std::unordered_map<std::string, std::shared_ptr<SpatialIndex>>
      texture_uri_to_spatial_index_;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_STICKER_SPATIAL_INDEX_FACTORY_H_
