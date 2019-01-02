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

#include "ink/engine/scene/types/source_details.h"

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

// static
Status CallbackFlags::ReadFromProto(const ink::proto::CallbackFlags& proto,
                                    CallbackFlags* callback_flags) {
  callback_flags->attach_compressed_mesh_data = proto.compressed_mesh_data();
  callback_flags->attach_uncompressed_outline = proto.uncompressed_outline();
  callback_flags->attach_compressed_input_points =
      proto.compressed_input_points();
  callback_flags->do_callback = callback_flags->attach_compressed_mesh_data ||
                                callback_flags->attach_uncompressed_outline ||
                                callback_flags->attach_compressed_input_points;
  return OkStatus();
}

SourceDetails::SourceDetails()
    : origin(Origin::Engine), host_source_details(0) {}

// static
SourceDetails SourceDetails::FromEngine() {
  SourceDetails res;
  res.origin = Origin::Engine;
  return res;
}

// static
SourceDetails SourceDetails::FromHost(uint32_t host_data) {
  SourceDetails res;
  res.origin = Origin::Host;
  res.host_source_details = host_data;
  return res;
}

// static
SourceDetails SourceDetails::EngineInternal() {
  SourceDetails res;
  res.origin = Origin::EngineInternal;
  return res;
}

// static
Status SourceDetails::ReadFromProto(const ink::proto::SourceDetails& proto,
                                    SourceDetails* source_details) {
  source_details->host_source_details = proto.host_source_details();
  return ink::util::ReadFromProto(proto.origin(), &(source_details->origin));
}

// static
void SourceDetails::WriteToProto(ink::proto::SourceDetails* proto,
                                 const SourceDetails& source_details) {
  ink::proto::SourceDetails::Origin origin;
  ink::util::WriteToProto(&origin, source_details.origin);
  proto->set_origin(origin);
  proto->set_host_source_details(source_details.host_source_details);
}

bool SourceDetails::operator==(const SourceDetails& other) const {
  return other.origin == origin &&
         other.host_source_details == host_source_details;
}

std::string SourceDetails::ToString() const {
  return Substitute("origin: $0, hostData: $1", origin, host_source_details);
}

namespace util {

template <>
Status ReadFromProto(const proto::SourceDetails::Origin& proto,
                     SourceDetails::Origin* origin) {
  switch (proto) {
    case proto::SourceDetails::ENGINE:
      *origin = SourceDetails::Origin::Engine;
      return OkStatus();
    case proto::SourceDetails::HOST:
      *origin = SourceDetails::Origin::Host;
      return OkStatus();
    default:
      *origin = SourceDetails::Origin::Host;
      return status::InvalidArgument(
          "Unrecognized source detail origin format: $0, using HOST.",
          static_cast<int>(proto));
  }
}

template <>
void WriteToProto(proto::SourceDetails::Origin* proto,
                  const SourceDetails::Origin& origin) {
  switch (origin) {
    case SourceDetails::Origin::Engine:
      *proto = proto::SourceDetails::ENGINE;
      break;
    case SourceDetails::Origin::Host:
      *proto = proto::SourceDetails::HOST;
      break;
    default:
      // Note: we should never send engineInternal messages back up, hence there
      // is no engineInternal type in the proto enum.
      SLOG(SLOG_ERROR, "Unrecognized or non-covertable source detail: $0.",
           static_cast<int>(origin));
      EXPECT(false);
  }
}

}  // namespace util
}  // namespace ink
