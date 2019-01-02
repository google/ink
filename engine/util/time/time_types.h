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

#ifndef INK_ENGINE_UTIL_TIME_TIME_TYPES_H_
#define INK_ENGINE_UTIL_TIME_TIME_TYPES_H_

#include <string>
#include <type_traits>

#include "ink/engine/util/strong_typedef.h"

namespace ink {

using DurationS = util::StrongTypedef<double, class TimeS_Tag>;

namespace time_type_internal {

// A time duration before or after a unique reference time (which is not
// necessarily known or defined). TimeTypes with different reference times are
// not directly compatible.
// Template parameter DurationType is assumed to be a StrongTypedef.
// Template parameter UniqueIdTag must be unique class that is only declared
// w.r.t. the TimeType, and does not need to be defined.
template <typename DurationType, typename UniqueIdTag>
class TimeType {
 public:
  using underlying_type = typename DurationType::underlying_type;

  constexpr TimeType() : value_() {}
  constexpr explicit TimeType(DurationType value) : value_(value) {}

  // These explicit conversion operators allow us to access the underlying value
  // via static_cast or brace initialization.

  // This allows us to cast to DurationType.
  explicit operator const DurationType() const { return value_; }

  // This allows us to cast to underlying_type.
  explicit operator const underlying_type() const {
    return static_cast<underlying_type>(value_);
  }

  // This allows us to cast to another fundamental type, provided that
  // underlying_type can be implicitly converted to said fundamental type.
  template <typename T, typename = typename std::enable_if<
                            std::is_fundamental<T>::value>::type>
  explicit operator const T() const {
    return static_cast<underlying_type>(value_);
  }

  std::string ToString() const { return Str(value_); }

  friend const DurationType operator-(const TimeType& lhs,
                                      const TimeType& rhs) {
    return lhs.value_ - rhs.value_;
  }
  friend const TimeType operator+(const TimeType& lhs,
                                  const DurationType& rhs) {
    return TimeType(lhs.value_ + rhs);
  }
  friend const TimeType operator+(const DurationType& lhs,
                                  const TimeType& rhs) {
    return TimeType(lhs + rhs.value_);
  }
  friend const TimeType operator-(const TimeType& lhs,
                                  const DurationType& rhs) {
    return TimeType(lhs.value_ - rhs);
  }
  friend const TimeType operator-(const DurationType& lhs,
                                  const TimeType& rhs) {
    return TimeType(lhs - rhs.value_);
  }
  friend TimeType& operator+=(TimeType& lhs, const DurationType& rhs) {
    lhs = lhs + rhs;
    return lhs;
  }
  friend TimeType& operator-=(TimeType& lhs, const DurationType& rhs) {
    lhs = lhs - rhs;
    return lhs;
  }
  friend bool operator==(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ == rhs.value_;
  }
  friend bool operator!=(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ != rhs.value_;
  }
  friend bool operator<(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ < rhs.value_;
  }
  friend bool operator>(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ > rhs.value_;
  }
  friend bool operator<=(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ <= rhs.value_;
  }
  friend bool operator>=(const TimeType& lhs, const TimeType& rhs) {
    return lhs.value_ >= rhs.value_;
  }

 private:
  DurationType value_;
};

}  // namespace time_type_internal

using FrameTimeS =
    time_type_internal::TimeType<DurationS, class FrameTimeS_Tag>;
using InputTimeS =
    time_type_internal::TimeType<DurationS, class InputTimeS_Tag>;
using WallTimeS = time_type_internal::TimeType<DurationS, class WallTimeS_Tag>;

}  // namespace ink

#endif  // INK_ENGINE_UTIL_TIME_TIME_TYPES_H_
