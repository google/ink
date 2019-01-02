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

#ifndef INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_CONTROLLER_H_
#define INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_CONTROLLER_H_

#include <memory>

#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/animation/sequential_animation.h"

namespace ink {

class ElementAnimationController {
 public:
  using SharedDeps = service::Dependencies<AnimationController>;

  explicit ElementAnimationController(
      std::shared_ptr<AnimationController> anim_controller)
      : anim_controller_(anim_controller) {}

  // Disallow copy and assign.
  ElementAnimationController(const ElementAnimationController&) = delete;
  ElementAnimationController& operator=(const ElementAnimationController&) =
      delete;

  void PushAnimation(const ElementId& id,
                     std::unique_ptr<Animation> elem_anim) {
    if (animations_.find(id) == animations_.end()) {
      std::unique_ptr<SequentialAnimation> sa(new SequentialAnimation());
      animations_.emplace(id, std::move(sa));
    }

    auto& sa = animations_[id];

    // If the SequentialAnimation isn't currently running anything, we'll need
    // to register it on the AnimatioController (after all of the sequential
    // animations have run it will unregister itself).
    if (sa->HasFinished()) {
      anim_controller_->AddListener(sa.get());
    }

    sa->Push(std::move(elem_anim));
  }

 private:
  std::shared_ptr<AnimationController> anim_controller_;

  std::unordered_map<ElementId, std::unique_ptr<SequentialAnimation>,
                     ElementIdHasher>
      animations_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_CONTROLLER_H_
