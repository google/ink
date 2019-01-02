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

#include "ink/engine/util/funcs/uuid_generator.h"

#include <sys/time.h>
#include <iomanip>
#include <limits>
#include <sstream>

#include "ink/engine/util/funcs/utils.h"

namespace ink {

UUIDGenerator::UUIDGenerator(uint64_t host_id)
    : clock_seq_(0), host_id_(host_id) {}

UUIDGenerator::~UUIDGenerator() {}

// The current UUID Time, defined as a 60 bit number of 100ns intervals since
// UTC October 15, 1582.
uint64_t UUIDGenerator::CurrentUUIDTime() const {
  // This is based on Page 27 of RFC4122 http://www.ietf.org/rfc/rfc4122.txt
  struct timeval tp;
  gettimeofday(&tp, nullptr);

  // Offset between UUID formatted times and Unix formatted times. This
  // constant taken directly from the RFC.
  // UUID UTC base time is October 15, 1582, Unix base time is January 1, 1970.
  const uint64_t uuid_unix_offset = UINT64_C(0x01B21DD213814000);

  return static_cast<uint64_t>(tp.tv_sec * 10000000) +
         static_cast<uint64_t>(tp.tv_usec * 10) + uuid_unix_offset;
}

UUID UUIDGenerator::GenerateUUID() {
  const uint32_t u32max = std::numeric_limits<uint32_t>::max();
  const uint16_t u16max = std::numeric_limits<uint16_t>::max();

  // UUIDv1, algorithm detailed on page 11 of RFC4122
  const uint64_t timestamp = CurrentUUIDTime();
  // A 128 bit number represented as 32 hexidecimal characters and 4 hyphens in
  // the form 8-4-4-4-12.
  std::stringstream out;
  out.setf(std::ios::hex, std::ios::basefield);
  out.fill('0');

  // time_low
  out << std::setw(8) << static_cast<uint32_t>(timestamp & u32max);
  out << '-';

  // time_mid
  out << std::setw(4) << static_cast<uint16_t>((timestamp >> 32) & u16max);
  out << '-';

  // time_hi_and_version
  out << std::setw(4)
      << static_cast<uint16_t>(((timestamp >> 48) & 0x0FFF) | (1 << 12));
  out << '-';

  // clock_seq_low, clock_seq_hi_and_reserved
  // 14 bits from ClockSeq, upper 2 bits are defined as 01.
  out << std::setw(4) << static_cast<uint16_t>((clock_seq_ & 0x3FFF) | 0x8000);
  clock_seq_++;
  out << '-';

  // node (aka hostId)
  out << std::setw(12) << (host_id_ & 0xFFFFFFFFFFFF);
  return UUID(out.str());
}
}  // namespace ink
