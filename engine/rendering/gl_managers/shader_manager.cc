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

#include "ink/engine/rendering/gl_managers/shader_manager.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>

#include "third_party/absl/strings/substitute.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/gl_managers/GLES/esshader_loader.h"
#include "ink/engine/rendering/gl_managers/bad_gl_handle.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

ShaderManager::ShaderManager(
    const ion::gfx::GraphicsManagerPtr& gl,
    const std::shared_ptr<BackgroundState>& background_state,
    const std::shared_ptr<MeshVBOProvider>& mesh_vbo_provider,
    const std::shared_ptr<TextureManager>& texture_manager)
    : packed_shader_(absl::make_unique<shaders::PackedVertShader>(
          gl, mesh_vbo_provider, background_state, texture_manager)),
      vert_colored_shader_(
          absl::make_unique<shaders::VertColoredShader>(gl, mesh_vbo_provider)),
      animated_shader_(
          absl::make_unique<shaders::AnimatedShader>(gl, mesh_vbo_provider)),
      eraser_shader_(absl::make_unique<shaders::SolidEraserShader>(
          gl, mesh_vbo_provider, background_state)),
      textured_eraser_shader_(absl::make_unique<shaders::TexturedEraserShader>(
          gl, mesh_vbo_provider, background_state, texture_manager)),
      textured_shader_(absl::make_unique<shaders::VertTexturedShader>(
          gl, mesh_vbo_provider, texture_manager)),
      particle_shader_(
          absl::make_unique<shaders::ParticleShader>(gl, mesh_vbo_provider)),
      textured_tint_shader_(absl::make_unique<shaders::TexturedColorTintShader>(
          gl, mesh_vbo_provider)),
      textured_mask_shader_(absl::make_unique<shaders::TexturedMaskToBgShader>(
          gl, mesh_vbo_provider)),
      blur_shader_(absl::make_unique<shaders::TexturedMotionBlurShader>(
          gl, mesh_vbo_provider)) {
  uint32_t num_filters =
      static_cast<uint32_t>(blit_attrs::FilterEffect::MaxValue);
  for (uint32_t i = 0; i < num_filters; ++i) {
    blit_attrs::FilterEffect effect = static_cast<blit_attrs::FilterEffect>(i);
    textured_shaders_.emplace(std::make_pair(
        effect, internal::LazilyLoadedShader<shaders::TexturedShader>(
                    absl::make_unique<shaders::TexturedShader>(
                        gl, mesh_vbo_provider,
                        blit_attrs::FragShaderNameForEffect(effect)))));
  }
}

shaders::TexturedShader& ShaderManager::GetShaderForEffect(
    blit_attrs::FilterEffect effect) {
  ASSERT(textured_shaders_.find(effect) != textured_shaders_.end());
  return *(textured_shaders_.at(effect));
}

void ShaderManager::LoadAllShaders() {
  packed_shader_.Load();
  vert_colored_shader_.Load();
  animated_shader_.Load();
  eraser_shader_.Load();
  textured_eraser_shader_.Load();
  textured_shader_.Load();
  particle_shader_.Load();
  textured_tint_shader_.Load();
  textured_mask_shader_.Load();
  blur_shader_.Load();
  for (auto& effect : textured_shaders_) {
    effect.second.Load();
  }
}

}  // namespace ink
