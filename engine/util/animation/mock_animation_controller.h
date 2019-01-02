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

#ifndef INK_ENGINE_UTIL_ANIMATION_MOCK_ANIMATION_CONTROLLER_H_
#define INK_ENGINE_UTIL_ANIMATION_MOCK_ANIMATION_CONTROLLER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/public/host/mock_engine_listener.h"
#include "ink/engine/public/host/mock_platform.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class MockAnimationController : public AnimationController {
 public:
  MockAnimationController()
      : AnimationController(std::make_shared<FrameState>(
            std::make_shared<MockPlatform>(),
            std::make_shared<MockEngineListener>())) {}

  std::shared_ptr<EventDispatch<Animation>> GetEventDispatch() {
    return dispatch_;
  }

  void RunFrame(FrameTimeS at_time) {
    frame_state_->FrameStart(at_time);
    UpdateAnimations();
    frame_state_->FrameEnd();
  }
};

};  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_MOCK_ANIMATION_CONTROLLER_H_
