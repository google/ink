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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_BACKGROUND_STATE_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_BACKGROUND_STATE_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"

namespace ink {

class ImageBackgroundState {
 public:
  ImageBackgroundState(const TextureInfo& bg_texture,
                       const Rect& first_instance_world_coords);

  // Maps the vertex coordinates from "imageMesh" to the texture space of the
  // background.  Note that for repeated textures, texture coordinates can fall
  // outside of [0,1].
  glm::mat4 WorldToUV() const;

  // The world coordinates that the background image will be drawn to. This
  // usually will exactly match the page bounds, drawing the image to fill the
  // full page. For repeating backgrounds this represents where the "first"
  // instance is (e.g. if the page width is 100 and first_instance width is 10,
  // and TextureParams::wrap_x is set to TextureWrap::Repeat, then it will be
  // repeated 10 time horizontally across the screen).
  Rect FirstInstanceWorldCoords() const;

  bool HasFirstInstanceWorldCoords() const;

  const TextureInfo& TextureHandle() const;

  void SetFirstInstanceWorldCoords(Rect first_instance_world_coords);

  blit_attrs::FilterEffect ImageFilterEffect() const;

  void SetImageFilterEffect(blit_attrs::FilterEffect effect);

 private:
  Rect first_instance_world_coords_;
  Mesh mesh_;
  blit_attrs::FilterEffect image_filter_effect_;
};

///////////////////////////////////////////////////////////////////////////////

// Stores the current background, either a color or an image.  Background images
// are automatically evicted from the TextureManager once they are no longer the
// current background.
class BackgroundState {
 public:
  BackgroundState();

  // Disallow copy and assign.
  BackgroundState(const BackgroundState&) = delete;
  BackgroundState& operator=(const BackgroundState&) = delete;

  void SetToImage(TextureManager* texture_manager,
                  const TextureInfo& bg_texture,
                  const Rect& first_instance_world_coords);

  // color should be in premultiplied alpha format.
  void SetToColor(TextureManager* texture_manager, const glm::vec4& color);

  // Set the background color to match the OOB color.
  void SetToOutOfBoundsColor(TextureManager* texture_manager);

  // If background is currently an image, return true and sets
  // "backgroundImage", otherwise returns false (background is currently a
  // color).
  bool GetImage(ImageBackgroundState** background_image);

  // Returns true if the background is currently an image.
  bool IsImage() const;

  // Returns true if the background is an image and the provided TextureManager
  // has the data needed to draw that image.
  bool IsImageAndReady(TextureManager* texture_manager) const;

  glm::vec4 GetColor();

  void SetOutOfBoundsColor(glm::vec4 out_of_bounds_color);
  glm::vec4 GetOutOfBoundsColor();

  void ClearImage(TextureManager* texture_manager);

  void SetToDefaultColor(TextureManager* texture_manager);

 private:
  enum class ColorMode { BACKGROUND_COLOR, OUT_OF_BOUNDS_COLOR };
  ColorMode mode_;

  // The background is given by "backgroundImage", unless null, then
  // "backgroundColor" is used.
  std::unique_ptr<ImageBackgroundState> background_image_;

  // premultiplied alpha format.
  glm::vec4 background_color_{0, 0, 0, 0};

  // The background of the non-drawable area.
  glm::vec4 out_of_bounds_color_{0, 0, 0, 0};
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_BACKGROUND_STATE_H_
