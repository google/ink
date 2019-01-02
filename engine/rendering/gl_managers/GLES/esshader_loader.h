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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_GLES_ESSHADER_LOADER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_GLES_ESSHADER_LOADER_H_

#include <string>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/gl.h"

namespace ink {
namespace gles {
GLuint BuildProgram(const ion::gfx::GraphicsManagerPtr& gl,
                    const std::string& vert_shader_path,
                    const std::string& vert_shader_source,
                    const std::string& frag_shader_path,
                    const std::string& frag_shader_source);
}  // namespace gles
}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_GLES_ESSHADER_LOADER_H_
