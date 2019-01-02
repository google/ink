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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_TEXTURE_RTREE_CREATOR_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_TEXTURE_RTREE_CREATOR_H_

#include <memory>
#include <string>

#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/spatial_index_factory.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/gl_managers/texture.h"

namespace ink {
namespace spatial {

class TextureRTreeCreator : public Task {
 public:
  TextureRTreeCreator(std::weak_ptr<SpatialIndexFactory> weak_factory,
                      std::string texture_uri, const Texture& texture);

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}
  void Execute() override;
  void OnPostExecute() override;

 private:
  GPUPixels PreprocessTexture() const;

  std::weak_ptr<SpatialIndexFactory> weak_factory_;
  std::string texture_uri_;
  GPUPixels pixels_;
  std::shared_ptr<SpatialIndex> index_;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_TEXTURE_RTREE_CREATOR_H_
