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

#ifndef INK_ENGINE_SCENE_FRAME_STATE_FRAME_STATE_H_
#define INK_ENGINE_SCENE_FRAME_STATE_FRAME_STATE_H_

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/gl.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/current_thread.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class FramerateLock;

class FrameStateListener : public EventListener<FrameStateListener> {
 public:
  virtual void OnFrameEnd() = 0;
};

class FrameState {
 public:
  using SharedDeps = service::Dependencies<IPlatform, IEngineListener>;

  explicit FrameState(std::shared_ptr<IPlatform> platform,
                      std::shared_ptr<IEngineListener> engine_listener);

  void FrameStart(FrameTimeS frame_time);
  void FrameEnd();
  std::unique_ptr<FramerateLock> AcquireFramerateLock(uint32_t min_framerate,
                                                      std::string why);

  // Ensures that we draw at least one more frame at the targeted framerate.
  void RequestFrame();  // Note this is not thread safe.

  // Thread-safe version of the above. Will attempt to ensure that there's only
  // one request outstanding at a time.
  void RequestFrameThreadSafe();

  uint32_t GetMinFramerate() const;
  uint32_t GetFrameNumber() const;
  FrameTimeS GetFrameTime() const;
  FrameTimeS GetLastFrameTime() const;

  void SequencePointReached(int32_t id);

  void AddListener(FrameStateListener* listener);
  void RemoveListener(FrameStateListener* listener);

 private:
  void UpdateControllerFpsIfChanged();
  void UpdateControllerFps();
  void ReleaseFramerateLock(uint32_t min_framerate, std::string why);

  bool is_mid_frame_;
  uint32_t frame_number_;
  FrameTimeS frame_time_;
  FrameTimeS last_frame_time_;
  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<IEngineListener> engine_listener_;
  std::multiset<uint32_t> min_framerate_locks_;

  // Locks that can keep us alive until the next frame.
  std::unique_ptr<FramerateLock> poke_lock_;

  // The last value that was sent to IPlatform->setTargetFPS. Used to
  // reduce the number of IPlatform calls made when lock
  // acquisition/releases happen.
  uint32_t last_targeted_framerate_;

  std::vector<int32_t> sequence_point_ids_;

  std::shared_ptr<EventDispatch<FrameStateListener>> dispatch_;

  // FrameState should only be used from the GL thread. Use a
  // CurrentThreadValidator to assert this.
  CurrentThreadValidator current_thread_validator_;

  std::atomic<bool> threadsafe_frame_requested_;

  friend class FramerateLock;
};

// FramerateLock provides RAII management of a minimum framerate request to the
// FrameState.
class FramerateLock {
 public:
  ~FramerateLock() { frame_state_->ReleaseFramerateLock(framerate_, why_); }

 private:
  FramerateLock(FrameState* frame_state, uint32_t framerate, std::string why)
      : frame_state_(frame_state), framerate_(framerate), why_(why) {}

  // Disallow copy and assign.
  FramerateLock(const FramerateLock&) = delete;
  FramerateLock& operator=(const FramerateLock&) = delete;

  FrameState* frame_state_;
  uint32_t framerate_;
  std::string why_;
  friend class FrameState;
};

class FramerateLimiter : public input::InputHandler {
 public:
  explicit FramerateLimiter(const service::UncheckedRegistry& registry);
  FramerateLimiter(std::shared_ptr<input::InputDispatch> input,
                   std::shared_ptr<FrameState> frame_state);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  bool GetEnabled();
  void SetEnabled(bool enabled);

  inline std::string ToString() const override { return "<FramerateLimiter>"; }

 private:
  bool enabled_;
  std::shared_ptr<FrameState> frame_state_;
  std::unique_ptr<FramerateLock> framerate_lock_;
  uint32_t last_ndown_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_FRAME_STATE_FRAME_STATE_H_
