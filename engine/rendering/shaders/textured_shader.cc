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

#include "ink/engine/rendering/shaders/textured_shader.h"

#include <cstddef>
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"
#include "ink/engine/rendering/shaders/shader_util.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace shaders {

using ink::DrawMesh;

static InterleavedAttributeSet TexturedAttrs(
    const ion::gfx::GraphicsManagerPtr& gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("textureCoord", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  return attrs;
}

TexturedShader::TexturedShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "TextureShaders/Textured",
             TexturedAttrs(gl)) {}

TexturedShader::TexturedShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
    const std::string& frag_shader_name)
    : Shader(gl, mesh_vbo_provider, "TextureShaders/Textured.vert",
             frag_shader_name, TexturedAttrs(gl)) {}

void TexturedShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform("view");
  LoadUniform("sampler");
}

void TexturedShader::Draw(const Mesh& mesh) const {
  EXPECT(mesh_vbo_provider_->HasVBO(mesh));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

void TexturedShader::Use(const Camera& cam) const {
  Shader::Use();

  auto wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform("view"), 1, 0, &wv[0][0]);

  gl_->Uniform1i(GetUniform("sampler"), 0);
}

///////////////////////////////////////////////////////////////////

static InterleavedAttributeSet TexturedColorTintAttrs(
    const ion::gfx::GraphicsManagerPtr& gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("textureCoord", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  attrs.AddAttribute("colorFilter", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  return attrs;
}

TexturedColorTintShader::TexturedColorTintShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "TextureShaders/TexturedColorTint",
             TexturedColorTintAttrs(gl)) {}

void TexturedColorTintShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform("view");
  LoadUniform("sampler");
}

void TexturedColorTintShader::Draw(const Mesh& mesh) const {
  EXPECT(mesh_vbo_provider_->HasVBO(mesh));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

void TexturedColorTintShader::Use(const Camera& cam) const {
  Shader::Use();

  auto wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform("view"), 1, 0, &wv[0][0]);

  gl_->Uniform1i(GetUniform("sampler"), 0);
}

///////////////////////////////////////////////////////////////////

static InterleavedAttributeSet TexturedMaskAttrs(
    const ion::gfx::GraphicsManagerPtr& gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("textureCoord", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  attrs.AddAttribute("colorFilter", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  return attrs;
}

TexturedMaskToBgShader::TexturedMaskToBgShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "TextureShaders/TexturedMask",
             TexturedMaskAttrs(gl)) {}

void TexturedMaskToBgShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform("view");
  LoadUniform("sampler");
}

void TexturedMaskToBgShader::Draw(const Mesh& mesh) const {
  EXPECT(mesh_vbo_provider_->HasVBO(mesh));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

void TexturedMaskToBgShader::Use(const Camera& cam) const {
  Shader::Use();

  auto wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform("view"), 1, 0, &wv[0][0]);

  gl_->Uniform1i(GetUniform("sampler"), 0);
}

///////////////////////////////////////////////////////////////////

static InterleavedAttributeSet TextureBlurAttrs(
    const ion::gfx::GraphicsManagerPtr& gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("texCoordFrom", sizeof(Vertex().texture_coords_from),
                     offsetof(Vertex, texture_coords_from));
  attrs.AddAttribute("texCoordTo", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  return attrs;
}

TexturedMotionBlurShader::TexturedMotionBlurShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "TextureShaders/TexturedBlur",
             TextureBlurAttrs(gl)) {}

void TexturedMotionBlurShader::Load() {
  Shader::Load();

  LoadUniform("view");
  LoadUniform("sampler");
}

void TexturedMotionBlurShader::Use(const Camera& cam) const {
  Shader::Use();
  auto wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform("view"), 1, 0, &wv[0][0]);
  gl_->Uniform1i(GetUniform("sampler"), 0);
}

void TexturedMotionBlurShader::Draw(const Mesh& mesh) const {
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

}  // namespace shaders
}  // namespace ink
