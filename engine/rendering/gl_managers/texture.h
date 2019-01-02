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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_H_

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/gl.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/gl_managers/nine_patch_info.h"
#include "ink/engine/rendering/gl_managers/texture_params.h"
#include "ink/engine/util/security.h"

namespace ink {

class Texture {
 public:
  explicit Texture(ion::gfx::GraphicsManagerPtr gl);
  Texture(ion::gfx::GraphicsManagerPtr gl, glm::ivec2 size, GLuint gl_id,
          TextureParams bind_params);

  // Disallow copy and assign.
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  virtual ~Texture();

  // Copies image byte data in "clientBitmap" to gpu.  Safe to call extra times.
  virtual void Load(const ClientBitmap& client_bitmap, TextureParams params);

  // Removes image byte data from gpu.  Safe to call extra times.
  void Unload();

  // Wraps glBindTexture.
  virtual void Bind(GLuint gl_texture_location) const;

  glm::ivec2 size() const { return size_; }
  GLuint TextureId() const { return gl_id_; }
  S_WARN_UNUSED_RESULT bool getNinePatchInfo(
      NinePatchInfo* nine_patch_info) const;

  bool UseForHitTesting() const { return texture_params_.use_for_hit_testing; }

  bool IsValid() const;

  bool GetPixels(GPUPixels* pixels) const;

  Texture& operator=(Texture&& move_from);

 private:
  ion::gfx::GraphicsManagerPtr gl_;

  // The width and height of the texture in pixels.
  glm::ivec2 size_{0, 0};

  NinePatchInfo nine_patch_info_;

  // Result of glGenTextures, or "kBadGLHandle" if not yet assigned.
  GLuint gl_id_;

  TextureParams texture_params_;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_H_
