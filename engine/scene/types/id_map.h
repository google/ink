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

#ifndef INK_ENGINE_SCENE_TYPES_ID_MAP_H_
#define INK_ENGINE_SCENE_TYPES_ID_MAP_H_

#include <unordered_map>

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

// A bidirectional map between element ids and uuids.
class IdMap {
 public:
  bool Contains(const UUID& uuid) const;
  bool Contains(const ElementId& el_id) const;
  void Insert(const UUID& uuid, const ElementId& el_id);
  ElementId GetElementId(const UUID& uuid) const;
  UUID GetUUID(const ElementId& el_id) const;
  void Remove(const ElementId& el_id);
  void Remove(const UUID& uuid);

  // Make IdMap iterable with a for range loop.
  std::unordered_map<ElementId, UUID, ElementIdHasher>::const_iterator begin()
      const {
    return element_to_uuid_.begin();
  }
  std::unordered_map<ElementId, UUID, ElementIdHasher>::const_iterator end()
      const {
    return element_to_uuid_.end();
  }

 private:
  void Remove(UUID uuid, ElementId el_id);
  std::unordered_map<UUID, ElementId> uuid_to_element_;
  std::unordered_map<ElementId, UUID, ElementIdHasher> element_to_uuid_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_ID_MAP_H_
