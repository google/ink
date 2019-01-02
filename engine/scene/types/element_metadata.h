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

#ifndef INK_ENGINE_SCENE_TYPES_ELEMENT_METADATA_H_
#define INK_ENGINE_SCENE_TYPES_ELEMENT_METADATA_H_

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/proto_traits.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

struct ColorModifier {
  // Premultiplied multiplicative component-wise modifier.
  glm::vec4 mul{0, 0, 0, 0};
  // Premultiplied additive component-wise modifier.
  glm::vec4 add{0, 0, 0, 0};

  ColorModifier()
      : ColorModifier(glm::vec4(1, 1, 1, 1), glm::vec4(0, 0, 0, 0)) {}

  ColorModifier(glm::vec4 mul, glm::vec4 add) : mul(mul), add(add) {}

  glm::vec4 Apply(glm::vec4 base_color) {
    return glm::fma(base_color, mul, add);
  }

  bool operator==(const ColorModifier& other) const {
    return other.mul == mul && other.add == add;
  }

  bool operator!=(const ColorModifier& other) const {
    return !operator==(other);
  }
};

// ElementMetadata is returned from the SceneGraph describing a snapshot of
// the element's state at the time SceneGraph::GetElementMetadata was called.
struct ElementMetadata {
  ElementId id;
  UUID uuid;
  glm::mat4 world_transform{1};           // Object to world.
  glm::mat4 group_transform{1};           // Object to group.
  glm::mat4 group_to_world_transform{1};  // Group to world.
  bool rendered_by_main;
  ElementAttributes attributes;
  ColorModifier color_modifier;
  GroupId group_id;
  bool visible;
  int opacity;

  ElementMetadata()
      : ElementMetadata(kInvalidElementId, kInvalidUUID, glm::mat4(1),
                        glm::mat4(1), glm::mat4(1), false, ElementAttributes(),
                        ColorModifier(), kInvalidElementId, true, 255) {}
  ElementMetadata(ElementId elid, UUID uuid, glm::mat4 world_transform,
                  glm::mat4 group_transform, glm::mat4 group_to_world_transform,
                  bool rendered_by_main, ElementAttributes attributes,
                  ColorModifier color_modifier, ElementId group_id,
                  bool visible, int opacity)
      : id(elid),
        uuid(uuid),
        world_transform(world_transform),
        group_transform(group_transform),
        group_to_world_transform(group_to_world_transform),
        rendered_by_main(rendered_by_main),
        attributes(attributes),
        color_modifier(color_modifier),
        group_id(group_id),
        visible(visible),
        opacity(opacity) {}

  bool operator==(const ElementMetadata& other) const {
    return other.id == id && other.uuid == uuid &&
           other.world_transform == world_transform &&
           other.group_transform == group_transform &&
           other.group_to_world_transform == group_to_world_transform &&
           other.rendered_by_main == rendered_by_main &&
           other.attributes == attributes &&
           other.color_modifier == color_modifier &&
           other.group_id == group_id && other.visible == visible &&
           other.opacity == opacity;
  }
};

enum class ElementMutationType {
  kNone,
  kTransformMutation,
  kColorMutation,
  kRenderedByMainMutation,
  kVisibilityMutation,
  kOpacityMutation,
  kZOrderMutation
};

struct ElementMutationData {
  ElementMutationType mutation_type;
  ElementMetadata original_element_data;
  ElementMetadata modified_element_data;
};

inline bool operator==(const ElementMutationData& lhs,
                       const ElementMutationData& rhs) {
  return lhs.mutation_type == rhs.mutation_type &&
         lhs.original_element_data == rhs.original_element_data &&
         lhs.modified_element_data == rhs.modified_element_data;
}

template <typename MutationsType>
inline void AppendElementMutation(
    const std::string& uuid,
    const typename ProtoTraits<MutationsType>::ValueType& value,
    MutationsType* mutations) {
  auto* mutation = mutations->add_mutation();
  mutation->set_uuid(uuid);
  ProtoTraits<MutationsType>::SetValue(mutation, value);
}

inline void AppendElementTransform(
    const std::string& uuid, const proto::AffineTransform& tx,
    proto::ElementTransformMutations* mutations) {
  AppendElementMutation(uuid, tx, mutations);
}

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_ELEMENT_METADATA_H_
