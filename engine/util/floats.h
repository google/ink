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

#ifndef INK_ENGINE_UTIL_FLOATS_H_
#define INK_ENGINE_UTIL_FLOATS_H_

#include <cfloat>
#include <cmath>
#include <limits>

namespace ink {
namespace util {
namespace floats {

// Decomposes v into a*(2^b), where a is in [0,1] and b is an integer. Returns
// b. T is expected to be float, double, or possibly long double. All others
// should fail.
template <typename T>
inline int Exponent(T v) {
  int exp;
  std::frexp(v, &exp);
  return exp;
}

// Gets the maximum number of digits that can be stored in the mantissa for type
// T. This method returns the number of digits according to FLT_RADIX (which is
// 2 on most platforms).
// T is expected to be either float or double. An implementation for long double
// may be added in the future if deemed necessary.
template <typename T>
int MaxMantissaDigits() {
  return std::numeric_limits<T>::digits;
}

// Returns the largest exponent that may be stored in type T.
// T is expected to be either float or double. An implementation for long double
// may be added in the future if deemed necessary.
template <typename T>
int MaxExponent() {
  return std::numeric_limits<T>::max_exponent;
}

// Returns the smallest possible (negative) exponent that may be stored in type
// T. T is expected to be either float or double. An implementation for long
// double may be added in the future if deemed necessary.
template <typename T>
int MinExponent() {
  return std::numeric_limits<T>::min_exponent;
}

// These two functions return the previous and next representable floating-point
// values, respectively (see std::nextafter).
inline float PreviousFloat(float f) {
  return std::nextafter(f, std::numeric_limits<float>::lowest());
}
inline float NextFloat(float f) {
  return std::nextafter(f, std::numeric_limits<float>::max());
}

namespace floats_internal {
static constexpr int kZeroTolIncrementablePower = 12;  // 2^12 = 4096
static constexpr int kSafeMaxIncrementPower = 0;       // 2^0 = 1
static_assert(std::numeric_limits<float>::radix == 2, "Float radix is not 2");
}  // namespace floats_internal

// The tolerance value used for checking if a float is zero. We use the largest
// representable value that can be added to
// 2 ^ floats_internal::kZeroTolIncrementablePower without changing it.
static constexpr float kFloatZeroTol =
    1.0f / (2 << (std::numeric_limits<float>::digits -
                  floats_internal::kZeroTolIncrementablePower - 1));

// The tolerance value used for checking if a float is "too big". We use the
// smallest representable value that can be incremented by
// 2 ^ floats_internal::kSafeMaxIncrementPower
static constexpr float kFloatSafeMax =
    (2 << (std::numeric_limits<float>::digits +
           floats_internal::kSafeMaxIncrementPower - 1)) -
    1;

#ifdef __FAST_MATH__
#error "Sketchology cannot be compiled with -ffast-math. See go/ink-fast-math"
#endif

}  // namespace floats
}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_FLOATS_H_
