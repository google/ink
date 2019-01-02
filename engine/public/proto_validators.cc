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

#include "ink/engine/public/proto_validators.h"

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

bool ValidateProto(const proto::ElementBundle& unsafe_bundle) {
  if (!unsafe_bundle.has_uuid()) {
    SLOG(SLOG_ERROR, "missing uuid");
    return false;
  }
  if (!is_valid_uuid(unsafe_bundle.uuid())) {
    SLOG(SLOG_ERROR, "invalid uuid");
    return false;
  }
  if (!unsafe_bundle.has_transform()) {
    SLOG(SLOG_ERROR, "missing transform");
    return false;
  }

  return true;
}

bool ValidateProtoForAdd(const proto::ElementBundle& unsafe_bundle) {
  if (!ValidateProto(unsafe_bundle)) {
    return false;
  }
  if (!unsafe_bundle.has_element()) {
    SLOG(SLOG_ERROR, "missing element");
    return false;
  }
  return true;
}

template <typename MutationsType>
bool ValidateMutationsProto(const MutationsType& unsafe_mutations) {
  auto size = unsafe_mutations.mutation_size();
  if (size <= 0) {
    SLOG(SLOG_ERROR, "Got empty ElementMutations proto.");
    return false;
  }

  for (int i = 0; i < size; i++) {
    const auto& mutation = unsafe_mutations.mutation(i);
    if (!is_valid_uuid(mutation.uuid())) {
      SLOG(SLOG_ERROR, "Invalid uuid, $1, in mutation at index $0", i,
           mutation.uuid());
      return false;
    }
  }
  return true;
}

bool ValidateProto(const proto::ElementTransformMutations& unsafe_mutations) {
  return ValidateMutationsProto(unsafe_mutations);
}

bool ValidateProto(const proto::ElementVisibilityMutations& unsafe_mutations) {
  return ValidateMutationsProto(unsafe_mutations);
}

bool ValidateProto(const proto::ElementOpacityMutations& unsafe_mutations) {
  if (!ValidateMutationsProto(unsafe_mutations)) return false;

  for (int i = 0; i < unsafe_mutations.mutation_size(); ++i) {
    const auto& mutation = unsafe_mutations.mutation(i);
    auto opacity = mutation.opacity();
    if (opacity < 0 || opacity > 255) {
      SLOG(SLOG_ERROR, "Invalid opacity, $0, should be 0 <= opacity <= 255.",
           opacity);
      return false;
    }
  }
  return true;
}

bool ValidateProto(const proto::ElementZOrderMutations& unsafe_mutations) {
  if (!ValidateMutationsProto(unsafe_mutations)) return false;

  for (int i = 0; i < unsafe_mutations.mutation_size(); ++i) {
    const auto& mutation = unsafe_mutations.mutation(i);
    const UUID& below_uuid = mutation.below_uuid();
    if (below_uuid != kInvalidUUID && !is_valid_uuid(below_uuid)) {
      SLOG(SLOG_ERROR, "Invalid value at index $1 for below_uuid ($0).",
           mutation.below_uuid(), i);
      return false;
    }
  }
  return true;
}

}  //  namespace ink
