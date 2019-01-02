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

#ifndef INK_ENGINE_UTIL_ANIMATION_ANIMATED_FN_H_
#define INK_ENGINE_UTIL_ANIMATION_ANIMATED_FN_H_

#include <functional>
#include <memory>

#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/animation/animation_curve.h"
#include "ink/engine/util/animation/fixed_interp_animation.h"
#include "ink/engine/util/animation/interpolator.h"
#include "ink/engine/util/animation/repeating_interp_animation.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// An AnimatedFn<T> uses FixedInterpAnimations (at most one at a time) to
// animate over getters/setter lambdas. Any new AnimateTo will stop the existing
// animation, and the next animation frame the getter to be called to determine
// the position to start from.
template <typename T>
class AnimatedFn {
 public:
  AnimatedFn(std::shared_ptr<AnimationController> ac, std::function<T()> get_fn,
             std::function<void(const T&)> set_fn)
      : anim_controller_(ac), get_fn_(get_fn), set_fn_(set_fn) {}

  void AnimateTo(T to, DurationS d) {
    animation_.reset(new FixedInterpAnimation<T>(d, to, get_fn_, set_fn_));
    anim_controller_->AddListener(animation_.get());
  }

  void AnimateTo(T to, DurationS d, std::unique_ptr<AnimationCurve> curve,
                 std::unique_ptr<Interpolator<T>> interp) {
    animation_.reset(new FixedInterpAnimation<T>(
        d, to, get_fn_, set_fn_, std::move(curve), std::move(interp)));
    anim_controller_->AddListener(animation_.get());
  }

  void StopAnimation() { animation_.reset(); }

  bool IsAnimating() const { return animation_ && !animation_->HasFinished(); }

  bool GetTarget(T* out) {
    if (animation_) {
      *out = animation_->To();
      return true;
    }
    return false;
  }

 private:
  std::shared_ptr<AnimationController> anim_controller_;
  const std::function<T()> get_fn_;
  const std::function<void(T)> set_fn_;

  std::unique_ptr<FixedInterpAnimation<T>> animation_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_ANIMATED_FN_H_
