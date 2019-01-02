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

#ifndef INK_ENGINE_UTIL_ANIMATION_FIXED_INTERP_ANIMATION_H_
#define INK_ENGINE_UTIL_ANIMATION_FIXED_INTERP_ANIMATION_H_

// A FixedInterpAnimation is an Animation with fixed duration and target, where
// the initial start value and start time are filled in the first time they run.

#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/animation/animation_curve.h"
#include "ink/engine/util/animation/interpolator.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

template <typename T>
class FixedInterpAnimation : public Animation {
 public:
  FixedInterpAnimation(DurationS duration, T to,
                       std::function<T(void)> get_value,
                       std::function<void(const T&)> set_value)
      : FixedInterpAnimation(duration, to, get_value, set_value,
                             DefaultAnimationCurve(),
                             DefaultInterpolator<T>()) {}

  FixedInterpAnimation(DurationS duration, T to,
                       std::function<T(void)> get_value,
                       std::function<void(const T&)> set_value,
                       std::unique_ptr<AnimationCurve> curve,
                       std::unique_ptr<Interpolator<T>> interp)
      : has_been_evaluated_(false),
        duration_(duration),
        get_value_(get_value),
        set_value_(set_value),
        to_(to),
        curve_(std::move(curve)),
        interp_(std::move(interp)) {
    ASSERT(duration > DurationS(0));
    ASSERT(curve_->Apply(0) == 0);
    ASSERT(curve_->Apply(1) == 1);
  }

  void UpdateImpl(FrameTimeS time) override {
    ASSERT(set_value_);
    MaybeInitialize(time);
    double linear_progress = ProgressAtTime(time);
    double progress = curve_->Apply(linear_progress);
    set_value_(interp_->Interpolate(from_, to_, progress));
  }

  const T& To() const { return to_; }

  bool HasFinished() const override {
    return has_been_evaluated_ && last_update_time_ >= start_time_ + duration_;
  }

  float CurrentProgress() const {
    if (!has_been_evaluated_) return 0;
    return ProgressAtTime(last_update_time_);
  }

  Interpolator<T>* GetInterpolator() const { return interp_.get(); }

  AnimationCurve* GetAnimationCurve() const { return curve_.get(); }

 private:
  double ProgressAtTime(FrameTimeS time) const {
    return util::Clamp01(
        util::Normalize(static_cast<double>(start_time_),
                        static_cast<double>(start_time_ + duration_),
                        static_cast<double>(time)));
  }

  void MaybeInitialize(FrameTimeS time) {
    if (!has_been_evaluated_) {
      ASSERT(get_value_);
      start_time_ = time;
      from_ = get_value_();
      has_been_evaluated_ = true;
    }
  }

  bool has_been_evaluated_;
  FrameTimeS start_time_;
  const DurationS duration_;
  const std::function<T(void)> get_value_;
  const std::function<void(T)> set_value_;
  mutable T from_;
  const T to_;
  const std::unique_ptr<AnimationCurve> curve_;
  const std::unique_ptr<Interpolator<T>> interp_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_FIXED_INTERP_ANIMATION_H_
