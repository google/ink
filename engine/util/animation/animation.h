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

#ifndef INK_ENGINE_UTIL_ANIMATION_ANIMATION_H_
#define INK_ENGINE_UTIL_ANIMATION_ANIMATION_H_

#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class Animation : public EventListener<Animation> {
 public:
  Animation() : EventListener<Animation>(), last_update_time_(0) {}
  void Update(FrameTimeS t) {
    UpdateImpl(t);
    last_update_time_ = t;
    if (HasFinished()) {
      if (on_finished_) on_finished_();
      UnregisterFromAll();
    }
  }

  virtual void UpdateImpl(FrameTimeS t) = 0;

  virtual bool HasFinished() const { return false; }

  virtual void SetOnFinishedFn(std::function<void()> fn) { on_finished_ = fn; }

 protected:
  FrameTimeS last_update_time_;
  std::function<void()> on_finished_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_ANIMATION_H_
