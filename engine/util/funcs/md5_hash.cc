// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/util/funcs/md5_hash.h"

#include <cmath>  // std::round
#include <cstdint>
#include <string>
#include <utility>  // std::pair

namespace ink {

// Unfortunately there is no htonll in netinet/in.h, and the version of boost in
// third_party is out of date and does not have endian conversion functions, so
// we have to write this ourselves here.
static uint64_t InterpretBytesAsBigEndianUint64(const uint8_t* d) {
  uint64_t v = 0;
  for (size_t i = 0; i < sizeof(uint64_t); ++i) {
    v <<= 8;
    v += *(d + i);
  }
  return v;
}

void MD5Hash::Add(const std::string& str) {
  AddBytes(str.c_str(), str.length());
}

void MD5Hash::AddApprox(const float* values, size_t num_values) {
  for (size_t i = 0; i < num_values; ++i) {
    int32_t approx_value = static_cast<int32_t>(std::round(values[i] * 1000));
    AddBytes(&approx_value, sizeof(approx_value));
  }
}

void MD5Hash::AddBytes(const void* data, size_t num_bytes) {
  md5_.update(data, num_bytes);
}

std::pair<uint64_t, uint64_t> MD5Hash::Hash128() const {
  MD5 copy = md5_;

  // d is a pointer into a 128 bit digest.
  const uint8_t* d = copy.finalize();

  // Endianness of the bytes returned by MD5_final is big endian but we are
  // guaranteed to be running on little endian, convert it.
  uint64_t high = InterpretBytesAsBigEndianUint64(d);
  uint64_t low = InterpretBytesAsBigEndianUint64(d + sizeof(uint64_t));
  return std::make_pair(high, low);
}

uint64_t MD5Hash::Hash64() const {
  auto h128 = Hash128();
  return h128.first ^ h128.second;
}
}  // namespace ink
