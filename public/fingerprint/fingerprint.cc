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

#include "ink/public/fingerprint/fingerprint.h"

#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/util/proto/serialize.h"

using ink::proto::ElementBundle;
using ink::proto::Snapshot;

namespace ink {

uint64_t GetFingerprint(const Snapshot& snapshot) {
  Fingerprinter fingerprinter;
  for (const ElementBundle& b : snapshot.element()) {
    fingerprinter.Note(b);
  }
  return fingerprinter.GetFingerprint();
}

Fingerprinter::Fingerprinter() {}
Fingerprinter::~Fingerprinter() {}

void Fingerprinter::Note(const ElementBundle& element) {
  Note(element.uuid(), element.transform());
}

void Fingerprinter::Note(const std::string& uuid,
                         const ink::proto::AffineTransform& transform) {
  glm::mat4 obj_to_world{1};
  if (!ink::util::ReadFromProto(transform, &obj_to_world)) {
    SLOG(SLOG_WARNING, "invalid transform for element $0", uuid);
    return;
  }
  Note(uuid, obj_to_world);
}

void Fingerprinter::Note(const std::string& uuid,
                         const glm::mat4& obj_to_world) {
  hasher_.Add(uuid);
  hasher_.AddApprox(glm::value_ptr(obj_to_world), 16);
}

uint64_t Fingerprinter::GetFingerprint() { return hasher_.Hash64(); }

}  // namespace ink
