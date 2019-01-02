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

#ifndef INK_ENGINE_UTIL_ANIMATION_ANIMATION_CONTROLLER_H_
#define INK_ENGINE_UTIL_ANIMATION_ANIMATION_CONTROLLER_H_

#include <memory>
#include <unordered_set>

#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/animation/animation.h"

namespace ink {

// Provides "UpdateAnimation" calls to animators
class AnimationController {
 public:
  using SharedDeps = service::Dependencies<FrameState>;

  explicit AnimationController(std::shared_ptr<FrameState> frame_state);
  void UpdateAnimations();

  // Adding an animation listener will hold the framerate at 60fps!
  //
  // Make sure to release the listener when you're done!
  void AddListener(Animation* listener);
  void RemoveListener(Animation* listener);

  size_t size() const { return dispatch_->size(); }

 private:
  std::shared_ptr<EventDispatch<Animation>> dispatch_;
  std::shared_ptr<FrameState> frame_state_;
  std::unique_ptr<FramerateLock> frame_lock_;
  std::unordered_set<std::unique_ptr<Animation>> held_anims_;

  friend class MockAnimationController;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_ANIMATION_CONTROLLER_H_
