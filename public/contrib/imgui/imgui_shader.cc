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

#include "ink/public/contrib/imgui/imgui_shader.h"

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"

namespace ink {
namespace imgui {

static InterleavedAttributeSet ImGuiShaderAttrs(
    const ion::gfx::GraphicsManagerPtr& gl) {
  InterleavedAttributeSet attrs(gl, sizeof(ImDrawVert));
  attrs.AddAttribute("position", sizeof(ImDrawVert().pos),
                     offsetof(ImDrawVert, pos));
  attrs.AddAttribute("textureCoords", sizeof(ImDrawVert().uv),
                     offsetof(ImDrawVert, uv));
  attrs.AddAttribute(ShaderAttribute("sourcecolor", sizeof(ImDrawVert().col),
                                     offsetof(ImDrawVert, col),
                                     GL_UNSIGNED_BYTE, 1, true));
  return attrs;
}

ImGuiShader::ImGuiShader(const std::shared_ptr<GLResourceManager>& gl_resources)
    : Shader(gl_resources->gl, gl_resources->mesh_vbo_provider,
             "MeshShaders/VertTexturedColored.vert",
             "MeshShaders/VertA8TexturedColored.frag",
             ImGuiShaderAttrs(gl_resources->gl)),
      gl_resources_(gl_resources),
      idx_vbo_(gl_resources->gl, 10, GL_STREAM_DRAW, GL_ELEMENT_ARRAY_BUFFER),
      vert_vbo_(gl_resources->gl, 10, GL_STREAM_DRAW, GL_ARRAY_BUFFER) {}

void ImGuiShader::Load() {
  Shader::Load();
  LoadUniform("proj");
  LoadUniform("sampler");
  GLASSERT_NO_ERROR(gl_resources_->gl);
}

void ImGuiShader::Draw(const Camera& cam, ImDrawList* cmd_list) const {
  if (cmd_list->CmdBuffer.Size == 0 || cmd_list->VtxBuffer.Size == 0 ||
      cmd_list->VtxBuffer.Size == 0) {
    // Don't attempt to allocate 0 size vbos, this is spec'd as transfer all
    // memory to the gpu on webgl.
    return;
  }
  auto& gl = gl_resources_->gl;
  GLASSERT_NO_ERROR(gl);

  int fb_height = static_cast<int>(ImGui::GetIO().DisplaySize.y);
  vert_vbo_.FitExactly(cmd_list->VtxBuffer.Data,
                       cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
  idx_vbo_.FitExactly(cmd_list->IdxBuffer.Data,
                      cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
  vert_vbo_.Bind();
  idx_vbo_.Bind();
  GetAttrs().BindVBO();

  const ImDrawIdx* idx_buffer_offset = nullptr;
#ifndef __native_client__
  // b/79432595 Scissor doesn't work correctly on nacl for unknown reason.
  gl->Enable(GL_SCISSOR_TEST);
#else
  gl->Disable(GL_SCISSOR_TEST);
#endif  // INK_PLATFORM_NACL

  for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
    const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
    if (pcmd->UserCallback) {
      pcmd->UserCallback(cmd_list, pcmd);
    } else {
      Texture* tex =
          reinterpret_cast<Texture*>(static_cast<void*>(pcmd->TextureId));
      tex->Bind(GL_TEXTURE0);
      gl->Scissor(static_cast<int>(pcmd->ClipRect.x),
                  static_cast<int>(fb_height - pcmd->ClipRect.w),
                  static_cast<int>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                  static_cast<int>(pcmd->ClipRect.w - pcmd->ClipRect.y));

      gl->DrawElements(
          GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount),
          sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
          idx_buffer_offset);
    }
    idx_buffer_offset += pcmd->ElemCount;
  }

  gl->Disable(GL_SCISSOR_TEST);

  GLASSERT_NO_ERROR(gl_resources_->gl);
}
void ImGuiShader::Use() const { RUNTIME_ERROR("You must pass a camera."); }

void ImGuiShader::Use(const Camera& cam) const {
  GLASSERT_NO_ERROR(gl_resources_->gl);
  ImGuiIO& io = ImGui::GetIO();

  Shader::Use();
  glm::mat4 proj =
      glm::ortho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, 1.0f);
  gl_resources_->gl->UniformMatrix4fv(GetUniform("proj"), 1, 0,
                                      glm::value_ptr(proj));

  GLASSERT_NO_ERROR(gl_resources_->gl);
}
}  // namespace imgui
}  // namespace ink
