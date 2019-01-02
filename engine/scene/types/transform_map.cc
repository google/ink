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

#include "ink/engine/scene/types/transform_map.h"

#include <utility>

#include "third_party/absl/base/optimization.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

TransformMap::TransformMap() {
  glm::mat4 identity{1};  // being explicit.
  // Make an "invalid" group. This represents the root.
  // It should never change. Touch the various data maps
  // to ensure that it looks like a group.
  obj_to_group_[kInvalidElementId] = identity;
  obj_to_world_[kInvalidElementId] = identity;
  world_to_obj_[kInvalidElementId] = identity;
  group_to_ids_[kInvalidElementId];
  group_generation_[kInvalidElementId] = 0;
  last_group_generation_[kInvalidElementId] = 0;
}

void TransformMap::MaybeRecompute(ElementId id) const {
  auto group = GetGroup(id);
  // This is called on every drawing frame for each element to
  // check if it's in the view. We generally shouldn't have to recompute since
  // most elements are static.
  ASSERT(last_group_generation_.count(id) > 0);
  ASSERT(group_generation_.count(group) > 0);
  if (ABSL_PREDICT_TRUE(last_group_generation_[id] ==
                        group_generation_[group])) {
    return;
  }
  ASSERT(obj_to_group_.count(id) > 0);
  auto& local_xform = obj_to_group_.find(id)->second;
  obj_to_world_[id] = ObjToWorld(group) * local_xform;
  world_to_obj_[id] = glm::inverse(obj_to_world_[id]);
  last_group_generation_[id] = group_generation_[group];
}

const glm::mat4& TransformMap::ObjToWorld(ElementId id) const {
  MaybeRecompute(id);
  ASSERT(obj_to_world_.count(id) > 0);
  return obj_to_world_.find(id)->second;
}

const glm::mat4& TransformMap::ObjToGroup(ElementId id) const {
  ASSERT(obj_to_group_.count(id) > 0);
  return obj_to_group_.find(id)->second;
}

const glm::mat4& TransformMap::WorldToObj(ElementId id) const {
  MaybeRecompute(id);
  ASSERT(world_to_obj_.count(id) > 0);
  return world_to_obj_.find(id)->second;
}

void TransformMap::Set(ElementId id, glm::mat4 obj_to_group) {
  Set(id, GetGroup(id), obj_to_group);
}

void TransformMap::Set(ElementId id, GroupId group, glm::mat4 obj_to_group) {
  ASSERT(id != kInvalidElementId);

  obj_to_group_[id] = obj_to_group;

  if (id.Type() == GROUP) {
    // Defers updating children until necessary. Over optimized? Just update
    // all children? That defeats the purpose of group translations...
    group_generation_[id]++;
    // Touch group_to_ids_ such that it exists.
    group_to_ids_[id];
  }

  // Note: root = kInvalidElementId
  // non-GROUPs can group to GROUPs or the root.
  // GROUPs can only group to the root.
  ASSERT(group == kInvalidElementId || group.Type() == GROUP);
  // Ensure that the group is defined.
  ASSERT(Contains(group) > 0);

  // Handle potential regrouping.
  auto old_group = id_to_group_.find(id);
  if (old_group != id_to_group_.end() && old_group->second != group) {
    // We had an old group. Erase this id from its old group's list.
    group_to_ids_[id_to_group_[id]].erase(id);
  }
  // Set the new group information.
  id_to_group_[id] = group;
  group_to_ids_[group].insert(id);

  // Generate the new ObjToWorld immediately.
  obj_to_world_[id] = ObjToWorld(group) * obj_to_group_[id];
  world_to_obj_[id] = glm::inverse(obj_to_world_[id]);

  // Store the generation we have of the group. We've already stored the
  // current value of the transform so we don't need to recalculate it until
  // the group changes.
  last_group_generation_[id] = group_generation_[group];
}

GroupId TransformMap::GetGroup(ElementId id) const {
  auto it = id_to_group_.find(id);
  if (it == id_to_group_.end()) {
    return kInvalidElementId;
  }
  return it->second;
}

void TransformMap::Remove(ElementId id) {
  world_to_obj_.erase(id);
  obj_to_world_.erase(id);
  obj_to_group_.erase(id);
  if (id_to_group_.count(id) > 0) {
    group_to_ids_[id_to_group_[id]].erase(id);
  }
  id_to_group_.erase(id);
  if (id.Type() == GROUP) {
    // There better be no elements that depend on us
    // as a group...
    ASSERT(group_to_ids_[id].empty());
    group_to_ids_.erase(id);
  }
  last_group_generation_.erase(id);
}

bool TransformMap::Contains(ElementId id) const {
  bool res = obj_to_world_.count(id) > 0;
  // The inverse and the obj to group should always be present.
  ASSERT((world_to_obj_.count(id) > 0) == res);
  ASSERT((obj_to_group_.count(id) > 0) == res);
  return res;
}

const absl::flat_hash_set<ElementId, ElementIdHasher>&
TransformMap::GetElementsForGroup(GroupId group) const {
  ASSERT(group_to_ids_.count(group) > 0);
  return group_to_ids_.find(group)->second;
}

}  // namespace ink
