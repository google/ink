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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_SINGLE_PARTITION_RENDERER_H_
#define INK_ENGINE_RENDERING_COMPOSITING_SINGLE_PARTITION_RENDERER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/baseGL/render_target.h"
#include "ink/engine/rendering/compositing/dbrender_target.h"
#include "ink/engine/rendering/compositing/partition_data.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/element_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/stopwatch.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

// Renderer for a single partition of data. Designed for cooperative
// multitasking, this splits large rendering jobs over multiple frames.
// Changing the data partition causes the internal caches to invalidate, and
// renders this class unable to draw for an arbitrary number of frames.
//
// Due to the non-blocking design, rendering results from this class
// are provided via caches. The owner of this class is expected to watch
// the cache state and hold framelocks + call update() for all states other
// than PartitionCacheState::Complete
// Repeated calls to update() will converge to the "complete" state
//
// If the owner instead wishes to have the framelock automatically adjusted,
// call enableFramerateLocks().
//
// If you intend to change the partition data assigned to this, consider a
// MultiPartitionRenderer instead to avoid frame misses
class SinglePartitionRenderer {
 public:
  explicit SinglePartitionRenderer(const service::UncheckedRegistry& registry)
      : SinglePartitionRenderer(registry.GetShared<WallClockInterface>(),
                                registry.GetShared<GLResourceManager>()) {}
  explicit SinglePartitionRenderer(
      std::shared_ptr<WallClockInterface> wall_clock,
      std::shared_ptr<GLResourceManager> gl_resources);

  // blit_attrs::Blit the cache of partition to the screen.
  void Draw(const Camera& cam, FrameTimeS draw_time, const SceneGraph& graph,
            const blit_attrs::BlitAttrs& attrs) const;

  // blit_attrs::Blit the cache of partition to the screen. worldDest and
  // worldSrc are
  // used to specify the output mapping.
  // worldSrc is the read from rectangle that is taken from the cache
  // worldDst is where the result of the worldSrc read is mapped to
  void Draw(const Camera& cam, FrameTimeS draw_time, const SceneGraph& graph,
            const blit_attrs::BlitAttrs& attrs, glm::mat4 transform) const;

  // Update any caches of the partition. May not complete in one frame.
  // Watch the return value and hold a framelock accordingly.
  PartitionCacheState Update(const Timer& frame_timer, const Camera& cam,
                             FrameTimeS draw_time, const SceneGraph& graph);

  void Resize(glm::ivec2 size);
  glm::ivec2 RenderingSize() const;

  PartitionCacheState CacheState() const;

  // The partition that will be drawn in response to a call to "draw"
  PartitionData::ParId CurrentPartition() const;

  // Request that this renderer prepare to render the data described by p.
  // When the data is availible to render "isComplete()" will return true.
  void AssignPartitionData(const PartitionData& p);

  // Disabled by default. If enabled, this class will hold a framerate lock
  // whenever cacheState() is not Complete. This allows the cache to be
  // as up to date as possible. The owner is responsible for calling
  // update() on every every frame, which will permit this object to release
  // its framelock when its cacheState() is Complete.
  void EnableFramerateLocks(std::shared_ptr<FrameState> frame_state);
  void DisableFramerateLocks();

  // If true, drastically reduces draw throughput to help find timing bugs.
  void DBGshouldDrawSlow(bool draw_slow) { dbgshould_draw_slow_ = draw_slow; }

 private:
  void UpdateFramelocks();
  void InvalidateBack();

  // returns the number of elements drawn
  int UpdateBack(const Timer& frame_timer, const SceneGraph& graph);
  bool IsPartitionFullyRendered() const;

  PartitionCacheState cache_state_;
  DBRenderTarget buffer_;
  PartitionData partition_;
  int partition_group_index_ = 0;
  int partition_element_index_ = 0;
  Camera back_camera_;
  FrameTimeS back_time_;
  bool dbgshould_draw_slow_;

  // If the state of the backbuffer is valid. If false the renderer will restart
  // work on the back buffer next update cycle.
  bool should_restart_back_;

  // Tracks wall time spent on an individual update to the back buffer
  Stopwatch back_update_timer_;
  signal_filters::ExpMovingAvg<DurationS, double> avg_back_draw_time_;

  std::shared_ptr<GLResourceManager> gl_resources_;
  ElementRenderer element_renderer_;

  // If framerateLocksEnabled_, then frameState_ is expected to be non-null.
  bool framerate_locks_enabled_;
  std::shared_ptr<FrameState> frame_state_;
  std::unique_ptr<FramerateLock> frame_lock_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_COMPOSITING_SINGLE_PARTITION_RENDERER_H_
