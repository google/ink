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

#include "ink/public/brix/brix.h"

#include <cmath>
#include <cstdint>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {

using proto::AffineTransform;
using proto::BrixElementBundle;
using proto::BrixElementMutation;
using proto::ElementBundle;
using proto::ElementTransformMutations;
using util::Base64ToProto;
using util::ProtoToBase64;

namespace brix {

bool ElementBundleToBrixElementBundle(const ElementBundle& in,
                                      BrixElementBundle* out) {
  out->Clear();
  if (!in.has_uuid()) {
    SLOG(SLOG_ERROR, "missing uuid");
    return false;
  }
  if (!in.has_element()) {
    SLOG(SLOG_ERROR, "missing element");
    return false;
  }
  if (!in.has_transform()) {
    SLOG(SLOG_ERROR, "missing transform");
    return false;
  }
  out->set_encoded_element(ProtoToBase64(in.element()));
  out->set_encoded_transform(ProtoToBase64(in.transform()));
  out->set_uuid(in.uuid());
  return true;
}

S_WARN_UNUSED_RESULT bool ElementMutationsToBrixElementMutation(
    const ink::proto::ElementTransformMutations& mutations,
    ink::proto::BrixElementMutation* out) {
  std::vector<UUID> uuids;
  std::vector<proto::AffineTransform> transforms;
  for (int i = 0; i < mutations.mutation_size(); ++i) {
    const auto& mutation = mutations.mutation(i);
    uuids.emplace_back(mutation.uuid());
    transforms.emplace_back(mutation.transform());
  }
  return ElementTransformsToBrixElementMutation(uuids, transforms, out);
}

bool ElementTransformsToBrixElementMutation(
    const std::vector<UUID>& uuids,
    const std::vector<proto::AffineTransform>& transforms,
    BrixElementMutation* out) {
  out->Clear();
  if (uuids.size() != transforms.size()) {
    SLOG(SLOG_ERROR, "mismatched sizes");
    return false;
  }
  for (int i = 0; i < uuids.size(); i++) {
    *out->add_uuid() = uuids[i];
    *out->add_encoded_transform() = ProtoToBase64(transforms[i]);
  }
  return true;
}

bool BrixElementToElementBundle(const std::string& uuid,
                                const std::string& encoded_element,
                                const std::string& encoded_transform,
                                ElementBundle* out) {
  out->set_uuid(uuid);
  if (!Base64ToProto(encoded_element, out->mutable_element())) {
    SLOG(SLOG_ERROR, "bad encoded element");
    out->Clear();
    return false;
  }
  if (!Base64ToProto(encoded_transform, out->mutable_transform())) {
    SLOG(SLOG_ERROR, "bad encoded transform");
    out->Clear();
    return false;
  }
  return true;
}

bool AppendBrixElementMutation(const std::string& uuid,
                               const std::string& encoded_transform,
                               ink::proto::ElementTransformMutations* target) {
  AffineTransform tx;
  if (!Base64ToProto(encoded_transform, &tx)) {
    SLOG(SLOG_ERROR, "bad encoded transform");
    return false;
  }

  auto* mutation = target->add_mutation();
  mutation->set_uuid(uuid);
  *mutation->mutable_transform() = tx;
  return true;
}

}  // namespace brix
}  // namespace ink
