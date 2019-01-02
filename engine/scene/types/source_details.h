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

#ifndef INK_ENGINE_SCENE_TYPES_SOURCE_DETAILS_H_
#define INK_ENGINE_SCENE_TYPES_SOURCE_DETAILS_H_

#include <cstdint>
#include <string>

#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

struct CallbackFlags {
  bool do_callback = false;
  bool attach_compressed_mesh_data = false;
  bool attach_uncompressed_outline = false;
  bool attach_compressed_input_points = false;

 public:
  CallbackFlags() {}

  static CallbackFlags IdOnly() {
    CallbackFlags ans;
    ans.do_callback = true;
    return ans;
  }
  static CallbackFlags IdAndFullStroke() {
    CallbackFlags ans;
    ans.do_callback = true;
    ans.attach_compressed_mesh_data = true;
    ans.attach_compressed_input_points = true;
    return ans;
  }
  static CallbackFlags All() {
    CallbackFlags ans;
    ans.do_callback = true;
    ans.attach_compressed_mesh_data = true;
    ans.attach_compressed_input_points = true;
    ans.attach_uncompressed_outline = true;
    return ans;
  }
  static CallbackFlags NoCallback() { return CallbackFlags(); }

  static ABSL_MUST_USE_RESULT Status ReadFromProto(
      const proto::CallbackFlags& proto, CallbackFlags* callback_flags);

  bool operator==(const CallbackFlags& other) const {
    return do_callback == other.do_callback &&
           attach_compressed_mesh_data == other.attach_compressed_mesh_data &&
           attach_uncompressed_outline == other.attach_uncompressed_outline &&
           attach_compressed_input_points ==
               other.attach_compressed_input_points;
  }
};

///////////////////////////////////////////////////////////

struct SourceDetails {
  // EngineInternal -- no callbacks ever generated, no fields are set
  // Engine -- callbacks conditionally generated
  // Host -- callbacks conditionally generated, hostSourceDetails is set
  //
  // For conditionally generated callbacks, the mapping between any given
  // SourceDetails and the CallbackFlags is defined by ElementNotifier
  enum class Origin { EngineInternal, Engine, Host };

  Origin origin;
  uint32_t host_source_details;

 public:
  SourceDetails();
  static SourceDetails FromEngine();
  static SourceDetails FromHost(uint32_t host_data);
  static SourceDetails EngineInternal();
  static ABSL_MUST_USE_RESULT Status ReadFromProto(
      const proto::SourceDetails& proto, SourceDetails* source_details);
  static void WriteToProto(proto::SourceDetails* proto,
                           const SourceDetails& source_details);

  std::string ToString() const;
  bool operator==(const SourceDetails& other) const;
};

template <>
inline std::string Str(const SourceDetails::Origin& origin) {
  switch (origin) {
    case SourceDetails::Origin::EngineInternal:
      return "EngineInternal";
    case SourceDetails::Origin::Engine:
      return "Engine";
    case SourceDetails::Origin::Host:
      return "Host";
    default:
      ASSERT(false);
      return "unknown origin enum";
  }
}

namespace util {

template <>
ABSL_MUST_USE_RESULT Status ReadFromProto(
    const proto::SourceDetails::Origin& proto, SourceDetails::Origin* origin);

template <>
void WriteToProto(proto::SourceDetails::Origin* proto,
                  const SourceDetails::Origin& origin);

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_SCENE_TYPES_SOURCE_DETAILS_H_
