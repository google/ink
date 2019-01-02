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

#ifndef INK_ENGINE_UTIL_SIGNAL_FILTERS_TIME_VARIANT_MOVING_AVG_H_
#define INK_ENGINE_UTIL_SIGNAL_FILTERS_TIME_VARIANT_MOVING_AVG_H_

#include <queue>

#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace signal_filters {

// A finite impulse response filter implemented as moving average of the samples
// within within some amount of time. Sample times are assumed to be
// non-decreasing.
template <typename T, typename TimeType>
class TimeVariantMovingAvg {
 public:
  TimeVariantMovingAvg()
      : TimeVariantMovingAvg(T(), TimeType(0), DurationS(1)) {}
  TimeVariantMovingAvg(T initial_value, TimeType initial_time,
                       DurationS timeout)
      : sum_(initial_value), timeout_(timeout) {
    samples_.push({initial_value, initial_time});
  }

  void Sample(T sample, TimeType time) {
    samples_.push({sample, time});
    sum_ += sample;
    while (!samples_.empty() && time - samples_.front().time > timeout_) {
      sum_ -= samples_.front().value;
      samples_.pop();
    }
  }

  T Value() const { return sum_ / static_cast<T>(samples_.size()); }
  T LastSample() const { return samples_.back().value; }
  double LastTime() const { return samples_.back().time; }

 private:
  struct SampleData {
    T value;
    TimeType time;
  };

  T sum_;
  DurationS timeout_;
  std::queue<SampleData> samples_;
};

}  // namespace signal_filters
}  // namespace ink

#endif  // INK_ENGINE_UTIL_SIGNAL_FILTERS_TIME_VARIANT_MOVING_AVG_H_
