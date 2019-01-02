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

#ifndef INK_ENGINE_RENDERING_SHADERS_MESH_SHADER_H_
#define INK_ENGINE_RENDERING_SHADERS_MESH_SHADER_H_

#include <memory>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/rendering/shaders/shader.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace shaders {

// Can draw a Mesh or vector<Vertex> with position and color data per vertex.
class VertColoredShader : public Shader {
 public:
  VertColoredShader(ion::gfx::GraphicsManagerPtr gl,
                    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;
  void UseIdentity() const;
};

// Draws a mesh with the background color
//
// The shader fails fast if either: (1) the input mesh is not marked as eraser,
// or (2) the current background is not a solid color.
class SolidEraserShader : public Shader {
 public:
  SolidEraserShader(ion::gfx::GraphicsManagerPtr gl,
                    std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
                    std::shared_ptr<BackgroundState> background_state);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override {
    EXPECT(false);
  }  // must call with camera and color
  void Use(const Camera& cam) const;

 private:
  std::shared_ptr<BackgroundState> background_state_;
};

// Draws a mesh that samples from the background texture.
//
// The shader fails fast if either: (1) the input mesh is not marked as eraser,
// or (2) the current background is not textured.
class TexturedEraserShader : public Shader {
 public:
  TexturedEraserShader(ion::gfx::GraphicsManagerPtr gl,
                       std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
                       std::shared_ptr<BackgroundState> background_state,
                       std::shared_ptr<TextureManager> texture_manager);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;

 private:
  std::shared_ptr<BackgroundState> background_state_;
  std::shared_ptr<TextureManager> texture_manager_;
};

// Draws a Mesh that specifies texture data
class VertTexturedShader : public Shader {
 public:
  VertTexturedShader(ion::gfx::GraphicsManagerPtr gl,
                     std::shared_ptr<MeshVBOProvider> mesh_vbo_provider,
                     std::shared_ptr<TextureManager> texture_manager);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera
  void Use(const Camera& cam) const;

 private:
  std::shared_ptr<TextureManager> texture_manager_;
};

// Draws a Mesh that specifies animation data
class AnimatedShader : public Shader {
 public:
  AnimatedShader(ion::gfx::GraphicsManagerPtr gl,
                 std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera & time
  void Use(const Camera& cam, DurationS time_since_init) const;
};

// Draws a Mesh with particle animation.
class ParticleShader : public Shader {
 public:
  ParticleShader(ion::gfx::GraphicsManagerPtr gl,
                 std::shared_ptr<MeshVBOProvider> mesh_vbo_provider);
  void Load() override;

  void Draw(const Mesh& mesh) const;

  void Use() const override { EXPECT(false); }  // must call with camera & time
  void Use(const Camera& cam, DurationS time_since_init) const;
};

}  // namespace shaders
}  // namespace ink
#endif  // INK_ENGINE_RENDERING_SHADERS_MESH_SHADER_H_
