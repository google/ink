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

#ifndef INK_ENGINE_SCENE_TYPES_ELEMENT_BUNDLE_H_
#define INK_ENGINE_SCENE_TYPES_ELEMENT_BUNDLE_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/engine/util/security.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/geometry_portable_proto.pb.h"

namespace ink {

// C++ version of proto::ElementBundle (see elements.proto)
class ElementBundle {
 public:
  ElementBundle();
  const UUID& safe_uuid() const;
  const proto::Element& unsafe_element() const;
  const proto::AffineTransform& unsafe_transform() const;

  static ABSL_MUST_USE_RESULT Status ReadFromProto(
      const proto::ElementBundle& unsafe_proto_bundle, ElementBundle* result);

  static void WriteToProto(proto::ElementBundle* write_to,
                           const UUID& from_uuid,
                           const proto::Element& from_element,
                           const proto::AffineTransform& from_transform);

  static ABSL_MUST_USE_RESULT Status
  ReadObjectMatrix(const proto::ElementBundle& proto, glm::mat4* mat);

 private:
  UUID uuid_;
  proto::Element element_;
  proto::AffineTransform transform_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_TYPES_ELEMENT_BUNDLE_H_
