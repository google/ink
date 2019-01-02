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

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_BORDER_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_BORDER_H_

#include "third_party/absl/memory/memory.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

class PageBorder : public IDrawable {
 public:
  using SharedDeps =
      service::Dependencies<GLResourceManager, PageBounds, PageManager>;

  PageBorder(std::shared_ptr<GLResourceManager> gl_resources,
             std::shared_ptr<PageBounds> page_bounds,
             std::shared_ptr<PageManager> page_manager);

  void SetTexture(const std::string& uri, float scale) {
    texture_info_ = absl::make_unique<TextureInfo>(uri);
    scale_ = scale;
  }
  void ClearTexture() { texture_info_.reset(); }

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

 private:
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<PageManager> page_manager_;
  MeshRenderer renderer_;

  std::unique_ptr<TextureInfo> texture_info_;
  float scale_ = 1;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_PAGE_PAGE_BORDER_H_
