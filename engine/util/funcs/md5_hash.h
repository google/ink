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

#ifndef INK_ENGINE_UTIL_FUNCS_MD5_HASH_H_
#define INK_ENGINE_UTIL_FUNCS_MD5_HASH_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "ink/third_party/md5/md5.h"

namespace ink {

class MD5Hash {
 public:
  MD5Hash() {}

  void Add(const std::string& str);
  // Adds the floating point numbers rounding to .001
  // Note it also might lose high order precision for values > MAX_FLOAT / 1000
  void AddApprox(const float* values, size_t num_values);
  void AddBytes(const void* data, size_t num_bytes);
  std::pair<uint64_t, uint64_t> Hash128() const;
  // A 64 bit hash (a xor of the upper and lower halves of the 128 bit hash).
  uint64_t Hash64() const;

 private:
  MD5 md5_;
};

}  // namespace ink
#endif  // INK_ENGINE_UTIL_FUNCS_MD5_HASH_H_
