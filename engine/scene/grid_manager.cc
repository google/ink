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

#include "ink/engine/scene/grid_manager.h"

#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/gl_managers/texture.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/gl_managers/texture_params.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

GridManager::GridManager(std::shared_ptr<GLResourceManager> gl_resources)
    : gl_resources_(std::move(gl_resources)),
      renderer_(gl_resources_),
      size_world_(50) {}

void GridManager::SetGrid(const proto::GridInfo& grid_info) {
  grid_texture_ = absl::make_unique<TextureInfo>(grid_info.uri());
  color_multiplier_ = UintToVec4RGBA(grid_info.rgba_multiplier());
  size_world_ = grid_info.size_world();
  if (grid_info.has_origin()) {
    origin_ = glm::vec2(grid_info.origin().x(), grid_info.origin().y());
  } else {
    origin_ = glm::vec2(0, 0);
  }
}

void GridManager::ClearGrid() {
  if (grid_texture_) {
    gl_resources_->texture_manager->Evict(*grid_texture_);
    grid_texture_.reset();
  }
}

void GridManager::Draw(const Camera& cam, FrameTimeS draw_time) const {
  if (!grid_texture_) return;

  Texture* texture = nullptr;
  if (gl_resources_->texture_manager->GetTexture(*grid_texture_, &texture)) {
    renderer_.Draw(
        cam, texture, blit_attrs::BlitColorOverride(color_multiplier_),
        cam.WorldRotRect(),
        RotRect(origin_ + .5f * size_world_, glm::vec2(size_world_), 0)
            .InvertYAxis());
  }
}

}  // namespace ink
