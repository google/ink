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

// Functions for fingerprinting ink scenes.
#ifndef INK_PUBLIC_FINGERPRINT_FINGERPRINT_H_
#define INK_PUBLIC_FINGERPRINT_FINGERPRINT_H_

#include <cstdint>  // uint64_t
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/funcs/md5_hash.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// Returns the fingerprint for the scene contained in the given Snapshot.
uint64_t GetFingerprint(const ink::proto::Snapshot& snapshot);

class Fingerprinter {
 public:
  Fingerprinter();
  ~Fingerprinter();

  // Add the given element to the fingerprint.
  void Note(const ink::proto::ElementBundle& element);

  // Add the given element details to the fingerprint.
  void Note(const std::string& uuid,
            const ink::proto::AffineTransform& transform);

  // Add the given derived element details to the fingerprint.
  void Note(const std::string& uuid, const glm::mat4& obj_to_world);

  // Generate and return the fingerprint for all of the noted elements.
  uint64_t GetFingerprint();

 private:
  MD5Hash hasher_;
};

}  // namespace ink

#endif  // INK_PUBLIC_FINGERPRINT_FINGERPRINT_H_
