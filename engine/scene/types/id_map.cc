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

#include "ink/engine/scene/types/id_map.h"

#include <utility>
#include "ink/engine/util/dbg/errors.h"

namespace ink {

bool IdMap::Contains(const UUID& uuid) const {
  return uuid_to_element_.count(uuid) > 0;
}

bool IdMap::Contains(const ElementId& el_id) const {
  return element_to_uuid_.count(el_id) > 0;
}

void IdMap::Insert(const UUID& uuid, const ElementId& el_id) {
  uuid_to_element_.insert(std::pair<UUID, ElementId>(uuid, el_id));
  element_to_uuid_.insert(std::pair<ElementId, UUID>(el_id, uuid));
}

ElementId IdMap::GetElementId(const UUID& uuid) const {
  return uuid_to_element_.at(uuid);
}
UUID IdMap::GetUUID(const ElementId& el_id) const {
  return element_to_uuid_.at(el_id);
}

// Pass copies to avoid erasing refs into our maps while we're using them
void IdMap::Remove(UUID uuid, ElementId el_id) {
  ASSERT(Contains(el_id));
  ASSERT(Contains(uuid));
  element_to_uuid_.erase(el_id);
  uuid_to_element_.erase(uuid);
}

void IdMap::Remove(const ElementId& el_id) {
  ASSERT(Contains(el_id));
  Remove(element_to_uuid_.at(el_id), el_id);
}

void IdMap::Remove(const UUID& uuid) {
  ASSERT(Contains(uuid));
  Remove(uuid, uuid_to_element_.at(uuid));
}
}  // namespace ink
