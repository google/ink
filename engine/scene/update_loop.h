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

#ifndef INK_ENGINE_SCENE_UPDATE_LOOP_H_
#define INK_ENGINE_SCENE_UPDATE_LOOP_H_

#include "ink/engine/util/time/time_types.h"

#include "ink/engine/debug_view/debug_view.h"
#include "ink/engine/input/cursor_manager.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/realtime/crop_mode.h"
#include "ink/engine/realtime/tool_controller.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/particle_manager.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/time/logging_perf_timer.h"

namespace ink {

// Allow replacing the update loop itself. Longform is the only thing that
// currently uses this - it could otherwise be folded into root_controller.
class UpdateLoop {
 public:
  virtual ~UpdateLoop() {}
  virtual void Update(float target_fps, FrameTimeS t) = 0;
};

class DefaultUpdateLoop : public UpdateLoop {
 public:
  using SharedDeps =
      service::Dependencies<AnimationController, ITaskRunner, SceneGraph,
                            ToolController, LiveRenderer, Camera,
                            WallClockInterface, ParticleManager, CropMode,
                            input::CursorManager, DebugView>;

  explicit DefaultUpdateLoop(
      const service::Registry<DefaultUpdateLoop>& registry);

  void Update(float target_fps, FrameTimeS t) override;

 private:
  std::shared_ptr<AnimationController> anim_;
  std::shared_ptr<ITaskRunner> tasks_;
  std::shared_ptr<SceneGraph> graph_;
  std::shared_ptr<ToolController> tools_;
  std::shared_ptr<SceneGraphRenderer> graph_renderer_;
  std::shared_ptr<Camera> cam_;
  std::shared_ptr<WallClockInterface> clock_;
  std::shared_ptr<ParticleManager> particle_manager_;
  std::shared_ptr<CropMode> crop_mode_;
  std::shared_ptr<input::CursorManager> cursor_manager_;
  std::shared_ptr<DebugView> debug_view_;

  std::unique_ptr<LoggingPerfTimer> logging_timer_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_UPDATE_LOOP_H_
