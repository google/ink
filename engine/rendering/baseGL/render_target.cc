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

#include "ink/engine/rendering/baseGL/render_target.h"

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/baseGL/msaa.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

RenderTarget::RenderTarget(std::shared_ptr<GLResourceManager> gl_resources)
    : RenderTarget(gl_resources, AntialiasingStrategy::kNone,
                   TextureMapping::Nearest) {}
RenderTarget::RenderTarget(std::shared_ptr<GLResourceManager> gl_resources,
                           AntialiasingStrategy aastrategy,
                           TextureMapping filter,
                           RenderTargetFormat internal_format)
    : gl_resources_(gl_resources),
      renderer_(gl_resources),
      size_(0, 0),
      fbo_(0),
      rbo_(0),
      tex_(gl_resources_->gl),
      aastrategy_(aastrategy),
      filter_(filter),
      internal_format_(internal_format) {}

RenderTarget::~RenderTarget() { DeleteBuffers(); }

void RenderTarget::Clear(glm::vec4 color) {
  SLOG(SLOG_GL_STATE, "Clearing to $0.", color);
  Bind();
  const auto& gl = gl_resources_->gl;
  gl->ClearColor(color.x, color.y, color.z, color.w);
  gl->Clear(GL_COLOR_BUFFER_BIT);
}

void RenderTarget::Bind() const {
  SLOG(SLOG_GL_STATE, "binding $0", *this);
  if (msaa()) {
    EXPECT(fbo_ != 0 && rbo_ != 0);
  } else {
    EXPECT(fbo_ != 0 && tex_.IsValid());
  }
  const auto& gl = gl_resources_->gl;
  gl->BindFramebuffer(GL_FRAMEBUFFER, fbo_);

  SLOG(SLOG_GL_STATE, "$0 setting glViewport to $1", *this, size_);
  gl->Viewport(0, 0, size_.x, size_.y);
  gl->Enable(GL_BLEND);
  gl->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  GLASSERT_NO_ERROR(gl_resources_->gl);
}

void RenderTarget::Blit(RenderTarget* destination,
                        absl::optional<Rect> area) const {
  Rect box = area.value_or(Rect({0, 0}, size_));
  // We only allow copying to another rendertarget that is the same size.
  ASSERT(size_ == destination->size_);
  destination->Clear();

  if (msaa()) {
    // If we are an MSAA RenderTarget we should only be blitted to another
    // RenderTarget that matches in size and has MSAA off to hit the fast path.
    ASSERT(!destination->msaa());
    // Resolve this target's msaa buffer directly into destination's
    // framebuffer.
    msaa::ResolveMultisampleFramebuffer(*gl_resources_, destination->fbo_, fbo_,
                                        box);
  } else {
    Camera blit_camera;
    blit_camera.SetScreenDim(destination->GetSize());
    blit_camera.SetPosition(.5f * glm::vec2(destination->GetSize()),
                            destination->GetSize(), 0);
    Draw(blit_camera, blit_attrs::Blit(), RotRect(box), RotRect(box));
  }
  GLASSERT_NO_ERROR(gl_resources_->gl);
}

void RenderTarget::Draw(const Camera& cam, const blit_attrs::BlitAttrs& attrs,
                        const RotRect& buffer_source,
                        const RotRect& world_dest) const {
  if (GetSize().x == 0 || GetSize().y == 0 || fbo_ == 0 || !tex_.IsValid()) {
    SLOG(SLOG_ERROR, "Tried to Draw invalid rendertarget");
    return;
  }
  SLOG(SLOG_DRAWING, "$0 blitting", *this);

  auto buffer_to_uv = Bounds().CalcTransformTo(Rect({0, 0}, {1, 1}));
  auto dest_to_source = world_dest.CalcTransformTo(buffer_source);
  renderer_.Draw(cam, &tex_, attrs, world_dest, buffer_to_uv * dest_to_source);
}

void RenderTarget::DeleteBuffers() {
  const auto& gl = gl_resources_->gl;
  if (rbo_ != 0) {
    gl->DeleteRenderbuffers(1, &rbo_);
    rbo_ = 0;
  }
  if (fbo_ != 0) {
    gl->DeleteFramebuffers(1, &fbo_);
    fbo_ = 0;
  }
  size_ = glm::ivec2(0, 0);
  tex_.Unload();
}

void RenderTarget::Resize(glm::ivec2 size) { GenBuffers(size); }

void RenderTarget::GenBuffers(glm::ivec2 size) {
  GLuint fbo = 0;
  GLuint rbo = 0;
  GLuint tex = 0;

  if (size == size_) return;

  SLOG(SLOG_GPU_OBJ_CREATION, "$0 regenerating fbo (new size $0)", *this, size);

  GLASSERT_NO_ERROR(gl_resources_->gl);
  DeleteBuffers();
  GLASSERT_NO_ERROR(gl_resources_->gl);

  // gen msaa buffers
  if (msaa()) {
    SLOG(SLOG_GL_STATE, "Generating MSAA buffers.");
    bool did_generate_buffer = false;
    switch (internal_format_) {
      case RenderTargetFormat::kRGB8:
        did_generate_buffer =
            msaa::GenMSAABuffers(*gl_resources_, &fbo, &rbo, size, GL_RGB8);
        break;
      case RenderTargetFormat::kBest:
      default:
        did_generate_buffer =
            msaa::GenMSAABuffers(*gl_resources_, &fbo, &rbo, size);
        break;
    }
    if (!did_generate_buffer) {
      SLOG(SLOG_ERROR, "Failed to generate MSAA buffers");
      aastrategy_ = AntialiasingStrategy::kNone;  // try and generate non-msaa
                                                  // as a fallback
    } else {
      GLEXPECT_NO_ERROR(gl_resources_->gl);
      fbo_ = fbo;
      rbo_ = rbo;
      size_ = size;
      return;
    }
  }

  // If we didn't generate the msaa buffers, generate non-msaa ones.
  const auto& gl = gl_resources_->gl;
  SLOG(SLOG_GL_STATE, "Generating non-msaa buffers.");
  GLASSERT_NO_ERROR(gl_resources_->gl);
  gl->GenFramebuffers(1, &fbo);
  gl->BindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLASSERT_NO_ERROR(gl_resources_->gl);

  gl->GenTextures(1, &tex);
  gl->BindTexture(GL_TEXTURE_2D, tex);
  GLASSERT_NO_ERROR(gl_resources_->gl);

  GLint min_mag_filter = GlTextureFilter(filter_, false /* mipmap */);

  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mag_filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, min_mag_filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GLASSERT_NO_ERROR(gl_resources_->gl);
  gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
  gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex, 0);

  TextureParams params;
  params.minify_filter = filter_;
  params.magnify_filter = filter_;
  Texture new_texture(gl, size, tex, params);
  tex_ = std::move(new_texture);

  GLEXPECT_NO_ERROR(gl);
  GLEXPECT(gl, msaa::IsFramebufferComplete(*gl_resources_));

  fbo_ = fbo;
  size_ = size;
}

std::string RenderTarget::ToString() const {
  return Substitute("rendertarget $0 size:$1, msaa:$2", HexStr(fbo_), size_,
                    msaa());
}

bool RenderTarget::msaa() const {
  return aastrategy_ == AntialiasingStrategy::kMSAA;
}

void RenderTarget::TransferTexture(Texture* out) {
  *out = std::move(tex_);
  DeleteBuffers();
}

bool RenderTarget::GetPixels(GPUPixels* pixels) const {
  pixels->Resize(size_);
  CaptureRawData(pixels->RawData().data());
  return true;
}

void RenderTarget::CaptureRawData(void* pixel_buffer_out) const {
  EXPECT(!msaa());
  Bind();
  gl_resources_->gl->ReadPixels(0, 0, size_.x, size_.y, GL_RGBA,
                                GL_UNSIGNED_BYTE, pixel_buffer_out);
  GLEXPECT_NO_ERROR(gl_resources_->gl);
}

}  // namespace ink
