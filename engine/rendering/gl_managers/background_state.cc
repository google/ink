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

#include "ink/engine/rendering/gl_managers/background_state.h"

#include <string>

#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

using glm::vec2;
using glm::vec4;

static const vec4 kDefaultDocumentBgColor(0.98, 0.98, 0.98, 1.0);
static const vec4 kDefaultOutOfBoundsColor(0.8, 0.8, 0.8, 1.0);

ImageBackgroundState::ImageBackgroundState(
    const TextureInfo& bg_texture, const Rect& first_instance_world_coords)
    : first_instance_world_coords_(first_instance_world_coords),
      image_filter_effect_(blit_attrs::FilterEffect::None) {
  mesh_.texture.reset(new TextureInfo(bg_texture));
}

glm::mat4 ImageBackgroundState::WorldToUV() const {
  return first_instance_world_coords_.CalcTransformTo(Rect(0, 0, 1, 1), true);
}

Rect ImageBackgroundState::FirstInstanceWorldCoords() const {
  return first_instance_world_coords_;
}

bool ImageBackgroundState::HasFirstInstanceWorldCoords() const {
  return first_instance_world_coords_.Area() > 0;
}

void ImageBackgroundState::SetFirstInstanceWorldCoords(
    Rect first_instance_world_coords) {
  first_instance_world_coords_ = first_instance_world_coords;
}

const TextureInfo& ImageBackgroundState::TextureHandle() const {
  return *mesh_.texture;
}

blit_attrs::FilterEffect ImageBackgroundState::ImageFilterEffect() const {
  return image_filter_effect_;
}

void ImageBackgroundState::SetImageFilterEffect(
    blit_attrs::FilterEffect effect) {
  image_filter_effect_ = effect;
}

///////////////////////////////////////////////////////////////////////////////

BackgroundState::BackgroundState()
    : mode_(ColorMode::BACKGROUND_COLOR),
      background_color_(kDefaultDocumentBgColor),
      out_of_bounds_color_(kDefaultOutOfBoundsColor) {}

void BackgroundState::SetToImage(TextureManager* texture_manager,
                                 const TextureInfo& bg_texture,
                                 const Rect& first_instance_world_coords) {
  // This is a workaround needed to prevent the following:
  // If the user sets the same texture twice, it would cause an eviction.  We
  // need a better overall eviction strategy.
  if (background_image_ &&
      background_image_->TextureHandle().uri != bg_texture.uri) {
    ClearImage(texture_manager);
  }
  background_image_.reset(
      new ImageBackgroundState(bg_texture, first_instance_world_coords));
}

void BackgroundState::SetToColor(TextureManager* texture_manager,
                                 const glm::vec4& color) {
  ClearImage(texture_manager);
  background_color_ = color;
  mode_ = ColorMode::BACKGROUND_COLOR;
}

void BackgroundState::ClearImage(TextureManager* texture_manager) {
  if (background_image_) {
    texture_manager->Evict(background_image_->TextureHandle());
    background_image_.reset(nullptr);
  }
}

bool BackgroundState::IsImage() const { return background_image_ != nullptr; }

bool BackgroundState::IsImageAndReady(TextureManager* texture_manager) const {
  return background_image_ &&
         texture_manager->HasTexture(background_image_->TextureHandle());
}

bool BackgroundState::GetImage(ImageBackgroundState** background_mesh) {
  *background_mesh = background_image_.get();
  return *background_mesh;
}

glm::vec4 BackgroundState::GetColor() {
  return mode_ == ColorMode::OUT_OF_BOUNDS_COLOR ? out_of_bounds_color_
                                                 : background_color_;
}

glm::vec4 BackgroundState::GetOutOfBoundsColor() {
  return out_of_bounds_color_;
}

void BackgroundState::SetOutOfBoundsColor(glm::vec4 out_of_bounds_color) {
  out_of_bounds_color_ = out_of_bounds_color;
}

void BackgroundState::SetToDefaultColor(TextureManager* texture_manager) {
  SetToColor(texture_manager, kDefaultDocumentBgColor);
}

void BackgroundState::SetToOutOfBoundsColor(TextureManager* texture_manager) {
  ClearImage(texture_manager);
  mode_ = ColorMode::OUT_OF_BOUNDS_COLOR;
}
}  // namespace ink
