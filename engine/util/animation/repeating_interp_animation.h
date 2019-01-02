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

#ifndef INK_ENGINE_UTIL_ANIMATION_REPEATING_INTERP_ANIMATION_H_
#define INK_ENGINE_UTIL_ANIMATION_REPEATING_INTERP_ANIMATION_H_

// A RepeatingInterpAnimation is an Animation with fixed duration and two end
// points, where it interpolates from one end to the other over Duration time,
// then back the other way, back and forth indefinitely.

#include <cmath>

#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/animation/animation_curve.h"
#include "ink/engine/util/animation/interpolator.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

template <typename T>
class RepeatingInterpAnimation : public Animation {
 public:
  RepeatingInterpAnimation(DurationS duration, T from, T to,
                           std::function<void(const T&)> set_value)
      : RepeatingInterpAnimation(duration, from, to, set_value,
                                 DefaultAnimationCurve(),
                                 DefaultInterpolator<T>()) {}
  RepeatingInterpAnimation(DurationS duration, T from, T to,
                           std::function<void(const T&)> set_value,
                           std::unique_ptr<AnimationCurve> curve,
                           std::unique_ptr<Interpolator<T>> interp)
      : has_been_evaluated_(false),
        duration_(duration),
        set_value_(set_value),
        from_(from),
        to_(to),
        curve_(std::move(curve)),
        interp_(std::move(interp)) {
    ASSERT(duration > DurationS(0));
    ASSERT(set_value);
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

  Interpolator<T>* GetInterpolator() const { return interp_.get(); }

  AnimationCurve* GetAnimationCurve() const { return curve_.get(); }

 private:
  double ProgressAtTime(FrameTimeS time) const {
    // p will be the number of cycles we have progress from the start, eg it
    // will be 0.5 if we are half way through the first cycle, and 7.5 if we are
    // half way through the 8th cycle.
    double p = util::Normalize(static_cast<double>(start_time_),
                               static_cast<double>(start_time_ + duration_),
                               static_cast<double>(time));

    double intpart;
    double fraction = std::modf(p, &intpart);
    int cycle_number = static_cast<int>(intpart);

    // Every other cycle we want to animate from 1->0 instead of 0->1.
    if (cycle_number % 2 == 1) {
      return 1 - fraction;
    }
    return fraction;
  }

  void MaybeInitialize(FrameTimeS time) {
    if (!has_been_evaluated_) {
      start_time_ = time;
      has_been_evaluated_ = true;
    }
  }

  bool has_been_evaluated_;
  FrameTimeS start_time_;
  const DurationS duration_;
  const std::function<void(T)> set_value_;
  const T from_;
  const T to_;
  const std::unique_ptr<AnimationCurve> curve_;
  const std::unique_ptr<Interpolator<T>> interp_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_REPEATING_INTERP_ANIMATION_H_
