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

#include "ink/engine/rendering/compositing/single_partition_renderer.h"

#include <unistd.h>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/stopwatch.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {
using util::Lerp;
using util::Normalize;

SinglePartitionRenderer::SinglePartitionRenderer(
    std::shared_ptr<WallClockInterface> wall_clock,
    std::shared_ptr<GLResourceManager> gl_resources)
    : cache_state_(PartitionCacheState::Incomplete),
      buffer_(wall_clock, gl_resources, TextureMapping::Linear),
      dbgshould_draw_slow_(false),
      should_restart_back_(false),
      back_update_timer_(wall_clock),
      gl_resources_(gl_resources),
      element_renderer_(gl_resources),
      framerate_locks_enabled_(false) {}

// Returns true is the cache is drawable.
bool CheckPartitionCacheStateForDraw(PartitionCacheState cache_state,
                                     const PartitionData& partition) {
  if (cache_state == PartitionCacheState::Incomplete) {
    SLOG(SLOG_DRAWING,
         "attempting to draw a partition, but result is not complete! ($0)",
         partition);
    return false;
  } else {
    SLOG(SLOG_DRAWING, "partition renderer drawing partition $0", partition);
    return true;
  }
}

void SinglePartitionRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                                   const SceneGraph& graph,
                                   const blit_attrs::BlitAttrs& attrs) const {
  Draw(cam, draw_time, graph, attrs, glm::mat4{1});
}

void SinglePartitionRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                                   const SceneGraph& graph,
                                   const blit_attrs::BlitAttrs& attrs,
                                   glm::mat4 transform) const {
  if (CheckPartitionCacheStateForDraw(cache_state_, partition_)) {
    RotRect window = back_camera_.WorldRotRect();
    buffer_.DrawFront(cam, attrs, RotRect(buffer_.Bounds()),
                      geometry::Transform(window, transform));
  }
}

bool SinglePartitionRenderer::IsPartitionFullyRendered() const {
  return partition_group_index_ == partition_.elements.size();
}

PartitionCacheState SinglePartitionRenderer::Update(const Timer& frame_timer,
                                                    const Camera& cam,
                                                    FrameTimeS draw_time,
                                                    const SceneGraph& graph) {
  if (partition_.id == PartitionData::invalid_partition) {
    ASSERT(cache_state_ == PartitionCacheState::Incomplete);
    return cache_state_;
  }

  // Determine if we should restart work
  if (cam.WorldWindow() != back_camera_.WorldWindow()) {
    // The target viewport (cam) is changing, which means we all buffers need to
    // be (eventually) updated.
    if (cache_state_ == PartitionCacheState::Complete) {
      cache_state_ = PartitionCacheState::OutOfDate;
    }

    // Restart the worker buffer (buffer_.back) in a few cases:
    //   1) back is complete. There's no point in keeping it around anymore
    //   2) back is not complete, but not expected to complete in a reasonable
    //   amount of time
    //
    // For case 2, the comparison roughly translates to
    // tp = [0.7, 0.1], linearly correlating to the amount of overlap between
    //      the complete buffer (front) and the in progress buffer (back)
    //      When a large amount of data overlap is detected, tp will be near
    //      0.1.
    //      When there's not much shared between the complete and inprogress
    //      buffer tp will approach 0.7.
    // https://www.wolframalpha.com/input/?i=0.2+%2B+%280.6+-+0.2%29+*+%28%28p+-+0.8%29+%2F+%280.1+-+0.8%29%29+with+p+from+0+to+1
    //
    // if ( timeAlreadySpent <
    //      [0.7(=not much overlap), 0.1(=lots of overlap)] * expectedTime) {
    //    restart;
    // }
    //
    // Thus, we are more likely to restart if there is not much overlap between
    // the completed data and the in progress data, which would happen ex if
    // we're rapidly panning.
    float coverage = 0;
    Rect intersection;
    if (geometry::Intersection(back_camera_.WorldWindow(), cam.WorldWindow(),
                               &intersection))
      coverage = intersection.Area() / back_camera_.WorldWindow().Area();
    auto p = Lerp(0.2f, 0.6f, Normalize(0.8f, 0.1f, coverage));
    if (IsPartitionFullyRendered() ||  // case 1
        back_update_timer_.Elapsed() <
            p * avg_back_draw_time_.Value()) {  // case 2
      should_restart_back_ = true;
    }
  }

  bool did_restart_back = false;
  if (should_restart_back_) {
    SLOG(SLOG_DRAWING, "PartitionData renderer restarting back");
    // Reset to restart work on the inprogress back buffer
    ASSERT(cache_state_ != PartitionCacheState::Complete);
    partition_group_index_ = 0;
    partition_element_index_ = 0;
    back_camera_ = cam;
    back_time_ = draw_time;
    buffer_.ClearBack();
    should_restart_back_ = false;
    did_restart_back = true;
  }

  // Draw
  bool did_complete_back_this_frame = false;
  if (cache_state_ != PartitionCacheState::Complete) {
    auto draw_count = UpdateBack(frame_timer, graph);

    // Tracking if we've completed this frame prevents unnecessary blits
    // if the camera and partition are unchanging
    did_complete_back_this_frame =
        IsPartitionFullyRendered() && (draw_count > 0 || did_restart_back);
  }

  // Check for completion
  if (did_complete_back_this_frame) {
    SLOG(SLOG_DRAWING,
         "PartitionData renderer blitting inprogress backToFront");

    buffer_.BlitBackToFront();

    if (cam.WorldWindow() == back_camera_.WorldWindow()) {
      cache_state_ = PartitionCacheState::Complete;
    } else {
      cache_state_ = PartitionCacheState::OutOfDate;
      InvalidateBack();
    }

    avg_back_draw_time_.Sample(back_update_timer_.Elapsed());
    back_update_timer_.Reset();
  }

  UpdateFramelocks();
  return cache_state_;
}

int SinglePartitionRenderer::UpdateBack(const Timer& frame_timer,
                                        const SceneGraph& graph) {
  SLOG(SLOG_DRAWING, "PartitionData renderer drawing to back");
  back_update_timer_.Resume();
  buffer_.BindBack();

  // The number of draws we've done, but normalized so "small" draws count less
  // and "large" draws count more
  float adjusted_draw_count = 0;

  // Exact number of elements we've attempted to draw
  int actual_draw_count = 0;

  // Main draw loop
  const float batch_size = 2;
  while (!IsPartitionFullyRendered()) {
#if INK_DEBUG
    if (dbgshould_draw_slow_) {
      usleep(10 * 1000);  // 10ms
    }
#endif

    const auto& group = partition_.elements[partition_group_index_];
    ElementId element = group.poly_ids[partition_element_index_];
    std::unique_ptr<Scissor> scissor;
    GroupId parent = graph.GetParentGroupId(element);
    if (graph.IsClippableGroup(parent)) {
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(back_camera_, graph.Mbr({parent}), CoordType::kWorld);
    }

    if (element_renderer_.Draw(element, graph, back_camera_, back_time_)) {
      float cvg = graph.Coverage(back_camera_, element);
      adjusted_draw_count += Lerp(0.25f, 1.0f, Normalize(0.0f, 0.4f, cvg));
    }
    actual_draw_count++;
    partition_element_index_++;
    if (partition_element_index_ == group.poly_ids.size()) {
      partition_group_index_++;
      partition_element_index_ = 0;
    }

    // Allow at least one batch through even if we're out of time. This way
    // we'll always (eventually) converge
    if (adjusted_draw_count > batch_size && frame_timer.Expired()) break;
  }
  back_update_timer_.Pause();
  return actual_draw_count;
}

void SinglePartitionRenderer::UpdateFramelocks() {
  if (framerate_locks_enabled_ &&
      cache_state_ != PartitionCacheState::Complete &&
      partition_.id != PartitionData::invalid_partition) {
    EXPECT(frame_state_);
    frame_lock_ = frame_state_->AcquireFramerateLock(
        30, "1-partition renderer valid and incomplete");
  } else {
    frame_lock_.reset();
  }
}

void SinglePartitionRenderer::EnableFramerateLocks(
    std::shared_ptr<FrameState> frame_state) {
  EXPECT(frame_state);
  frame_state_ = frame_state;
  framerate_locks_enabled_ = true;
  UpdateFramelocks();
}

void SinglePartitionRenderer::DisableFramerateLocks() {
  frame_state_.reset();
  framerate_locks_enabled_ = false;
  UpdateFramelocks();
}

void SinglePartitionRenderer::InvalidateBack() {
  SLOG(SLOG_DRAWING,
       "PartitionData renderer invalidating inprogress back buffer");
  should_restart_back_ = true;
  if (cache_state_ == PartitionCacheState::Complete) {
    cache_state_ = PartitionCacheState::OutOfDate;
  }
  UpdateFramelocks();
}

void SinglePartitionRenderer::Resize(glm::ivec2 size) {
  buffer_.Resize(size);
  InvalidateBack();
}

glm::ivec2 SinglePartitionRenderer::RenderingSize() const {
  return buffer_.GetSize();
}

PartitionCacheState SinglePartitionRenderer::CacheState() const {
  return cache_state_;
}

PartitionData::ParId SinglePartitionRenderer::CurrentPartition() const {
  return partition_.id;
}

void SinglePartitionRenderer::AssignPartitionData(const PartitionData& p) {
  SLOG(SLOG_DATA_FLOW, "Partition renderer $0 assigned data $1",
       AddressStr(this), p);
  for (const auto& group : p.elements) {
    SLOG(SLOG_DATA_FLOW, "Rendering group: $0", group.group_id);
    SLOG(SLOG_DATA_FLOW, "partition:", group.poly_ids);
  }
  partition_ = p;
  cache_state_ = PartitionCacheState::Incomplete;
  InvalidateBack();
}

}  // namespace ink
