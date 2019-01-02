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

#ifndef INK_ENGINE_RENDERING_SHADERS_TEXTURED_SHADER_H_
#define INK_ENGINE_RENDERING_SHADERS_TEXTURED_SHADER_H_

#include <memory>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/shaders/shader.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace shaders {

class TexturedShader : public Shader {
 public:
  TexturedShader(ion::gfx::GraphicsManagerPtr gl,
                 std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  TexturedShader(ion::gfx::GraphicsManagerPtr gl,
                 std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
                 const std::string& frag_shader_name);

  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;
};

class TexturedColorTintShader : public Shader {
 public:
  explicit TexturedColorTintShader(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;
};

class TexturedMaskToBgShader : public Shader {
 public:
  explicit TexturedMaskToBgShader(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;
};

class TexturedMotionBlurShader : public Shader {
 public:
  explicit TexturedMotionBlurShader(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;
};

}  // namespace shaders
}  // namespace ink
#endif  // INK_ENGINE_RENDERING_SHADERS_TEXTURED_SHADER_H_
