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

#include "ink/engine/scene/types/element_bundle.h"

#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {

using status::InvalidArgument;

ElementBundle::ElementBundle() : uuid_(kInvalidUUID) {}

Status ElementBundle::ReadFromProto(
    const ink::proto::ElementBundle& unsafe_proto_bundle,
    ElementBundle* result) {
  *result = ElementBundle();
  if (!unsafe_proto_bundle.has_element() ||
      !unsafe_proto_bundle.has_transform()) {
    return InvalidArgument("tried to read proto that was missing fields");
  }
  result->element_ = unsafe_proto_bundle.element();
  result->transform_ = unsafe_proto_bundle.transform();
  UUID uuid = unsafe_proto_bundle.uuid();
  if (!ink::is_valid_uuid(uuid)) {
    return InvalidArgument("uuid cannot be read or is invalid.");
  }
  result->uuid_ = uuid;
  return OkStatus();
}

// static
void ElementBundle::WriteToProto(
    ink::proto::ElementBundle* write_to, const UUID& from_uuid,
    const ink::proto::Element& from_element,
    const ink::proto::AffineTransform& from_transform) {
  write_to->set_uuid(from_uuid);
  *write_to->mutable_element() = from_element;
  *write_to->mutable_transform() = from_transform;
}

// static
Status ElementBundle::ReadObjectMatrix(const proto::ElementBundle& proto,
                                       glm::mat4* mat) {
  ASSERT(mat);
  *mat = glm::mat4{1};
  if (!proto.has_transform()) {
    return InvalidArgument("given Bundle has no transform");
  }
  return util::ReadFromProto(proto.transform(), mat);
}

const UUID& ElementBundle::safe_uuid() const { return uuid_; }
const ink::proto::Element& ElementBundle::unsafe_element() const {
  return element_;
}
const ink::proto::AffineTransform& ElementBundle::unsafe_transform() const {
  return transform_;
}

}  // namespace ink
