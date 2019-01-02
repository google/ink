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

#ifndef INK_ENGINE_REALTIME_PARTICLE_BUILDER_H_
#define INK_ENGINE_REALTIME_PARTICLE_BUILDER_H_

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/input/modeled_input.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/particle_manager.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {

/**
 * ParticleBuilder is the particle equivalent to LineBuilder. It converts
 * modeled inputs to meshes that will be displayed by the ParticleManager.
 */
class ParticleBuilder {
 public:
  explicit ParticleBuilder(const service::UncheckedRegistry& registry);

  /**
   * Set the init time that is used for any particle animations.
   */
  void SetInitTime(FrameTimeS init_time);

  /**
   * Begin particle conversion, initializing particle display params and
   * internals.
   */
  void SetupNewLine(glm::vec4 rgba);

  /**
   * Add particles along the "line" defined by the list of modeled inputs.
   * NOTE: Calling this before SetupNewLine (or after Finalize() or Clear()
   * will have no effect.
   */
  void ExtrudeModeledInput(const std::vector<input::ModeledInput>& modeled);

  /**
   * Complete the current "line" of particles.
   * This may grab a FramerateLock to ensure that the current animation
   * completes.
   */
  void Finalize();

  /**
   * Clear out any data relating to the current "line".
   * Calling this before calling Finalize() will result in undefined behavior.
   */
  void Clear();

  /** Grab a pointer to the mesh being constructed for the current "line". */
  const Mesh& mesh() const { return *mesh_; }

 private:
  bool active_ = false;
  DurationS animation_duration_;
  ShaderMetadata shader_metadata_;

  bool input_time_valid_ = false;
  InputTimeS first_input_time_ = InputTimeS(0);

  std::shared_ptr<ParticleManager> particle_manager_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<GLResourceManager> resource_manager_;
  glm::vec4 rgba_{0, 0, 0, 0};

  Mesh* mesh_;
  FrameTimeS earliest_release_time_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_PARTICLE_BUILDER_H_
