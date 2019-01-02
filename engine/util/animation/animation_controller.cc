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

#include "ink/engine/util/animation/animation_controller.h"

#include <vector>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

AnimationController::AnimationController(
    std::shared_ptr<FrameState> frame_state)
    : dispatch_(new EventDispatch<Animation>()), frame_state_(frame_state) {}

void AnimationController::UpdateAnimations() {
  SLOG(SLOG_ANIMATION, "updating animations ($0 targets)", dispatch_->size());
  dispatch_->Send(&Animation::Update, frame_state_->GetFrameTime());

  if (dispatch_->size() == 0) {
    frame_lock_.reset();
  }
}

void AnimationController::AddListener(Animation* listener) {
  if (!frame_lock_) {
    frame_lock_ = frame_state_->AcquireFramerateLock(60, "animation");
  }
  listener->RegisterOnDispatch(dispatch_);
}

void AnimationController::RemoveListener(Animation* listener) {
  listener->Unregister(dispatch_);
}

}  // namespace ink
