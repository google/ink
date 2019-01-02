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

// Classes for applying ink::proto::mutation::Mutations to an
// ink::proto::Snapshot.
// See go/ink-streaming-mutations for an introduction and rationale.
#ifndef INK_PUBLIC_MUTATIONS_MUTATION_APPLIER_H_
#define INK_PUBLIC_MUTATIONS_MUTATION_APPLIER_H_

#include <memory>

#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/status_or.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/mutations_portable_proto.pb.h"
#include "ink/public/document/document.h"
#include "ink/public/document/single_user_document.h"
#include "ink/public/document/storage/in_memory_storage.h"

namespace ink {

/**
 * Information about how the resulting Snapshot came to be made from the
 * mutations supplied to a MutationApplier's Apply() functions over the lifetime
 * of the MutationApplier.
 *
 * SUCCESS indicates that all given mutations were applied.
 *
 * SUCCESS_WITH_MERGE_CONFLICT_RESOLVED means that not all given mutations
 * were applicable, but the dropped ones are presumed to be due to losing a race
 * to the server. Everything was applied as best it could be.
 *
 * BAD_MUTATION means that the given data for some mutation was corrupt, or that
 * there was no way to make sense of it.
 *
 * INTERNAL_ERROR is an unrecoverable error indicating an Ink implementation
 * bug.
 *
 * The order of these enums is not arbitrary; a value that's greater than
 * another value represents a worse, less recoverable error condition.
 * Therefore, as a MutationApplier applies mutations, its overall
 * MutationApplicationStatus is always std::max(status_so_far, newest_status).
 */
enum class MutationApplicationStatus {
  SUCCESS,
  SUCCESS_WITH_MERGE_CONFLICT_RESOLVED,  // Any unapplied or partially-applied
                                         // mutations.
  BAD_MUTATION,                          // Malformed input.
  INTERNAL_ERROR                         // Programming error in this class.
};

template <>
std::string Str(const MutationApplicationStatus& status);

/** A struct containing the result of an application of mutations. */
class MutationResult {
 public:
  MutationResult(const proto::Snapshot& snapshot,
                 const MutationApplicationStatus& status)
      : snapshot_(snapshot), status_(status) {}

  const proto::Snapshot& Snapshot() const { return snapshot_; }
  const MutationApplicationStatus& Status() const { return status_; }

  static void WriteToProto(proto::mutations::MutationResult* proto,
                           const MutationResult& obj);

 private:
  proto::Snapshot snapshot_;
  MutationApplicationStatus status_;
};

/*
 * go/ink-streaming-mutations
 *
 * A MutationApplier applies ink::proto::mutation::Mutations to an
 * ink::proto::Snapshot.
 *
 * After optionally applying any number of Mutation messages to a
 * MutationApplier, you may ask it to set the current scene to the result of
 * those mutations, via LoadEngine(), or you may retrieve a MutationResult via
 * the Result() function.
 *
 * A MutationResult contains a Snapshot reflecting all of the applied mutations,
 * and a MutationApplicationStatus, which is documented here. The overall
 * MutationApplicationStatus reflects the most severe non-OK condition
 * encountered while applying mutations during the lifetime of this
 * MutationApplier.
 *
 * As each mutation is applied, the MutationApplier keeps track of a status
 * reflecting the union of mutations applied so far in its lifetime. That
 * status, a MutationApplicationStatus enum, is returned as part of a
 * MutationResult. You may retrieve a MutationResult at any time, and continue
 * to apply mutations. So, if all given mutations were successfully applied, the
 * status will be SUCCESS. If any mutations were interpreted as a merge (e.g.,
 * an attempt to remove an element that is not present in the existing
 * Snapshot), the overall status will be SUCCESS_WITH_MERGE_CONFLICT_RESOLVED,
 * etc.
 */
class MutationApplier {
 public:
  // Creates an empty MutationApplier.
  MutationApplier();

  // Creates a MutationApplier, and initializes it from the given Snapshot. If
  // FromSnapshot() returns ok(), the given out param will be populated with a
  // properly initialized MutationApplier; otherwise, the out param will be
  // nullptr.
  static ink::StatusOr<std::unique_ptr<MutationApplier>> FromSnapshot(
      const proto::Snapshot& snapshot);

  // Creates a MutationApplier, and initializes it from the given Document.
  explicit MutationApplier(std::unique_ptr<Document> doc)
      : doc_(std::move(doc)),
        running_status_(MutationApplicationStatus::SUCCESS) {}

  // Returns the result of all mutations applied since the construction of this
  // MutationApplier.
  MutationResult Result() const;

  // Returns the running status of all mutations applied so far.
  MutationApplicationStatus RunningStatus() const;

  // Loads the scene represented by the current state of this MutationApplier
  // into the given engine. Templated to avoid OpenGL dependencies.
  template <typename T>
  Status LoadEngine(T* engine) const {
    engine->SetDocument(doc_);
    return OkStatus();
  }

  // Applies the given Mutation proto. Returns ok() if the given mutations could
  // all be applied, or did not apply for a benign reason. Returns an error
  // otherwise.
  Status Apply(const proto::mutations::Mutation& mutation);

  // MutationApplier is neither copyable nor movable.
  MutationApplier(const MutationApplier&) = delete;
  MutationApplier& operator=(const MutationApplier&) = delete;

 private:
  inline static bool IsNeverMergeStatus(const Status& status) { return false; }

  // Applies the given Chunk. Returns ok() if the given mutation could
  // be applied, or did not apply for a benign reason. Returns an error
  // otherwise.
  Status Apply(const proto::mutations::Mutation::Chunk& chunk);

  // Considers the given Status returned by a Document operation. If it is ok(),
  // returns OkStatus(). If it matches the is_merge_status predicate, updates
  // the running_status to indicate merge conflict resolved, and returns
  // OkStatus(). If it IsInvalid, updates the running_status_ to indicate at
  // least BAD_MUTATION. If it is any other non-ok() Status, indicates
  // INTERNAL_ERROR.
  Status ProcessDocumentStatus(
      const Status& status,
      StatusPredicate is_merge_status = &IsNeverMergeStatus);

  // Shared pointer in order to permit loading the engine from this state
  // without losing that state.
  std::shared_ptr<Document> doc_;
  MutationApplicationStatus running_status_;

  friend class MutationApplierTest_GoldenPath_Test;
  friend class MutationApplierTest_TransformMissingElementIsHarmless_Test;
  friend class MutationApplierTest_VisibilityMissingElementIsHarmless_Test;
  friend class MutationApplierTest_AddMergeIsReorder_Test;
  friend class MutationApplierTest_RemoveMergeIsNoop_Test;
  friend class MutationApplierTest_CreateFromSnapshot_Test;
};

}  // namespace ink

#endif  // INK_PUBLIC_MUTATIONS_MUTATION_APPLIER_H_
