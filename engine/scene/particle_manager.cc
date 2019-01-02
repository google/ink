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

#include "ink/engine/scene/particle_manager.h"

#include "third_party/absl/memory/memory.h"

namespace ink {

Mesh* ParticleManager::StartNewMesh() {
  ASSERT(current_mesh_.mesh == nullptr);  // Did we call finalize?

  current_mesh_.mesh = absl::make_unique<Mesh>();
  return current_mesh_.mesh.get();
}

void ParticleManager::FinalizeMesh(FrameTimeS lock_until_time) {
  if (!frame_lock_) {
    frame_lock_ = frame_state_->AcquireFramerateLock(60, "Particles");
  }

  current_mesh_.earliest_release_time = lock_until_time;
  completed_meshes_.emplace_back(std::move(current_mesh_));

  ASSERT(current_mesh_.mesh == nullptr);  // Did we call finalize?
}

void ParticleManager::Update(FrameTimeS draw_time) {
  // Remove any particle meshes which are done animating.
  auto new_end =
      std::remove_if(completed_meshes_.begin(), completed_meshes_.end(),
                     [draw_time](const MeshWrapper& wrapper) {
                       return draw_time > wrapper.earliest_release_time;
                     });
  completed_meshes_.erase(new_end, completed_meshes_.end());

  if (completed_meshes_.empty() && frame_lock_) {
    frame_lock_.reset();
  }
}

void ParticleManager::Draw(const Camera& cam, FrameTimeS draw_time) const {
  for (const auto& completed_mesh : completed_meshes_) {
    renderer_.Draw(cam, draw_time, *completed_mesh.mesh);
  }
  Mesh* active_mesh = current_mesh_.mesh.get();
  if (active_mesh) {
    renderer_.Draw(cam, draw_time, *active_mesh);
  }
}

}  // namespace ink
