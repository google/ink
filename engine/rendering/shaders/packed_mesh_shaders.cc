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

#include "ink/engine/rendering/shaders/packed_mesh_shaders.h"

#include <cstddef>
#include <memory>

#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/geometry/mesh/gl/indexed_vbo.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/rendering/shaders/interleaved_attribute_set.h"
#include "ink/engine/rendering/shaders/shader_util.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {
namespace shaders {

using glm::mat4;
using glm::value_ptr;
using glm::vec2;
using glm::vec4;

using ink::DrawMesh;

constexpr char kSourceColorUniformName[] = "sourcecolor";
constexpr char kViewUniformName[] = "view";
constexpr char kObjectUniformName[] = "object";
constexpr char kObjToUVUniformName[] = "objToUV";
constexpr char kPackedUvToUvUniformName[] = "packed_uv_to_uv";

/////////////////////////////

static InterleavedAttributeSet CreatePkShaderAttribute(
    ion::gfx::GraphicsManagerPtr gl, size_t size_in_bytes) {
  return InterleavedAttributeSet::CreatePacked(gl, "pkdata", size_in_bytes);
}

PackedVertShader::PackedVertShader(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
    std::shared_ptr<BackgroundState> background_state,
    std::shared_ptr<TextureManager> texture_manager)
    : gl_(gl),
      background_state_(background_state),
      mesh_vbo_provider_(mesh_vbo_provider),
      texture_manager_(texture_manager),
      shader_x12y12_(gl, mesh_vbo_provider),
      shader_x12y12textured_(gl, mesh_vbo_provider),
      shader_x32y32_(gl, mesh_vbo_provider),
      shader_x11a7r6y11g7b6_(gl, mesh_vbo_provider),
      shader_x11a7r6y11g7b6u12v12_(gl, mesh_vbo_provider) {}

bool PackedVertShader::ShouldDrawAsEraserTexture(
    const OptimizedMesh& mesh) const {
  return mesh.type == EraseShader &&
         background_state_->IsImageAndReady(texture_manager_.get());
}

const Shader* PackedVertShader::GetShader(const OptimizedMesh& mesh) const {
  VertFormat format = mesh.verts.GetFormat();

  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
      return &shader_x11a7r6y11g7b6_;
    case VertFormat::x32y32:
      return &shader_x32y32_;
    case VertFormat::x12y12:
      if (ShouldDrawAsEraserTexture(mesh)) {
        return &shader_x12y12textured_;
      } else {
        return &shader_x12y12_;
      }
    case VertFormat::x11a7r6y11g7b6u12v12:
      return &shader_x11a7r6y11g7b6u12v12_;
  }
}

void PackedVertShader::Use(const Camera& cam, const OptimizedMesh& mesh) const {
  const Shader* shdr = GetShader(mesh);
  shdr->Use();
  mat4 wv = cam.WorldToDevice();
  gl_->UniformMatrix4fv(shdr->GetUniform(kViewUniformName), 1, 0,
                        value_ptr(wv));
  if (ShouldDrawAsEraserTexture(mesh)) {
    ImageBackgroundState* image_background;
    EXPECT(background_state_->GetImage(&image_background));
    mat4 world_to_uv = image_background->WorldToUV();
    mat4 obj_to_uv = world_to_uv * mesh.object_matrix;
    gl_->UniformMatrix4fv(shdr->GetUniform(kObjToUVUniformName), 1, 0,
                          value_ptr(obj_to_uv));
    EXPECT(texture_manager_->Bind(image_background->TextureHandle()));
  }
}

void PackedVertShader::Unuse(const OptimizedMesh& mesh) const {
  GetShader(mesh)->Unuse();
}

void PackedVertShader::Load() {
  shader_x12y12_.Load();
  shader_x12y12textured_.Load();
  shader_x32y32_.Load();
  shader_x11a7r6y11g7b6_.Load();
  shader_x11a7r6y11g7b6u12v12_.Load();
}

void PackedVertShader::Draw(const OptimizedMesh& mesh) const {
  const Shader* shdr = GetShader(mesh);
  if (shdr->HasUniform(kSourceColorUniformName)) {
    vec4 draw_color = util::Clamp01(
        glm::fma(mesh.color, mesh.mul_color_modifier, mesh.add_color_modifier));
    if (mesh.type == EraseShader) {
      ASSERT(!ShouldDrawAsEraserTexture(mesh));
      draw_color = background_state_->GetColor();
    }
    gl_->Uniform4fv(shdr->GetUniform(kSourceColorUniformName), 1,
                    value_ptr(draw_color));
  }
  if (shdr->HasUniform(kObjectUniformName)) {
    gl_->UniformMatrix4fv(shdr->GetUniform(kObjectUniformName), 1, 0,
                          value_ptr(mesh.object_matrix));
  }
  if (mesh.texture) {
    ASSERT(mesh.type == ShaderType::TexturedVertShader);
    ASSERT(mesh.verts.GetFormat() == VertFormat::x11a7r6y11g7b6u12v12);
    if (!texture_manager_->Bind(*mesh.texture)) {
      return;
    }
    if (shdr->HasUniform(kPackedUvToUvUniformName)) {
      gl_->UniformMatrix4fv(shdr->GetUniform(kPackedUvToUvUniformName), 1, 0,
                            value_ptr(mesh.verts.PackedUvToUvTransform()));
    } else {
      ASSERT(false);
      return;
    }
  }
  DrawMesh(gl_, mesh_vbo_provider_, mesh, shdr->GetAttrs());
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

PackedShaderX12Y12::PackedShaderX12Y12(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Packed32X12.vert",
             "MeshShaders/Passthrough.frag",
             CreatePkShaderAttribute(gl, sizeof(float))) {}

void PackedShaderX12Y12::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kSourceColorUniformName);
  LoadUniform(kObjectUniformName);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

PackedShaderX12Y12Textured::PackedShaderX12Y12Textured(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Packed32X12Textured.vert",
             "MeshShaders/SolidTexture.frag",
             CreatePkShaderAttribute(gl, sizeof(float))) {}

void PackedShaderX12Y12Textured::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kObjToUVUniformName);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

PackedShaderX32Y32::PackedShaderX32Y32(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Packed64X32.vert",
             "MeshShaders/Passthrough.frag",
             CreatePkShaderAttribute(gl, sizeof(vec2))) {}

void PackedShaderX32Y32::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kSourceColorUniformName);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

PackedShaderX11A7R6Y11G7B6U12V12::PackedShaderX11A7R6Y11G7B6U12V12(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider,
             "MeshShaders/packed_x11a7r6y11g7b6u12v12.vert",
             "MeshShaders/TintedTexture.frag",
             CreatePkShaderAttribute(gl, sizeof(float) * 3)) {}

void PackedShaderX11A7R6Y11G7B6U12V12::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
  LoadUniform(kPackedUvToUvUniformName);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

PackedShaderX11A7R6Y11G7B6::PackedShaderX11A7R6Y11G7B6(
    ion::gfx::GraphicsManagerPtr gl,
    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider)
    : Shader(gl, mesh_vbo_provider, "MeshShaders/Packed64X11.vert",
             "MeshShaders/Passthrough.frag",
             CreatePkShaderAttribute(gl, sizeof(float) * 2)) {}

void PackedShaderX11A7R6Y11G7B6::Load() {
  Shader::Load();

  // uniforms
  LoadUniform(kViewUniformName);
  LoadUniform(kObjectUniformName);
}

///////////////////////////////////////////////////////////////
}  // namespace shaders
}  // namespace ink
