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

#ifndef INK_ENGINE_SCENE_TYPES_TRANSFORM_MAP_H_
#define INK_ENGINE_SCENE_TYPES_TRANSFORM_MAP_H_

#include <cstdint>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/flat_hash_set.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

// A map between element ids and their transforms and groups. Used
// by the SceneGraph to keep track of where elements are.
//
// There are three transforms stored in this map:
// ObjToWorld - Transforms points in an object into their world coordinates.
// WorldToObj - Given points in world coordinate space, the transform to obj
//              coordinates.
// ObjToGroup - Transforms an object into its group's space. A group is
//              a GROUP element or kInvalidElementId (the root). When
//              associating an element to a group, the group must already
//              be present in this transform map. The root element,
//              kInvalidElementId is special and is added explicitly when
//              constructing this object.
//
// This class is NOT thread safe.
class TransformMap {
 public:
  TransformMap();

  // The object to group transform. All elements are at least
  // a child of the root group (signaled by group == kInvalidElementId),
  // which has an identity transform. This means that, when groups
  // aren't being used, ObjToGroup == ObjToWorld.
  const glm::mat4& ObjToGroup(ElementId id) const;

  const glm::mat4& ObjToWorld(ElementId id) const;
  const glm::mat4& WorldToObj(ElementId id) const;
  // Returns if we have a transform associated with this element or group.
  bool Contains(ElementId id) const;

  // Set the element's object to group transform, keeping its current group.
  // Just a short hand for Set(id, transform_map.GetGroup(id), obj_to_group);
  void Set(ElementId id, glm::mat4 obj_to_group);
  // Set the element's object to group transform, setting its
  // group to the group passed in. This will also update the element's
  // world-relative position based on its [potentially new] group and new obj
  // to group transform. This call is an UPSERT, if the element does not exist,
  // it will be created. If it already exists, it will be updated.
  // POLY elements may have a GROUP element group or specify the group as
  // the root (kInvalidElementId).
  // GROUP elements may only have a group set to kInvalidElementId.
  void Set(ElementId id, GroupId group, glm::mat4 obj_to_group);
  // Remove an element entirely. For groups, all elements associated with
  // the group must already have been removed (enforced by assertion).
  void Remove(ElementId id);
  // Return the group for an element. May return kInvalidElementId (root
  // group) if the element is not contained by this map.
  GroupId GetGroup(ElementId id) const;
  // Return all elements and groups that are direct children of the passed in
  // group. Pass in kInvalidElementId to get all the root elements.
  const absl::flat_hash_set<ElementId, ElementIdHasher>& GetElementsForGroup(
      GroupId group) const;

 private:
  // Potentially recompute the objtoworld transform for the given element.
  // The const-ness is a lie. It will work on the mutable attributes below.
  // Called by ObjToWorld and WorldToObj.
  void MaybeRecompute(ElementId id) const;

  // Map of group group -> latest generation id. Used to invalidate
  // the obj to world map.
  mutable GroupIdHashMap<uint64_t> group_generation_;
  // Map of the element id -> group generation stored. Used to invalidate
  // the obj to world map.
  mutable ElementIdHashMap<uint64_t> last_group_generation_;
  // object to world transform. This may be computed on the fly if
  // the group generation changed for the group of a given element.
  // mutable because we're treating this as a cache.
  mutable ElementIdHashMap<glm::mat4> obj_to_world_;
  // world to object trasnsform. This may be computed on the fly if
  // the group generation changed for the group of a given element.
  // mutable because we're treating this as a cache.
  mutable ElementIdHashMap<glm::mat4> world_to_obj_;

  // object to group transform. All elements have a group, though in
  // the "non-group" case, that group can be kInvalidElementId, which will
  // have an identity transform. In that case, objtogroup == objtoworld.
  ElementIdHashMap<glm::mat4> obj_to_group_;
  // element id to group group id. All elements have a group, though
  // that group may be kInvalidElementId to indicate the group is the root.
  ElementIdHashMap<GroupId> id_to_group_;
  // This lets us quickly find out the set of elements for a given group.
  GroupIdHashMap<absl::flat_hash_set<ElementId, ElementIdHasher>> group_to_ids_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_TRANSFORM_MAP_H_
