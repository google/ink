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

#include "ink/engine/rendering/shaders/mesh_shader.h"

#include <cstddef>
#include <memory>

#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"
#include "ink/engine/rendering/shaders/shader_util.h"
#include "ink/engine/util/dbg/glerrors.h"

namespace ink {
namespace shaders {

using glm::mat4;
using glm::value_ptr;
using glm::vec4;

using ink::DrawMesh;

const char* const kSourceColorUniformName = "sourcecolor";
const char* const kViewUniformName = "view";
const char* const kObjectUniformName = "object";
const char* const kObjToUVUniformName = "objToUV";
const char* const kTimeUniformName = "time";

////////////////////////// VertColoredShader //////////////////////////////

static InterleavedAttributeSet CreateVertColoredAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("sourcecolor", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  return attrs;
}

VertColoredShader::VertColoredShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/VertColored.vert",
             "MeshShaders/Passthrough.frag",
             CreateVertColoredAttributeSet(gl)) {}

void VertColoredShader::Use(const Camera& cam) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));
}

void VertColoredShader::UseIdentity() const {
  Shader::Use();
  mat4 wv{1};
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));
}

void VertColoredShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
}

void VertColoredShader::Draw(const Mesh& mesh) const {
  gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                        value_ptr(mesh.object_matrix));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

//////////////// Solid Eraser Shader ///////////////////////////

static InterleavedAttributeSet CreateSolidEraserAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("pkdata", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  return attrs;
}

SolidEraserShader::SolidEraserShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
    std::shared_ptr<BackgroundState> background_state)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Packed64X32.vert",
             "MeshShaders/Passthrough.frag", CreateSolidEraserAttributeSet(gl)),
      background_state_(background_state) {}

void SolidEraserShader::Use(const Camera& cam) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));
  vec4 bg_color = background_state_->GetColor();
  gl_->Uniform4fv(GetUniform(kSourceColorUniformName), 1, value_ptr(bg_color));
}

void SolidEraserShader::Load() {
  Shader::Load();
  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kSourceColorUniformName);
}

void SolidEraserShader::Draw(const Mesh& mesh) const {
  ASSERT(!background_state_->IsImage());
  ASSERT(mesh.shader_metadata.IsEraser());

  gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                        value_ptr(mesh.object_matrix));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

//////////////// TexturedEraserShader ///////////////////////////

static InterleavedAttributeSet CreateTexturedEraserAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  return attrs;
}

TexturedEraserShader::TexturedEraserShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
    std::shared_ptr<BackgroundState> background_state,
    std::shared_ptr<TextureManager> texture_manager)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/VertTextureFromPosition.vert",
             "MeshShaders/SolidTexture.frag",
             CreateTexturedEraserAttributeSet(gl)),
      background_state_(background_state),
      texture_manager_(texture_manager) {}

void TexturedEraserShader::Use(const Camera& cam) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));

  GLASSERT_NO_ERROR(gl_);
}

void TexturedEraserShader::Load() {
  Shader::Load();
  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kObjToUVUniformName);
}

void TexturedEraserShader::Draw(const Mesh& mesh) const {
  ImageBackgroundState* background_state = nullptr;
  EXPECT(background_state_->GetImage(&background_state));
  ASSERT(mesh.shader_metadata.IsEraser());

  // Texture may still be loading.
  if (texture_manager_->Bind(background_state->TextureHandle())) {
    mat4 world_to_uv = background_state->WorldToUV();
    mat4 obj_to_uv = world_to_uv * mesh.object_matrix;
    gl_->UniformMatrix4fv(GetUniform(kObjToUVUniformName), 1, 0,
                          value_ptr(obj_to_uv));

    gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                          value_ptr(mesh.object_matrix));
    DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
  }
}

//////////////////////////// VertTexturedShader ///////////////////////////

static InterleavedAttributeSet CreateVertTexturedAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("sourcecolor", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  attrs.AddAttribute("textureCoords", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  return attrs;
}

VertTexturedShader::VertTexturedShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
    std::shared_ptr<TextureManager> texture_manager)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/VertTextured.vert",
             "MeshShaders/TintedTexture.frag",
             CreateVertTexturedAttributeSet(gl)),
      texture_manager_(texture_manager) {}

void VertTexturedShader::Use(const Camera& cam) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));

  GLASSERT_NO_ERROR(gl_);
}

void VertTexturedShader::Load() {
  Shader::Load();
  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
}

void VertTexturedShader::Draw(const Mesh& mesh) const {
  ASSERT(mesh.texture != nullptr);
  if (mesh.texture != nullptr && texture_manager_->Bind(*mesh.texture)) {
    gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                          value_ptr(mesh.object_matrix));
    DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
  }
}

////////////////////////// AnimatedShader //////////////////////////////

static InterleavedAttributeSet CreateAnimatedAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("positionFrom", sizeof(Vertex().position_from),
                     offsetof(Vertex, position_from));
  attrs.AddAttribute("positionTo", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("positionTimings", sizeof(Vertex().position_timings),
                     offsetof(Vertex, position_timings));

  attrs.AddAttribute("sourceColorFrom", sizeof(Vertex().color_from),
                     offsetof(Vertex, color_from));
  attrs.AddAttribute("sourceColorTo", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  attrs.AddAttribute("sourceColorTimings", sizeof(Vertex().color_timings),
                     offsetof(Vertex, color_timings));
  return attrs;
}

AnimatedShader::AnimatedShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Animated.vert",
             "MeshShaders/Passthrough.frag", CreateAnimatedAttributeSet(gl)) {}

void AnimatedShader::Use(const Camera& cam, DurationS time_since_init) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));
  gl_->Uniform1f(GetUniform(kTimeUniformName),
                 static_cast<float>(time_since_init));
}

void AnimatedShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kTimeUniformName);
}

void AnimatedShader::Draw(const Mesh& mesh) const {
  gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                        value_ptr(mesh.object_matrix));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

////////////////////////// ParticleShader //////////////////////////////

static InterleavedAttributeSet CreateParticleAttributeSet(
    ion::gfx::GraphicsManagerPtr gl) {
  InterleavedAttributeSet attrs(gl, sizeof(Vertex));
  attrs.AddAttribute("position", sizeof(Vertex().position),
                     offsetof(Vertex, position));
  attrs.AddAttribute("velocity", sizeof(Vertex().position_from),
                     offsetof(Vertex, position_from));
  attrs.AddAttribute("positionTimings", sizeof(Vertex().position_timings),
                     offsetof(Vertex, position_timings));

  attrs.AddAttribute("sourceColorFrom", sizeof(Vertex().color_from),
                     offsetof(Vertex, color_from));
  attrs.AddAttribute("sourceColorTo", sizeof(Vertex().color),
                     offsetof(Vertex, color));
  attrs.AddAttribute("sourceColorTimings", sizeof(Vertex().color_timings),
                     offsetof(Vertex, color_timings));

  attrs.AddAttribute("textureCoords", sizeof(Vertex().texture_coords),
                     offsetof(Vertex, texture_coords));
  return attrs;
}

ParticleShader::ParticleShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/ParticleShader.vert",
             "MeshShaders/ParticleShader.frag",
             CreateParticleAttributeSet(gl)) {}

void ParticleShader::Use(const Camera& cam, DurationS time_since_init) const {
  Shader::Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(GetUniform(kViewUniformName), 1, 0, value_ptr(wv));
  gl_->Uniform1f(GetUniform(kTimeUniformName),
                 static_cast<float>(time_since_init));
}

void ParticleShader::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kTimeUniformName);
}

void ParticleShader::Draw(const Mesh& mesh) const {
  gl_->UniformMatrix4fv(GetUniform(kObjectUniformName), 1, 0,
                        value_ptr(mesh.object_matrix));
  DrawMesh(gl_, mesh_vbo_provider_, mesh, GetAttrs());
}

}  // namespace shaders
}  // namespace ink
