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

#ifndef INK_ENGINE_UTIL_FUNCS_UTILS_H_
#define INK_ENGINE_UTIL_FUNCS_UTILS_H_

#include <cmath>
#include <cstdint>

#include "third_party/absl/base/config.h"  // x-platform endian checks

namespace ink {
namespace util {

// Mathematical mod operation.  If i >= 0, same as i % m.  E.g.
// mod(-2, 7) ===> 5
// mod(-10,7) ===> 4
//
// Behavior is undefined if m < 0
template <typename T, typename S>
inline T Mod(T i, S m) {
  if (m == 0) {
    return 0;
  }
  if (i < 0) {
    i += std::ceil((-i) / m + 1) * m;
  }
  return std::fmod(i, m);
}

inline bool IsPowerOf2(uint32_t x) { return x != 0 && ((x - 1) & x) == 0; }

namespace little_endian {
#if ABSL_IS_BIG_ENDIAN
inline uint16_t FromHost16(uint16_t x) { return absl::gbswap_16(x); }
inline uint16_t ToHost16(uint16_t x) { return absl::gbswap_16(x); }
#else
inline uint16_t FromHost16(uint16_t x) { return x; }
inline uint16_t ToHost16(uint16_t x) { return x; }
#endif  // ABSL_IS_BIG_ENDIAN

}  // namespace little_endian

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_FUNCS_UTILS_H_
