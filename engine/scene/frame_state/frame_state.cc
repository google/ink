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

#include "ink/engine/scene/frame_state/frame_state.h"

#include <iterator>

#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

FrameState::FrameState(std::shared_ptr<IPlatform> platform,
                       std::shared_ptr<IEngineListener> engine_listener)
    : is_mid_frame_(false),
      frame_number_(0),
      frame_time_(0),
      last_frame_time_(0),
      platform_(platform),
      engine_listener_(engine_listener),
      last_targeted_framerate_(-1),
      dispatch_(new EventDispatch<FrameStateListener>()),
      current_thread_validator_(),
      threadsafe_frame_requested_(false) {}

void FrameState::FrameStart(FrameTimeS frame_time) {
  current_thread_validator_.CheckIfOnSameThread();
  is_mid_frame_ = true;
  frame_number_++;
  last_frame_time_ = frame_time_;
  frame_time_ = frame_time;

  poke_lock_.reset();
  threadsafe_frame_requested_ = false;
}

void FrameState::FrameEnd() {
  current_thread_validator_.CheckIfOnSameThread();
  is_mid_frame_ = false;

  dispatch_->Send(&FrameStateListener::OnFrameEnd);

  // Trigger a SetTargetFps() at the end of every frame.
  UpdateControllerFps();

  // Notify of all the reached sequence points at the end of the frame.
  for (std::vector<int32_t>::iterator it = sequence_point_ids_.begin();
       it != sequence_point_ids_.end(); ++it) {
    engine_listener_->SequencePointReached(*it);
  }
  sequence_point_ids_.clear();
}

void FrameState::RequestFrame() {
  if (!poke_lock_) {
    poke_lock_ = AcquireFramerateLock(30, "poke");
    UpdateControllerFpsIfChanged();
  }
}

void FrameState::RequestFrameThreadSafe() {
  if (threadsafe_frame_requested_) {
    return;
  }
  threadsafe_frame_requested_ = true;
  // Forward the request to the platform. A frame should run soon.
  platform_->RequestFrame();
}

std::unique_ptr<FramerateLock> FrameState::AcquireFramerateLock(
    uint32_t min_framerate, std::string why) {
  current_thread_validator_.CheckIfOnSameThread();
  min_framerate_locks_.insert(min_framerate);
  UpdateControllerFpsIfChanged();

  SLOG(SLOG_FRAMERATE_LOCKS,
       "acquiring framerate for $0: $1 -- current min is: $2 -- ($3 locks)",
       why, min_framerate, GetMinFramerate(), min_framerate_locks_.size());

  return std::unique_ptr<FramerateLock>(
      new FramerateLock(this, min_framerate, why));
}

void FrameState::ReleaseFramerateLock(uint32_t min_framerate, std::string why) {
  current_thread_validator_.CheckIfOnSameThread();
  auto f = min_framerate_locks_.find(min_framerate);
  EXPECT(f != min_framerate_locks_.end());
  min_framerate_locks_.erase(f);
  UpdateControllerFpsIfChanged();

  SLOG(SLOG_FRAMERATE_LOCKS,
       "releasing framerate taken for $0: $1 -- current min is: $2 -- ($3 "
       "locks)",
       why.c_str(), min_framerate, GetMinFramerate(),
       min_framerate_locks_.size());
}

uint32_t FrameState::GetMinFramerate() const {
  current_thread_validator_.CheckIfOnSameThread();
  if (!min_framerate_locks_.empty()) {
    return *min_framerate_locks_.rbegin();
  } else {
    return 0;
  }
}

void FrameState::UpdateControllerFpsIfChanged() {
  current_thread_validator_.CheckIfOnSameThread();

  // If we aren't currently inside a frame, dispatch a callback if the targeted
  // FPS has changed.
  if (GetMinFramerate() != last_targeted_framerate_) {
    UpdateControllerFps();
  }
}

void FrameState::UpdateControllerFps() {
  // If we are in the middle of a draw, don't call the host to avoid unnecessary
  // calls. We'll dispatch a SetTargetFps once the frame is done.
  if (is_mid_frame_) {
    return;
  }

  uint32_t min_framerate = GetMinFramerate();
  platform_->SetTargetFPS(min_framerate);
  last_targeted_framerate_ = min_framerate;
}

uint32_t FrameState::GetFrameNumber() const { return frame_number_; }

FrameTimeS FrameState::GetFrameTime() const { return frame_time_; }

FrameTimeS FrameState::GetLastFrameTime() const { return last_frame_time_; }

void FrameState::SequencePointReached(int32_t id) {
  current_thread_validator_.CheckIfOnSameThread();
  sequence_point_ids_.push_back(id);
}

void FrameState::AddListener(FrameStateListener* listener) {
  listener->RegisterOnDispatch(dispatch_);
}

void FrameState::RemoveListener(FrameStateListener* listener) {
  listener->Unregister(dispatch_);
}

/////////////////////////////////////////

FramerateLimiter::FramerateLimiter(const service::UncheckedRegistry& registry)
    : FramerateLimiter(registry.GetShared<input::InputDispatch>(),
                       registry.GetShared<FrameState>()) {}
FramerateLimiter::FramerateLimiter(std::shared_ptr<input::InputDispatch> input,
                                   std::shared_ptr<FrameState> frame_state)
    : input::InputHandler(input::Priority::ObserveOnly),
      enabled_(true),
      frame_state_(std::move(frame_state)),
      last_ndown_(0) {
  RegisterForInput(input);
}

input::CaptureResult FramerateLimiter::OnInput(const input::InputData& data,
                                               const Camera& camera) {
  if (!enabled_) return input::CapResObserve;

  if (data.id == input::MouseIds::MouseWheel) {
    // Draw at least one frame for wheel events.
    frame_state_->RequestFrame();
    return input::CapResObserve;
  }

  if (last_ndown_ == data.n_down) return input::CapResObserve;
  switch (data.n_down) {
    case 0:
      // Release the ongoing framelock, but ensure we draw one more frame.
      frame_state_->RequestFrame();
      framerate_lock_.reset();
      break;
    case 1:
      if (data.Get(input::Flag::Right)) {
        framerate_lock_ =
            frame_state_->AcquireFramerateLock(30, "right mouse button down");
      } else {
        framerate_lock_ =
            frame_state_->AcquireFramerateLock(60, "single input down");
      }
      break;
    default:
      framerate_lock_ =
          frame_state_->AcquireFramerateLock(30, "multiple inputs down");
      break;
  }

  last_ndown_ = data.n_down;
  return input::CapResObserve;
}

bool FramerateLimiter::GetEnabled() { return enabled_; }

void FramerateLimiter::SetEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled_) {
    framerate_lock_.reset();
  }
}

}  // namespace ink
