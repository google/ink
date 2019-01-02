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

#ifndef INK_ENGINE_UTIL_STRONG_TYPEDEF_H_
#define INK_ENGINE_UTIL_STRONG_TYPEDEF_H_

#include <string>
#include <type_traits>

#include "ink/engine/util/dbg/str.h"

namespace ink {
namespace util {

// A "strong typedef" around the underlying type, incompatible with other strong
// typedefs with the same underlying type. You can create a strong typedef with
// a normal typedef or a using declaration, like so:
//    typedef StrongTypedef<double, class MyDoubleType_Tag> MyDoubleType;
// or
//    using MyDoubleType = StrongTypedef<double, class MyDoubleType_Tag>;
// The template parameter UniqueIdTag ("MyDoubleType_Tag" in the examples
// above) must be unique class that is only declared w.r.t. the StrongTypedef,
// and does not need to be defined.
template <typename UnderlyingType, typename UniqueIdTag>
class StrongTypedef {
 public:
  using underlying_type = UnderlyingType;

  constexpr StrongTypedef() : value_() {}
  constexpr StrongTypedef(UnderlyingType value) : value_(value) {}
  StrongTypedef &operator=(UnderlyingType value) {
    value_ = value;
    return *this;
  }

  // These explicit conversion operators allow us to access the underlying value
  // via static_cast or brace initialization.

  // This allows us to cast to UnderlyingType.
  explicit operator const UnderlyingType() const { return value_; }

  // This allows us to cast to another fundamental type, provided that
  // UnderlyingType can be implicitly converted to said fundamental type.
  template <typename T, typename = typename std::enable_if<
                            std::is_fundamental<T>::value>::type>
  explicit operator const T() const {
    return value_;
  }

  std::string ToString() const { return Str(value_); }

  friend const StrongTypedef operator+(const StrongTypedef &lhs,
                                       const StrongTypedef &rhs) {
    return StrongTypedef(lhs.value_ + rhs.value_);
  }
  friend const StrongTypedef operator-(const StrongTypedef &lhs,
                                       const StrongTypedef &rhs) {
    return StrongTypedef(lhs.value_ - rhs.value_);
  }
  friend const StrongTypedef operator*(const StrongTypedef &lhs,
                                       const StrongTypedef &rhs) {
    return StrongTypedef(lhs.value_ * rhs.value_);
  }
  friend const StrongTypedef operator/(const StrongTypedef &lhs,
                                       const StrongTypedef &rhs) {
    return StrongTypedef(lhs.value_ / rhs.value_);
  }
  friend StrongTypedef &operator+=(StrongTypedef &lhs,
                                   const StrongTypedef &rhs) {
    lhs.value_ += rhs.value_;
    return lhs;
  }
  friend StrongTypedef &operator-=(StrongTypedef &lhs,
                                   const StrongTypedef &rhs) {
    lhs.value_ -= rhs.value_;
    return lhs;
  }
  friend StrongTypedef &operator*=(StrongTypedef &lhs,
                                   const StrongTypedef &rhs) {
    lhs.value_ *= rhs.value_;
    return lhs;
  }
  friend StrongTypedef &operator/=(StrongTypedef &lhs,
                                   const StrongTypedef &rhs) {
    lhs.value_ /= rhs.value_;
    return lhs;
  }
  friend bool operator==(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ == rhs.value_;
  }
  friend bool operator!=(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ != rhs.value_;
  }
  friend bool operator<(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ < rhs.value_;
  }
  friend bool operator>(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ > rhs.value_;
  }
  friend bool operator<=(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ <= rhs.value_;
  }
  friend bool operator>=(const StrongTypedef &lhs, const StrongTypedef &rhs) {
    return lhs.value_ >= rhs.value_;
  }

 private:
  UnderlyingType value_;
};

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_STRONG_TYPEDEF_H_
