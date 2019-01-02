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

#ifndef INK_ENGINE_RENDERING_SHADERS_SHADER_H_
#define INK_ENGINE_RENDERING_SHADERS_SHADER_H_

#include <map>
#include <memory>
#include <string>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"

namespace ink {
namespace shaders {

// Visible for testing.
std::string GetShaderByPath(const std::string& path);

// Interface to a gl shader program.  Stores the gl bindings for the program,
// its attributes, and uniforms.  Wraps calls to the gl state machine for
// initializing a program (load) binding the program to the gl state machine
// (use), and running the program (subclasses typically implement a method
// called draw).
//
// Invariants: Load must be called exactly once, before use or draw can be
// called.  Use must be called every time you want to draw with:
//   1. a different shader, or
//   2. the same shader but with different uniforms.
class Shader {
 public:
  // Convenience constructor for when vertex and fragment shader have the same
  // base name.  E.g., shaders "Foo.vert" and "Foo.frag", call Shader("Foo")
  Shader(ion::gfx::GraphicsManagerPtr gl,
         std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
         const std::string& shader_path, InterleavedAttributeSet attrs)
      : Shader(std::move(gl), std::move(mesh_vbo_provider),
               shader_path + ".vert", shader_path + ".frag", std::move(attrs)) {
  }
  Shader(ion::gfx::GraphicsManagerPtr gl,
         std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
         std::string vertex_shader_path, std::string fragment_shader_path,
         InterleavedAttributeSet attrs);
  virtual ~Shader() {}

  // Set gl handles for program, attributes, and uniforms (via loadUniform).
  virtual void Load();

  // Enable attributes and set uniforms.
  virtual void Use() const;
  virtual void Unuse() const;

  GLuint GetUniform(const GLchar* name) const;
  bool HasUniform(const GLchar* name) const;

  const InterleavedAttributeSet& GetAttrs() const;

 protected:
  // Shaders are non-copyable
  Shader(const Shader&) = delete;
  void operator=(const Shader& other) = delete;

  GLuint LoadUniform(const GLchar* name);

 protected:
  ion::gfx::GraphicsManagerPtr gl_;
  std::shared_ptr<MeshVBOProvider> mesh_vbo_provider_;

 private:
  GLuint program_;  // Actual GL handle. Defaults to "kBadGLHandle"
  InterleavedAttributeSet attribute_set_;
  std::string vertex_shader_path_;
  std::string fragment_shader_path_;

  // Uniform name in shader program to gl handle.
  std::map<std::string, GLuint> uniforms_;
};

}  // namespace shaders
}  // namespace ink
#endif  // INK_ENGINE_RENDERING_SHADERS_SHADER_H_
