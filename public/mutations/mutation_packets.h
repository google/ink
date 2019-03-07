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

// Functions for building MutationPackets and for clearing unsynced changes from
// Snapshots.

#ifndef INK_PUBLIC_MUTATIONS_MUTATION_PACKETS_H_
#define INK_PUBLIC_MUTATIONS_MUTATION_PACKETS_H_

#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/security.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// Returns true if the given snapshot has any pending mutations.
S_WARN_UNUSED_RESULT bool SnapshotHasPendingMutationPacket(
    const ink::proto::Snapshot& snapshot);

// Extracts pending mutations from the given Snapshot, and returns true unless
// the given Snapshot is in an unrecoverable inconsistent state. If the given
// Snapshot has no pending mutations, you get an empty MutationPacket.
S_WARN_UNUSED_RESULT Status
ExtractMutationPacket(const ink::proto::Snapshot& snapshot,
                      ink::proto::MutationPacket* mutation_packet);

// Clears the undo/redo stack of the passed Snapshot.
void ClearPendingMutationPacket(ink::proto::Snapshot* snapshot);

}  // namespace ink

#endif  // INK_PUBLIC_MUTATIONS_MUTATION_PACKETS_H_
