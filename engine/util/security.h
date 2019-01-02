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

#ifndef INK_ENGINE_UTIL_SECURITY_H_
#define INK_ENGINE_UTIL_SECURITY_H_

#include <cstddef>  // size_t

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

// Sketchology's WARN_UNUSED_RESULT -- other third_party_packages also
// define this macro.
#define S_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

// Discared a result without checking it
#define S_IGNORE_RESULT(expr) \
  do {                        \
    (void)(expr);             \
  } while (0)

///////////////////////////////////////////////////////////////////////////////
// IncEx
// Checks of the form [a, b)
///////////////////////////////////////////////////////////////////////////////

template <typename LowerBoundsType, typename UpperBoundsType,
          typename ValueType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncEx(ValueType value, LowerBoundsType inclusive_lower_bound,
                 UpperBoundsType exclusive_upper_bound) {
  if (value >= inclusive_lower_bound && value < exclusive_upper_bound)
    return OkStatus();
  return ErrorStatus(StatusCode::FAILED_PRECONDITION, "$0 not in [$1, $2)",
                     value, inclusive_lower_bound, exclusive_upper_bound);
}

template <typename LowerBoundsType, typename UpperBoundsType, typename Vec>
Status S_WARN_UNUSED_RESULT
VecBoundsCheckIncEx(Vec value, LowerBoundsType inclusive_lower_bound,
                    UpperBoundsType exclusive_upper_bound) {
  for (size_t i = 0; i < value.length(); i++) {
    INK_RETURN_UNLESS(BoundsCheckIncEx(value[i], inclusive_lower_bound,
                                       exclusive_upper_bound));
  }
  return OkStatus();
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncEx(glm::vec2 vec, LowerBoundsType inclusive_lower_bound,
                 UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncEx(vec, inclusive_lower_bound, exclusive_upper_bound);
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncEx(glm::vec3 vec, LowerBoundsType inclusive_lower_bound,
                 UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncEx(vec, inclusive_lower_bound, exclusive_upper_bound);
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncEx(glm::vec4 vec, LowerBoundsType inclusive_lower_bound,
                 UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncEx(vec, inclusive_lower_bound, exclusive_upper_bound);
}

// checks of the form (a, b)
template <typename LowerBoundsType, typename UpperBoundsType,
          typename ValueType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncInc(ValueType value, LowerBoundsType inclusive_lower_bound,
                  UpperBoundsType inclusive_upper_bound) {
  if (value >= inclusive_lower_bound && value <= inclusive_upper_bound)
    return OkStatus();
  return ErrorStatus(StatusCode::FAILED_PRECONDITION, "$0 not in [$1, $2]",
                     value, inclusive_lower_bound, inclusive_upper_bound);
}

///////////////////////////////////////////////////////////////////////////////
// IncInc
// Checks of the form [a, b]
///////////////////////////////////////////////////////////////////////////////

template <typename LowerBoundsType, typename UpperBoundsType, typename Vec>
Status S_WARN_UNUSED_RESULT
VecBoundsCheckIncInc(Vec value, LowerBoundsType inclusive_lower_bound,
                     UpperBoundsType inclusive_upper_bound) {
  for (size_t i = 0; i < value.length(); i++) {
    INK_RETURN_UNLESS(BoundsCheckIncInc(value[i], inclusive_lower_bound,
                                        inclusive_upper_bound));
  }
  return OkStatus();
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncInc(glm::vec2 vec, LowerBoundsType inclusive_lower_bound,
                  UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncInc(vec, inclusive_lower_bound,
                              exclusive_upper_bound);
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncInc(glm::vec3 vec, LowerBoundsType inclusive_lower_bound,
                  UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncInc(vec, inclusive_lower_bound,
                              exclusive_upper_bound);
}

template <typename LowerBoundsType, typename UpperBoundsType>
Status S_WARN_UNUSED_RESULT
BoundsCheckIncInc(glm::vec4 vec, LowerBoundsType inclusive_lower_bound,
                  UpperBoundsType exclusive_upper_bound) {
  return VecBoundsCheckIncInc(vec, inclusive_lower_bound,
                              exclusive_upper_bound);
}

///////////////////////////////////////////////////////////////////////////////
// ExInc
// checks of the form (a, b]
///////////////////////////////////////////////////////////////////////////////

template <typename LowerBoundsType, typename UpperBoundsType,
          typename ValueType>
Status S_WARN_UNUSED_RESULT
BoundsCheckExInc(ValueType value, LowerBoundsType exclusive_lower_bound,
                 UpperBoundsType inclusive_upper_bound) {
  if (value > exclusive_lower_bound && value <= inclusive_upper_bound)
    return OkStatus();
  return ErrorStatus(StatusCode::FAILED_PRECONDITION, "$0 not in ($1, $2]",
                     value, exclusive_lower_bound, inclusive_upper_bound);
}

///////////////////////////////////////////////////////////////////////////////
// ExEx
// checks of the form (a, b)
///////////////////////////////////////////////////////////////////////////////

template <typename LowerBoundsType, typename UpperBoundsType,
          typename ValueType>
Status S_WARN_UNUSED_RESULT
BoundsCheckExEx(ValueType value, LowerBoundsType exclusive_lower_bound,
                UpperBoundsType exclusive_upper_bound) {
  if (value > exclusive_lower_bound && value < exclusive_upper_bound)
    return OkStatus();
  return ErrorStatus(StatusCode::FAILED_PRECONDITION, "$0 not in ($1, $2)",
                     value, exclusive_lower_bound, exclusive_upper_bound);
}

// Checks that a+b does not overflow.  Note: this is not particularly
// performant.
template <typename T>
bool AddOverflowsSigned(T a, T b) {
  static_assert(std::numeric_limits<T>::is_signed, "Use only for signed types");
  return ((b > 0) && (a > std::numeric_limits<T>::max() - b)) ||
         ((b < 0) && (a < std::numeric_limits<T>::lowest() - b));
}

// Checks that a+b does not overflow.  Note: this is not particularly
// performant.
template <typename T>
bool AddOverflowsUnsigned(T a, T b) {
  static_assert(!std::numeric_limits<T>::is_signed,
                "Use only for unsigned types");
  return (b > 0) && (a > std::numeric_limits<T>::max() - b);
}

}  // namespace ink
#endif  // INK_ENGINE_UTIL_SECURITY_H_
