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

#ifndef INK_ENGINE_UTIL_SIGNAL_FILTERS_EXP_MOVING_AVG_H_
#define INK_ENGINE_UTIL_SIGNAL_FILTERS_EXP_MOVING_AVG_H_

namespace ink {
namespace signal_filters {

// An infinite impulse response filter implemented as an exponential moving
// average. The time between samples is assumed to be constant.
template <typename ValueType, typename ScalarType>
class ExpMovingAvg {
 public:
  ExpMovingAvg() : ExpMovingAvg(ValueType(), 0.9) {}
  ExpMovingAvg(ValueType initial, ScalarType smoothing_factor)
      : value_(initial),
        smoothing_factor_(smoothing_factor),
        last_sample_(initial) {}

  void Sample(ValueType sample) {
    value_ = smoothing_factor_ * value_ + (1 - smoothing_factor_) * sample;
    last_sample_ = sample;
  }

  ValueType Value() const { return value_; }
  ValueType LastSample() const { return last_sample_; }

 private:
  ValueType value_;
  ScalarType smoothing_factor_;
  ValueType last_sample_;
};

}  // namespace signal_filters
}  // namespace ink

#endif  // INK_ENGINE_UTIL_SIGNAL_FILTERS_EXP_MOVING_AVG_H_
