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


#include "ink/public/mutations/mutation_packets.h"

#include <unordered_map>

#include "ink/engine/util/dbg/log.h"

using ink::proto::ElementBundle;
using ink::proto::MutationPacket;
using ink::proto::Snapshot;
using ink::proto::StorageAction;

namespace ink {

bool SnapshotHasPendingMutationPacket(const Snapshot& snapshot) {
  return snapshot.undo_action_size() > 0;
}

Status AddElementToMutationPacket(
    const std::unordered_map<UUID, const ElementBundle&>& element_table,
    MutationPacket* mutation_packet, UUID add_uuid) {
  const auto it = element_table.find(add_uuid);
  if (it == element_table.end()) {
    mutation_packet->Clear();
    return ErrorStatus(
        "undoable add with uuid $0 does not have matching element bundle",
        add_uuid);
  }
  *mutation_packet->add_element() = it->second;
  return OkStatus();
}

Status ExtractMutationPacket(const Snapshot& snapshot,
                             MutationPacket* mutation_packet) {
  mutation_packet->Clear();

  if (snapshot.has_page_properties()) {
    *mutation_packet->mutable_page_properties() = snapshot.page_properties();
  }

  if (!SnapshotHasPendingMutationPacket(snapshot)) {
    SLOG(SLOG_INFO, "no mutations");
    return OkStatus();
  }

  std::unordered_map<UUID, const ElementBundle&> element_table;
  for (const ElementBundle& element : snapshot.element()) {
    element_table.insert({element.uuid(), element});
  }
  // Some might be added then removed.
  for (const ElementBundle& element : snapshot.dead_element()) {
    element_table.insert({element.uuid(), element});
  }

  for (const StorageAction& action : snapshot.undo_action()) {
    *mutation_packet->add_mutation() = action;
    if (action.has_add_action()) {
      const auto& add = action.add_action();
      INK_RETURN_UNLESS(AddElementToMutationPacket(
          element_table, mutation_packet, add.uuid()));
    } else if (action.has_add_multiple_action()) {
      const auto& add = action.add_multiple_action();
      for (int i = 0; i < add.uuid_size(); ++i) {
        INK_RETURN_UNLESS(AddElementToMutationPacket(
            element_table, mutation_packet, add.uuid(i)));
      }
    }
  }
  return OkStatus();
}

Status ClearPendingMutationPacket(const Snapshot& source, Snapshot* target) {
  *target = source;
  target->clear_dead_element();
  target->clear_undo_action();
  target->clear_redo_action();
  target->clear_element_state_index();
  for (int i = 0; i < target->element_size(); i++) {
    target->add_element_state_index(proto::ElementState::ALIVE);
  }
  return OkStatus();
}
}  // namespace ink

