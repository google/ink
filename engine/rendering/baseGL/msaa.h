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

#ifndef INK_ENGINE_RENDERING_BASEGL_MSAA_H_
#define INK_ENGINE_RENDERING_BASEGL_MSAA_H_

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace msaa {

// Blits the MSAA framebuffer into the non-MSAA framebuffer in the area
// delineated by box.  The underlying GL functions expect integer values, so the
// float values in the box are truncated to the nearest pixel value.
void ResolveMultisampleFramebuffer(const GLResourceManager& resource_manager,
                                   GLuint fbo, GLuint fbo_msaa,
                                   const Rect& box);

// Returns true if the MSAA buffers were successfully generated.
bool GenMSAABuffers(const GLResourceManager& resource_manager, GLuint* fbo_msaa,
                    GLuint* rbo_msaa, glm::ivec2 backing_size,
                    GLenum internal_format = 0);

bool IsFramebufferComplete(const GLResourceManager& resource_manager);

}  // namespace msaa
}  // namespace ink

#endif  // INK_ENGINE_RENDERING_BASEGL_MSAA_H_
