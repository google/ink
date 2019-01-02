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

#include "ink/engine/realtime/particle_builder.h"

#include <algorithm>

#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/util/funcs/rand_funcs.h"

namespace ink {

ParticleBuilder::ParticleBuilder(const service::UncheckedRegistry& registry)
    : animation_duration_(2.0),
      shader_metadata_(ShaderMetadata::Default()),
      particle_manager_(registry.GetShared<ParticleManager>()),
      frame_state_(registry.GetShared<FrameState>()),
      resource_manager_(registry.GetShared<GLResourceManager>()),
      earliest_release_time_(0) {}

void ParticleBuilder::SetInitTime(FrameTimeS init_time) {
  shader_metadata_ = ShaderMetadata::Particle(init_time, true);
  if (mesh_) {
    mesh_->shader_metadata = shader_metadata_;
  }
}

void ParticleBuilder::SetupNewLine(glm::vec4 rgba) {
  active_ = true;
  rgba_ = rgba;
  mesh_ = particle_manager_->StartNewMesh();
  mesh_->shader_metadata = shader_metadata_;
  earliest_release_time_ = FrameTimeS(0);
}

void ParticleBuilder::ExtrudeModeledInput(
    const std::vector<input::ModeledInput>& modeled) {
  if (!active_ || modeled.empty()) return;

  if (!input_time_valid_) {
    // Record the first input time for the entire line, and all start/end times
    // for the shader are computed relative to this.
    first_input_time_ = modeled[0].time;
    input_time_valid_ = true;
  }

  for (const input::ModeledInput& mi : modeled) {
    Mesh mesh;
    Rect particle_rect = Rect(mi.world_pos.x - 7.5, mi.world_pos.y - 7.5,
                              mi.world_pos.x + 7.5, mi.world_pos.y + 7.5);
    MakeRectangleMesh(&mesh, particle_rect, glm::vec4{1},
                      particle_rect.CalcTransformTo(Rect(0.0, 0.0, 1.0, 1.0)));

    // Add some "jitter" so that the particles start randomly, otherwise, they
    // look like a wave, which is a fine effect, just not what I want here.
    DurationS jitter =
        Drand(0.0, static_cast<double>(animation_duration_) / 2.0);

    DurationS start_time = mi.time - first_input_time_ + jitter;
    DurationS end_time = start_time + animation_duration_;

    // Each particle will have some random amount of starting x velocity.
    // From -100 to 100 in world coords.
    float init_x_velocity = Drand(-100.0, 100.0);

    for (auto& vert : mesh.verts) {
      vert.color_from = rgba_;
      vert.color = glm::vec4(0.0, 0.0, 0.0, 0.0);
      vert.color_timings = glm::vec2(start_time, end_time);

      // We are using position_from to pass initial velocity.
      vert.position_from = glm::vec2(init_x_velocity, 350);
      vert.position_timings = glm::vec2(start_time, end_time);
    }
    mesh_->Append(mesh);
    earliest_release_time_ = FrameTimeS(std::max(
        earliest_release_time_, frame_state_->GetFrameTime() + end_time));
  }

  resource_manager_->mesh_vbo_provider->ExtendVBO(mesh_, GL_DYNAMIC_DRAW);
}

void ParticleBuilder::Clear() {
  mesh_ = nullptr;
  active_ = false;
  input_time_valid_ = false;
}

void ParticleBuilder::Finalize() {
  if (!active_) return;
  // If the animation cycles, then we don't want the animation stopping in
  // our lifetime.
  FrameTimeS release_time = shader_metadata_.IsCycling()
                                ? FrameTimeS(std::numeric_limits<float>::max())
                                : earliest_release_time_;
  particle_manager_->FinalizeMesh(release_time);
  Clear();
}

}  // namespace ink
