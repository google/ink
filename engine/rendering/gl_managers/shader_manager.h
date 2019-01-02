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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_SHADER_MANAGER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_SHADER_MANAGER_H_

#include <map>
#include <string>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/rendering/shaders/packed_mesh_shaders.h"
#include "ink/engine/rendering/shaders/textured_shader.h"

namespace ink {

namespace internal {
template <typename T>
class LazilyLoadedShader {
 public:
  explicit LazilyLoadedShader(std::unique_ptr<T> shader)
      : shader_(std::move(shader)) {}

  void Load() {
    if (!loaded_) {
      shader_->Load();
      loaded_ = true;
    }
  }

  T& operator*() {
    Load();
    return *shader_;
  }

 private:
  std::unique_ptr<T> shader_;
  bool loaded_ = false;
};
}  // namespace internal

// ShaderManager loads and compiles shaders, given the shader source paths.
// It caches compiled programs.
// It disposes of all compiled programs when destroyed.
class ShaderManager {
 public:
  ShaderManager(const ion::gfx::GraphicsManagerPtr& gl,
                const std::shared_ptr<BackgroundState>& background_state,
                const std::shared_ptr<MeshVBOProvider>& mesh_vbo_provider,
                const std::shared_ptr<TextureManager>& texture_manager);

  void LoadAllShaders();

  shaders::PackedVertShader& PackedShader() { return *packed_shader_; }
  shaders::VertColoredShader& VertColoredShader() {
    return *vert_colored_shader_;
  }
  shaders::AnimatedShader& AnimatedShader() { return *animated_shader_; }
  shaders::SolidEraserShader& SolidEraserShader() { return *eraser_shader_; }
  shaders::TexturedEraserShader& TexturedEraserShader() {
    return *textured_eraser_shader_;
  }
  shaders::VertTexturedShader& VertTexturedShader() {
    return *textured_shader_;
  }
  shaders::ParticleShader& ParticleShader() { return *particle_shader_; }
  shaders::TexturedColorTintShader& TexturedColorTintShader() {
    return *textured_tint_shader_;
  }
  shaders::TexturedMaskToBgShader& TexturedMaskShader() {
    return *textured_mask_shader_;
  }
  shaders::TexturedMotionBlurShader& TexturedMotionBlurShader() {
    return *blur_shader_;
  }

  shaders::TexturedShader& GetShaderForEffect(blit_attrs::FilterEffect effect);

 private:
  internal::LazilyLoadedShader<shaders::PackedVertShader> packed_shader_;
  internal::LazilyLoadedShader<shaders::VertColoredShader> vert_colored_shader_;
  internal::LazilyLoadedShader<shaders::AnimatedShader> animated_shader_;
  internal::LazilyLoadedShader<shaders::SolidEraserShader> eraser_shader_;
  internal::LazilyLoadedShader<shaders::TexturedEraserShader>
      textured_eraser_shader_;
  internal::LazilyLoadedShader<shaders::VertTexturedShader> textured_shader_;
  internal::LazilyLoadedShader<shaders::ParticleShader> particle_shader_;
  internal::LazilyLoadedShader<shaders::TexturedColorTintShader>
      textured_tint_shader_;
  internal::LazilyLoadedShader<shaders::TexturedMaskToBgShader>
      textured_mask_shader_;
  internal::LazilyLoadedShader<shaders::TexturedMotionBlurShader> blur_shader_;

  std::unordered_map<blit_attrs::FilterEffect,
                     internal::LazilyLoadedShader<shaders::TexturedShader>>
      textured_shaders_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_SHADER_MANAGER_H_
