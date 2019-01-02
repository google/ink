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

#ifndef INK_ENGINE_UTIL_TIME_TIMER_H_
#define INK_ENGINE_UTIL_TIME_TIMER_H_

#include <memory>
#include <utility>

#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

class Timer {
 public:
  Timer(std::shared_ptr<WallClockInterface> clock, DurationS target_interval)
      : clock_(std::move(clock)),
        target_interval_(target_interval),
        start_time_(clock_->CurrentTime()) {}

  inline bool Expired() const { return TimeRemaining() < 0; }
  inline DurationS TimeRemaining() const {
    return start_time_ + TargetInterval() - clock_->CurrentTime();
  }
  inline DurationS TargetInterval() const { return target_interval_; }

  inline void Reset() { start_time_ = clock_->CurrentTime(); }
  inline void Reset(DurationS target_interval) {
    target_interval_ = target_interval;
    Reset();
  }

 private:
  std::shared_ptr<WallClockInterface> clock_;
  DurationS target_interval_;
  WallTimeS start_time_;
};

}  // namespace ink
#endif  // INK_ENGINE_UTIL_TIME_TIMER_H_
