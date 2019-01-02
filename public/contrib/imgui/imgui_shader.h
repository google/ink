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

#ifndef INK_PUBLIC_CONTRIB_IMGUI_IMGUI_SHADER_H_
#define INK_PUBLIC_CONTRIB_IMGUI_IMGUI_SHADER_H_

#include "third_party/dear_imgui/imgui.h"
#include "ink/engine/geometry/mesh/gl/vbo.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace imgui {

class ImGuiShader : public shaders::Shader {
 public:
  explicit ImGuiShader(const std::shared_ptr<GLResourceManager>& gl_resources);

  void Load() override;
  void Draw(const Camera& cam, ImDrawList* cmd_list) const;
  void Use() const override;
  void Use(const Camera& cam) const;

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;
  mutable VBO idx_vbo_;
  mutable VBO vert_vbo_;
};

}  // namespace imgui
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_IMGUI_IMGUI_SHADER_H_
