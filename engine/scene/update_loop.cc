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

#include "ink/engine/scene/update_loop.h"

namespace ink {

DefaultUpdateLoop::DefaultUpdateLoop(
    const service::Registry<DefaultUpdateLoop>& registry)
    : anim_(registry.GetShared<AnimationController>()),
      tasks_(registry.GetShared<ITaskRunner>()),
      graph_(registry.GetShared<SceneGraph>()),
      tools_(registry.GetShared<ToolController>()),
      graph_renderer_(registry.GetShared<LiveRenderer>()),
      cam_(registry.GetShared<Camera>()),
      clock_(registry.GetShared<WallClockInterface>()),
      particle_manager_(registry.GetShared<ParticleManager>()),
      crop_mode_(registry.GetShared<CropMode>()),
      cursor_manager_(registry.GetShared<input::CursorManager>()),
      debug_view_(registry.GetShared<DebugView>()),
      logging_timer_(new LoggingPerfTimer(clock_, "update time")) {}

void DefaultUpdateLoop::Update(float target_fps, FrameTimeS t) {
  auto buffer = 6.0 / 1000.0;
  auto target_update_time = 1.0 / target_fps - buffer;
  target_update_time = std::max(target_update_time, 1.0 / 1000);
  Timer update_timer(clock_, target_update_time);

  logging_timer_->Begin();
  anim_->UpdateAnimations();
  tasks_->ServiceMainThreadTasks();
  graph_->Update(*cam_);
  tools_->Update(*cam_, t);
  particle_manager_->Update(t);
  graph_renderer_->Update(update_timer, *cam_, t);
  crop_mode_->Update(*cam_);
  cursor_manager_->Update(*cam_);
  debug_view_->Update(t);
  logging_timer_->End();
}

}  // namespace ink
