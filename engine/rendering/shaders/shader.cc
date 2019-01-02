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

#include "ink/engine/rendering/shaders/shader.h"

#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/rendering/gl_managers/GLES/esshader_loader.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/rendering/gl_managers/shader_resources.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace shaders {

std::string GetShaderByPath(const std::string& path) {
  SLOG(SLOG_GL_STATE, "Loading resource $0", path);
  for (const FileToc* p = ink::shader_resources_create(); p->name != nullptr;
       ++p) {
    if (path == p->name) {
      return p->data;
    }
  }
  SLOG(SLOG_ERROR, "Lookup of $0 failed.", path);
  return "";
}

using ink::kBadGLHandle;

Shader::Shader(ion::gfx::GraphicsManagerPtr gl,
               std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
               std::string vertex_shader_path, std::string fragment_shader_path,
               InterleavedAttributeSet attrs)
    : gl_(std::move(gl)),
      mesh_vbo_provider_(std::move(mesh_vbo_provider)),
      program_(kBadGLHandle),
      attribute_set_(attrs),
      vertex_shader_path_(vertex_shader_path),
      fragment_shader_path_(fragment_shader_path) {}

void Shader::Use() const {
  EXPECT(program_ != kBadGLHandle);
  gl_->UseProgram(program_);
  attribute_set_.Use();
}

void Shader::Unuse() const {
  attribute_set_.Unuse();
  gl_->UseProgram(0);
  GLASSERT_NO_ERROR(gl_);
}

void Shader::Load() {
  const std::string vert_source = GetShaderByPath(vertex_shader_path_);
  const std::string frag_source = GetShaderByPath(fragment_shader_path_);
  program_ = ink::gles::BuildProgram(gl_, vertex_shader_path_, vert_source,
                                     fragment_shader_path_, frag_source);
  ASSERT(program_ != ink::kBadGLHandle);
  attribute_set_.LoadAttributes(program_);
}

GLuint Shader::LoadUniform(const GLchar* name) {
  GLEXPECT(gl_, program_ != kBadGLHandle);
  auto h = static_cast<GLuint>(gl_->GetUniformLocation(program_, name));
  GLEXPECT(gl_, h != kBadGLHandle);
  uniforms_[std::string(name)] = h;
  return h;
}

GLuint Shader::GetUniform(const GLchar* name) const {
  auto res = uniforms_.find(std::string(name));
  EXPECT(res != uniforms_.end());
  EXPECT(res->second != kBadGLHandle);
  return res->second;
}

bool Shader::HasUniform(const GLchar* name) const {
  auto res = uniforms_.find(std::string(name));
  return (res != uniforms_.end());
}

const InterleavedAttributeSet& Shader::GetAttrs() const {
  return attribute_set_;
}

}  // namespace shaders
}  // namespace ink
