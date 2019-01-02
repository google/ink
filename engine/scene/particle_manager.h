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

#ifndef INK_ENGINE_SCENE_PARTICLE_MANAGER_H_
#define INK_ENGINE_SCENE_PARTICLE_MANAGER_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

/**
 * ParticleManager manages allocation/deallocation of meshes for particles.
 * It also ensures that Draw is called while particle animations are active.
 */
class ParticleManager : public IDrawable {
 public:
  using SharedDeps = service::Dependencies<GLResourceManager, FrameState>;

  explicit ParticleManager(std::shared_ptr<GLResourceManager> gl_resources,
                           std::shared_ptr<FrameState> frame_state)
      : frame_state_(frame_state), renderer_(gl_resources) {}

  /**
   * On each Update, removes meshes for any particles which have completed their
   * animations. Also, releases the FramerateLock, if no animations are active.
   */
  void Update(FrameTimeS draw_time);

  /**
   * Draws any meshes for particles with active animations, including the
   * "current" mesh which may be under construction.
   */
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

  /**
   * Begin a new "current" mesh and return it to the caller for construction.
   * Calling StartNewMesh while there is a "current" mesh is an error.
   */
  Mesh* StartNewMesh();
  /**
   * Finalize the "current" mesh and ensure that animations will be run until
   * lock_until_time. After this call, there will be no "current" mesh.
   */
  void FinalizeMesh(FrameTimeS lock_until_time);

 private:
  struct MeshWrapper {
    MeshWrapper() : earliest_release_time(0) {}

    FrameTimeS earliest_release_time;
    std::unique_ptr<Mesh> mesh;
  };

  MeshWrapper current_mesh_;
  std::vector<MeshWrapper> completed_meshes_;
  std::unique_ptr<FramerateLock> frame_lock_;

  std::shared_ptr<FrameState> frame_state_;
  MeshRenderer renderer_;
};

}  // namespace ink

#endif  //  INK_ENGINE_SCENE_PARTICLE_MANAGER_H_
