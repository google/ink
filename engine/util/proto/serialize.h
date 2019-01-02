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

#ifndef INK_ENGINE_UTIL_PROTO_SERIALIZE_H_
#define INK_ENGINE_UTIL_PROTO_SERIALIZE_H_

#include <limits>
#include <string>

#include "third_party/absl/strings/escaping.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/strings/substitute.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/security.h"

namespace ink {
namespace util {

// Interprets the given string as a base64-encoded serialized proto
// of the type belonging to the output parameter.
template <typename P>
ABSL_MUST_USE_RESULT Status Base64ToProto(absl::string_view escaped_proto,
                                          P* proto) {
  ::string wire_format;
  if (!absl::Base64Unescape(escaped_proto, &wire_format)) {
    return status::InvalidArgument("could not Base64Unescape encoded proto");
  }
  // ::string and std::string are different types under different environments.
  // It's easy enough just to copy here. When absl moves to std::string, this
  // can be eliminated.
  std::string std_wire = wire_format;
  if (!proto->ParseFromString(std_wire)) {
    return status::InvalidArgument("could not parse proto");
  }
  return OkStatus();
}

// Returns a base64-encoded serialized proto.
template <typename P>
std::string ProtoToBase64(const P& proto) {
  auto serialized_transform = proto.SerializeAsString();
  ::string temp_dest;
  absl::Base64Escape(serialized_transform, &temp_dest);
  // ::string and std::string are different types under different environments.
  // It's easy enough just to copy here. When absl moves to std::string, this
  // can be eliminated.
  std::string dest = temp_dest;
  return dest;
}

// Generic helpers to go to/from protos for classes that support it
template <typename TProto, typename TObj>
void WriteToProto(TProto* proto, const TObj& obj) {
  TObj::WriteToProto(proto, obj);
}

template <typename TProto, typename TObj>
ABSL_MUST_USE_RESULT Status ReadFromProto(const TProto& proto, TObj* obj) {
  return TObj::ReadFromProto(proto, obj);
}

template <typename TObj, typename TProto>
TObj ReadFromProtoOrDie(const TProto& proto) {
  TObj res;
  auto status = ReadFromProto(proto, &res);
  if (!status.ok()) {
    RUNTIME_ERROR("$0", status.error_message());
  }
  return res;
}

inline size_t ProtoSizeToSizeT(int proto_size) {
  EXPECT(proto_size >= 0);
  return static_cast<size_t>(proto_size);
}

// std::vector<glm::vec2> into RepeatedPtrField
// e.g. WriteToProto(mutable_point_field(), outline);
template <typename TProto>
void WriteToProto(TProto* proto, const std::vector<glm::vec2>& pts) {
  for (auto pt : pts) {
    auto new_pt = proto->Add();
    new_pt->set_x(pt.x);
    new_pt->set_y(pt.y);
  }
}

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_UTIL_PROTO_SERIALIZE_H_
