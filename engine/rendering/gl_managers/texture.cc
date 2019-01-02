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

#include "ink/engine/rendering/gl_managers/texture.h"

#include <sys/types.h>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

namespace {
void BindFromParams(const ion::gfx::GraphicsManagerPtr& gl,
                    const TextureParams& params) {
  GLint min_filter =
      ink::GlTextureFilter(params.minify_filter, params.use_mipmap);

  // Note: mipmap filters are only used for min filters, never for mag filters.
  GLint mag_filter = ink::GlTextureFilter(params.magnify_filter, false);

  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    ink::GlTextureWrap(params.wrap_x));
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    ink::GlTextureWrap(params.wrap_y));
}
}  // anonymous namespace

using ink::kBadGLHandle;

Texture::Texture(ion::gfx::GraphicsManagerPtr gl)
    : Texture(gl, glm::ivec2(0, 0), kBadGLHandle, TextureParams()) {}

Texture::Texture(ion::gfx::GraphicsManagerPtr gl, glm::ivec2 size, GLuint gl_id,
                 TextureParams bind_params)
    : gl_(gl), size_(size), gl_id_(gl_id), texture_params_(bind_params) {}

Texture::~Texture() { Unload(); }

Texture& Texture::operator=(Texture&& from) {
  using std::swap;  // go/using-std-swap
  swap(size_, from.size_);
  swap(gl_id_, from.gl_id_);
  swap(texture_params_, from.texture_params_);
  swap(gl_, from.gl_);
  return *this;
}

void Texture::Load(const ClientBitmap& client_bitmap, TextureParams params) {
  if (params.is_nine_patch) {
    nine_patch_info_ = NinePatchInfo(client_bitmap);
  }

  texture_params_ = params;
  size_ = glm::ivec2(client_bitmap.sizeInPx().width,
                     client_bitmap.sizeInPx().height);

  // load the img to the graphics driver
  gl_->GenTextures(1, &gl_id_);
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);

  auto gl_tex_format = GlTextureFormat(client_bitmap.format());
  gl_->PixelStorei(GL_UNPACK_ALIGNMENT, gl_tex_format.unpack_alignment);
  gl_->TexImage2D(GL_TEXTURE_2D, 0, gl_tex_format.internal_format, size_.x,
                  size_.y, 0, gl_tex_format.format, gl_tex_format.type,
                  client_bitmap.imageByteData());

  if (texture_params_.use_mipmap) {
    EXPECT(size_.x == size_.y && util::IsPowerOf2(size_.x));
    gl_->GenerateMipmap(GL_TEXTURE_2D);
  }
  BindFromParams(gl_, texture_params_);

  GLASSERT_NO_ERROR(gl_);
}

void Texture::Bind(GLuint gl_texture_location) const {
  ASSERT(gl_id_ != kBadGLHandle);
  gl_->ActiveTexture(gl_texture_location);
  gl_->BindTexture(GL_TEXTURE_2D, gl_id_);
  GLASSERT_NO_ERROR(gl_);
}

void Texture::Unload() {
  if (gl_id_ != kBadGLHandle) {
    size_ = glm::ivec2(0, 0);
    gl_->DeleteTextures(1, &gl_id_);
    gl_id_ = kBadGLHandle;
  }
}

bool Texture::getNinePatchInfo(NinePatchInfo* nine_patch_info) const {
  if (!nine_patch_info_.is_nine_patch) {
    return false;
  }

  *nine_patch_info = nine_patch_info_;
  return true;
}

bool Texture::IsValid() const {
  return size_.x > 0 && size_.y > 0 && gl_id_ != kBadGLHandle;
}

bool Texture::GetPixels(GPUPixels* pixels) const {
  ASSERT(pixels != nullptr);
  if (!IsValid()) return false;

  pixels->Resize(size());
  GLuint frame_buffer;
  gl_->GenFramebuffers(1, &frame_buffer);
  gl_->BindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
  gl_->FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, TextureId(), 0);
  gl_->ReadPixels(0, 0, size().x, size().y, GL_RGBA, GL_UNSIGNED_BYTE,
                  pixels->RawData().data());
  gl_->DeleteFramebuffers(1, &frame_buffer);
  GLASSERT_NO_ERROR(gl_);

  return true;
}
}  // namespace ink
