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

#ifndef INK_ENGINE_UTIL_FUNCS_UUID_GENERATOR_H_
#define INK_ENGINE_UTIL_FUNCS_UUID_GENERATOR_H_

#include <cstdint>

#include "ink/engine/public/types/uuid.h"

namespace ink {

class UUIDGenerator {
 public:
  // hostUid is a 48 bit identifier, e.g. Mac address.
  explicit UUIDGenerator(uint64_t host_uid);

  // Disallow copy and assign.
  UUIDGenerator(const UUIDGenerator&) = delete;
  UUIDGenerator& operator=(const UUIDGenerator&) = delete;

  virtual ~UUIDGenerator();

  // Provides a UUID String. This is a string of hexidecimal digits and dashes
  // that represents a 128 bit number that is generally considered globally
  // unique.
  UUID GenerateUUID();

 protected:
  // These fields protected for testing.
  virtual uint64_t CurrentUUIDTime() const;
  uint16_t clock_seq_;

 private:
  uint64_t host_id_;
};

}  // namespace ink
#endif  // INK_ENGINE_UTIL_FUNCS_UUID_GENERATOR_H_
