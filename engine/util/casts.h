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

#ifndef INK_ENGINE_UTIL_CASTS_H_
#define INK_ENGINE_UTIL_CASTS_H_

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include "ink/engine/util/dbg/errors.h"

namespace ink {

// Converts from numeric type "From" to numeric type "To", where "To" must be a
// _smaller number_ type than From (E.g. From=int64_t, To=int32_t, see below for
// explanation), avoiding overflow (underflow) by replacing the answer with the
// largest (most negative) value that numbers of type "To" can take.
//
// Assumption: the range of legal values for "From" is larger than the range of
// legal values for "To".  Should assert fail at compile time otherwise.
//
// Correct Example:
// From=int64_t, To=int32_t.
// The range of int64_t is [-2^63, 2^63 -1]
// The range of int32_t is [-2^31, 2^31 -1]
// The former range contains the latter.
//
// SafeNumericCast<int64_t, int32_t>(2^45)   ->   2^31-1
// SafeNumericCast<int64_t, int32_t>(-2^45)  ->  -2^31
// SafeNumericCast<int64_t, int32_t>(17)     ->   17
//
// SafeNumericCast<double, int32_t>(17.3)    ->   17
// SafeNumericCast<double, int32_t>(Inf)     ->   2^31-1
//
// Note: There is a known bug in this for:
// SafeNumericCast<float, int32_t>(Inf)     ->   -2^31
// Due to 2^31-1 not being exactly representable in float.
//
// NaNs:
//
// SafeNumericCast<double, int32_t>(Nan)          0
// SafeNumericCast<double, float>(Nan)            Nan
template <typename From, typename To>
To SafeNumericCast(From x) {
  static_assert(
      std::numeric_limits<From>::max() > std::numeric_limits<To>::max(),
      "Range of From must exceed range of To");
  static_assert(
      std::numeric_limits<From>::lowest() < std::numeric_limits<To>::lowest(),
      "Range of From must exceed range of To");
  if (std::numeric_limits<From>::has_quiet_NaN && std::isnan(x)) {
    if (std::numeric_limits<To>::has_quiet_NaN) {
      return std::numeric_limits<To>::quiet_NaN();
    } else {
      return 0;
    }
  }
  return static_cast<To>(
      std::max<From>(std::min<From>(std::numeric_limits<To>::max(), x),
                     std::numeric_limits<To>::lowest()));
}

}  // namespace ink

#endif  // INK_ENGINE_UTIL_CASTS_H_
