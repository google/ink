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

#ifndef INK_ENGINE_RENDERING_SHADERS_PACKED_MESH_SHADERS_H_
#define INK_ENGINE_RENDERING_SHADERS_PACKED_MESH_SHADERS_H_

#include <memory>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/shaders/shader.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace shaders {

// Shader for an optimized mesh with VertFormat::x12y12.
class PackedShaderX12Y12 : public Shader {
 public:
  explicit PackedShaderX12Y12(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
};

// Shader for an optimized mesh for erasing to the background texture.
class PackedShaderX12Y12Textured : public Shader {
 public:
  explicit PackedShaderX12Y12Textured(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
};

// Shader for an optimized mesh with VertFormat::x32y32.
class PackedShaderX32Y32 : public Shader {
 public:
  explicit PackedShaderX32Y32(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
};

// Shader for an optimized mesh with VertFormat::x11a7r6y11g7b6.
class PackedShaderX11A7R6Y11G7B6 : public Shader {
 public:
  explicit PackedShaderX11A7R6Y11G7B6(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
};

// Shader for an optimized mesh with VertFormat::x11a7r6y11g7b6u12v12
class PackedShaderX11A7R6Y11G7B6U12V12 : public Shader {
 public:
  explicit PackedShaderX11A7R6Y11G7B6U12V12(
      ion::gfx::GraphicsManagerPtr gl,
      std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;
};

// Can draw an OptimizedMesh.  Delegates to one of the Shaders
// above based on the OptimizedMesh data format.
class PackedVertShader {
 public:
  PackedVertShader(ion::gfx::GraphicsManagerPtr gl,
                   std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
                   std::shared_ptr<BackgroundState> background_state,
                   std::shared_ptr<TextureManager> texture_manager);
  void Load();
  void Draw(const OptimizedMesh& mesh) const;

  void Use(const Camera& cam, const OptimizedMesh& mesh) const;
  void Unuse(const OptimizedMesh& mesh) const;

 private:
  const Shader* GetShader(const OptimizedMesh& mesh) const;
  bool ShouldDrawAsEraserTexture(const OptimizedMesh& mesh) const;

  ion::gfx::GraphicsManagerPtr gl_;
  std::shared_ptr<BackgroundState> background_state_;
  std::shared_ptr<MeshVBOProvider> mesh_vbo_provider_;
  std::shared_ptr<TextureManager> texture_manager_;
  PackedShaderX12Y12 shader_x12y12_;
  PackedShaderX12Y12Textured shader_x12y12textured_;
  PackedShaderX32Y32 shader_x32y32_;
  PackedShaderX11A7R6Y11G7B6 shader_x11a7r6y11g7b6_;
  PackedShaderX11A7R6Y11G7B6U12V12 shader_x11a7r6y11g7b6u12v12_;
};

}  // namespace shaders
}  // namespace ink

#endif  // INK_ENGINE_RENDERING_SHADERS_PACKED_MESH_SHADERS_H_
