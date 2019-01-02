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

#ifndef INK_ENGINE_SCENE_TYPES_ELEMENT_ATTRIBUTES_H_
#define INK_ENGINE_SCENE_TYPES_ELEMENT_ATTRIBUTES_H_

#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

enum GroupType { kLayerGroupType, kUnknownGroupType };

inline proto::ElementAttributes::GroupType GroupTypeToProto(
    GroupType group_type) {
  switch (group_type) {
    case kLayerGroupType:
      return proto::ElementAttributes::LAYER;
    case kUnknownGroupType:
      return proto::ElementAttributes::UNKNOWN;
  }
}

inline GroupType GroupTypeFromProto(
    proto::ElementAttributes::GroupType proto_group_type) {
  switch (proto_group_type) {
    case proto::ElementAttributes::LAYER:
      return kLayerGroupType;
    case proto::ElementAttributes::UNKNOWN:
      return kUnknownGroupType;
  }
}

// Holder for values of the ElementAttributes proto.
struct alignas(8) ElementAttributes {
  bool selectable = true;
  bool magic_erasable = true;
  bool is_sticker = false;
  bool is_text = false;
  GroupType group_type = kUnknownGroupType;

  // A zoomable element should be rendered by ZoomableRectRenderer.
  bool is_zoomable = false;

  bool operator==(const ElementAttributes& rhs) const {
    // Compare each of this object's attributes with those of rhs.
    return std::tie(selectable, magic_erasable, is_sticker, is_text, group_type,
                    is_zoomable) == std::tie(rhs.selectable, rhs.magic_erasable,
                                             rhs.is_sticker, is_text,
                                             group_type, is_zoomable);
  }

  static void WriteToProto(proto::ElementAttributes* attributes_proto,
                           const ElementAttributes& attributes) {
    SLOG(SLOG_DATA_FLOW, "Writing attributes selectable:$0 erasable:$1",
         attributes.selectable, attributes.magic_erasable);
    attributes_proto->set_selectable(attributes.selectable);
    attributes_proto->set_magic_erasable(attributes.magic_erasable);
    attributes_proto->set_is_sticker(attributes.is_sticker);
    attributes_proto->set_is_text(attributes.is_text);
    attributes_proto->set_is_zoomable(attributes.is_zoomable);
    attributes_proto->set_group_type(GroupTypeToProto(attributes.group_type));
  }

  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const proto::ElementAttributes& unsafe_proto_bundle,
                ElementAttributes* result) {
    SLOG(SLOG_DATA_FLOW, "Reading element attributes selectable:$0 erasable:$1",
         unsafe_proto_bundle.selectable(),
         unsafe_proto_bundle.magic_erasable());
    result->selectable = unsafe_proto_bundle.selectable();
    result->magic_erasable = unsafe_proto_bundle.magic_erasable();
    result->is_sticker = unsafe_proto_bundle.is_sticker();
    result->is_text = unsafe_proto_bundle.is_text();
    result->is_zoomable = unsafe_proto_bundle.is_zoomable();
    result->group_type = GroupTypeFromProto(unsafe_proto_bundle.group_type());
    return OkStatus();
  }
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_ELEMENT_ATTRIBUTES_H_
