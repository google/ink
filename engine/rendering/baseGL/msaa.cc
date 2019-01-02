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

#include "ink/engine/rendering/baseGL/msaa.h"

#include <cstdint>
#include <string>

#include "geo/render/ion/portgfx/glheaders.h"
#include "geo/render/ion/portgfx/isextensionsupported.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace msaa {

bool IsFramebufferComplete(const GLResourceManager& resource_manager) {
  GLenum status = resource_manager.gl->CheckFramebufferStatus(GL_FRAMEBUFFER);
  switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
      return true;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      // Choose different formats
      SLOG(SLOG_ERROR,
           "Framebuffer object format is unsupported by the video hardware. "
           "(GL_FRAMEBUFFER_UNSUPPORTED)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      SLOG(SLOG_ERROR,
           "Incomplete attachment. "
           "(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      SLOG(SLOG_ERROR,
           "Incomplete missing attachment. "
           "(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      SLOG(SLOG_ERROR,
           "Incomplete dimensions. "
           "(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      SLOG(SLOG_ERROR,
           "Incomplete draw buffer. "
           "(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      SLOG(SLOG_ERROR,
           "Incomplete read buffer. "
           "(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)");
      return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      SLOG(SLOG_ERROR,
           "Incomplete multisample buffer. "
           "(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)");
      return false;
    default:
      // Programming error; will fail on all hardware
      SLOG(SLOG_ERROR,
           "Some video driver error or programming error occurred. "
           "Framebuffer object status is invalid.");
      return false;
  }
}

void ResolveMultisampleFramebuffer(const GLResourceManager& resource_manager,
                                   GLuint fbo, GLuint fbo_msaa,
                                   const Rect& box) {
  const auto& gl = resource_manager.gl;
  // GL_READ_FRAMEBUFFER and GL_READ_FRAMEBUFFER_APPLE have the same value.
  gl->BindFramebuffer(GL_READ_FRAMEBUFFER, fbo_msaa);
  gl->BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

  // At least one of these features must be present.  If not present, then
  // ShouldUseMSAA should return false as well.
  //
  // Try the ES3 framebuffer blit, often available as an ES2 extension.
  if (gl->IsFeatureAvailable(ion::gfx::GraphicsManager::kFramebufferBlit)) {
    gl->BlitFramebuffer(box.Left(), box.Bottom(), box.Right(), box.Top(),
                        box.Left(), box.Bottom(), box.Right(), box.Top(),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    // Try the Apple-only ES2 extension.
  } else if (gl->IsFeatureAvailable(
                 ion::gfx::GraphicsManager::kMultisampleFramebufferResolve)) {
    Scissor scissor(gl);
    scissor.SetScissor(Camera(), box, CoordType::kScreen);
    gl->ResolveMultisampleFramebuffer();
  } else {
    SLOG(SLOG_ERROR,
         "No MSAA rendering functions available, but did create an MSAA "
         "buffer.");
  }
}

bool GenMSAABuffers(const GLResourceManager& resource_manager, GLuint* fbo_msaa,
                    GLuint* rbo_msaa, glm::ivec2 backing_size,
                    GLenum internal_format) {
  using ion::gfx::GraphicsManager;
  const ion::gfx::GraphicsManagerPtr& gl = resource_manager.gl;
  GLEXPECT_NO_ERROR(gl);
  GLint requested_samples = 4;
  GLint max_samples_allowed;
  gl->GetIntegerv(GL_MAX_SAMPLES, &max_samples_allowed);
  uint32_t samples_to_use = (requested_samples > max_samples_allowed)
                                ? max_samples_allowed
                                : requested_samples;

  if (internal_format == 0) {
    if (gl->IsFeatureAvailable(GraphicsManager::kRgba8)) {
      internal_format = GL_RGBA8;
    } else {
      internal_format = GL_RGBA4;
    }
  }

  /* Create the MSAA framebuffer (offscreen) */
  gl->GenFramebuffers(1, fbo_msaa);
  gl->BindFramebuffer(GL_FRAMEBUFFER, *fbo_msaa);

  /* Create the offscreen MSAA color buffer.
   * After rendering, the contents of this will be blitted into
   * resolveColorbuffer */
  gl->GenRenderbuffers(1, rbo_msaa);
  gl->BindRenderbuffer(GL_RENDERBUFFER, *rbo_msaa);
  gl->RenderbufferStorageMultisample(GL_RENDERBUFFER, samples_to_use,
                                     internal_format, backing_size.x,
                                     backing_size.y);
  gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, *rbo_msaa);

  // Gracefully recover from all gl errors when generating the MSAA buffers.
  auto err = gl->GetError();
  if (err != GL_NO_ERROR) {
    SLOG(SLOG_ERROR, "Error trying to generate MSAA buffers ($0)",
         gl->ErrorString(err));
    return false;
  }

  return IsFramebufferComplete(resource_manager);
}

}  // namespace msaa
}  // namespace ink
