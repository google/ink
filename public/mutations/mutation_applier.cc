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

#include "ink/public/mutations/mutation_applier.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

template <>
std::string Str(const MutationApplicationStatus& status) {
  switch (status) {
    case MutationApplicationStatus::INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    case MutationApplicationStatus::BAD_MUTATION:
      return "BAD_MUTATION";
    case MutationApplicationStatus::SUCCESS_WITH_MERGE_CONFLICT_RESOLVED:
      return "SUCCESS_WITH_MERGE_CONFLICT_RESOLVED";
    case MutationApplicationStatus::SUCCESS:
      return "SUCCESS";
  }
}

/* static */ void MutationResult::WriteToProto(
    proto::mutations::MutationResult* proto, const MutationResult& obj) {
  switch (obj.status_) {
    case MutationApplicationStatus::SUCCESS:
      proto->set_status(proto::mutations::MutationResult::SUCCESS);
      break;
    case MutationApplicationStatus::SUCCESS_WITH_MERGE_CONFLICT_RESOLVED:
      proto->set_status(proto::mutations::MutationResult::
                            SUCCESS_WITH_MERGE_CONFLICT_RESOLVED);
      break;
    case MutationApplicationStatus::BAD_MUTATION:
      proto->set_status(proto::mutations::MutationResult::BAD_MUTATION);
      break;
    case MutationApplicationStatus::INTERNAL_ERROR:
      proto->set_status(proto::mutations::MutationResult::INTERNAL_ERROR);
      break;
  }
  *(proto->mutable_snapshot()) = obj.snapshot_;
}

MutationApplier::MutationApplier()
    : MutationApplier(absl::make_unique<SingleUserDocument>(
          std::make_shared<InMemoryStorage>())) {}

ink::StatusOr<std::unique_ptr<MutationApplier>> MutationApplier::FromSnapshot(
    const proto::Snapshot& snapshot) {
  std::unique_ptr<Document> doc;
  INK_RETURN_UNLESS(SingleUserDocument::CreateFromSnapshot(
      std::make_shared<InMemoryStorage>(), snapshot, &doc));
  return absl::make_unique<MutationApplier>(std::move(doc));
}

MutationResult MutationApplier::Result() const {
  return MutationResult(
      doc_->GetSnapshot(Document::SnapshotQuery::DO_NOT_INCLUDE_UNDO_STACK),
      running_status_);
}

MutationApplicationStatus MutationApplier::RunningStatus() const {
  return running_status_;
}

Status MutationApplier::Apply(const proto::mutations::Mutation& mutation) {
  if (mutation.chunk_size() == 0) {
    SLOG(SLOG_WARNING, "suspicious empty mutation");
  }
  for (const auto& chunk : mutation.chunk()) {
    INK_RETURN_UNLESS(Apply(chunk));
  }
  return OkStatus();
}

namespace {
// An attempted element removal can meet with a NOT_FOUND or an INCOMPLETE,
// either of which should be treated as a merge.
bool IsNotFoundOrIncomplete(const Status& status) {
  return status::IsNotFound(status) || status::IsIncomplete(status);
}
}  // namespace

Status MutationApplier::Apply(const proto::mutations::Mutation::Chunk& chunk) {
  if (chunk.has_set_grid()) {
    return ProcessDocumentStatus(doc_->SetGrid(chunk.set_grid().grid()));
  }
  if (chunk.has_set_border()) {
    return ProcessDocumentStatus(
        doc_->SetPageBorder(chunk.set_border().border()));
  }
  if (chunk.has_set_background_color()) {
    proto::BackgroundColor bc;
    bc.set_rgba(chunk.set_background_color().rgba_non_premultiplied());
    return ProcessDocumentStatus(doc_->SetBackgroundColor(bc));
  }
  if (chunk.has_set_world_bounds()) {
    return ProcessDocumentStatus(
        doc_->SetPageBounds(chunk.set_world_bounds().bounds()));
  }
  if (chunk.has_remove_element()) {
    return ProcessDocumentStatus(doc_->Remove({chunk.remove_element().uuid()}),
                                 &IsNotFoundOrIncomplete);
  }
  if (chunk.has_add_element()) {
    const proto::mutations::AddElement& element = chunk.add_element();
    return ProcessDocumentStatus(
        doc_->AddBelow(element.element(), element.below_element_with_uuid()),
        &status::IsAlreadyExists);
  }
  if (chunk.has_set_element_transform()) {
    const proto::mutations::SetElementTransform& set_element_transform =
        chunk.set_element_transform();
    proto::ElementTransformMutations mus;
    AppendElementMutation(set_element_transform.uuid(),
                          set_element_transform.transform(), &mus);
    return ProcessDocumentStatus(doc_->ApplyMutations(mus),
                                 &status::IsNotFound);
  }
  if (chunk.has_set_visibility()) {
    const auto& set_visibility = chunk.set_visibility();
    proto::ElementVisibilityMutations mus;
    AppendElementMutation(set_visibility.uuid(), set_visibility.visibility(),
                          &mus);
    return ProcessDocumentStatus(doc_->ApplyMutations(mus),
                                 &status::IsNotFound);
  }
  if (chunk.has_set_opacity()) {
    const auto& set_opacity = chunk.set_opacity();
    proto::ElementOpacityMutations mus;
    AppendElementMutation(set_opacity.uuid(), set_opacity.opacity(), &mus);
    return ProcessDocumentStatus(doc_->ApplyMutations(mus),
                                 &status::IsNotFound);
  }
  if (chunk.has_change_z_order()) {
    const auto& change_z_order = chunk.change_z_order();
    proto::ElementZOrderMutations mus;
    AppendElementMutation(change_z_order.uuid(), change_z_order.below_uuid(),
                          &mus);
    return ProcessDocumentStatus(doc_->ApplyMutations(mus),
                                 &status::IsNotFound);
  }

  return ErrorStatus(StatusCode::INVALID_ARGUMENT, "empty mutation");
}

Status MutationApplier::ProcessDocumentStatus(const Status& status,
                                              StatusPredicate is_merge_status) {
  {
    if (status.ok()) return status;
    if (is_merge_status(status)) {
      running_status_ = std::max(
          running_status_,
          MutationApplicationStatus::SUCCESS_WITH_MERGE_CONFLICT_RESOLVED);
      return OkStatus();
    }
    if (status::IsInvalidArgument(status)) {
      running_status_ =
          std::max(running_status_, MutationApplicationStatus::BAD_MUTATION);
    } else {
      running_status_ =
          std::max(running_status_, MutationApplicationStatus::INTERNAL_ERROR);
    }
    return status;
  }
}

}  // namespace ink
