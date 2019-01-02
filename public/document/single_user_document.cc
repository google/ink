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

#include "ink/public/document/single_user_document.h"
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/public/document/storage/document_storage.h"

namespace ink {

SingleUserDocument::SingleUserDocument(std::shared_ptr<DocumentStorage> storage)
    : storage_(storage),
      undo_(DocumentDispatch(), ElementDispatch(), MutationDispatch(),
            PagePropertiesDispatch(), storage),
      last_reported_empty_state_(storage_->IsEmpty()) {}

/* static */ S_WARN_UNUSED_RESULT Status SingleUserDocument::CreateFromSnapshot(
    std::shared_ptr<DocumentStorage> storage,
    const ink::proto::Snapshot& snapshot, std::unique_ptr<Document>* out) {
  if (!storage->SupportsSnapshot()) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "$0 does not support Snapshot load", *storage);
  }
  auto doc = absl::make_unique<SingleUserDocument>(std::move(storage));
  INK_RETURN_UNLESS(doc->storage_->ReadFromProto(snapshot));
  doc->undo_.ReadFromProto(snapshot);
  *out = std::move(doc);
  return OkStatus();
}

void SingleUserDocument::MaybeNotifyEmptyStateChanged() {
  bool empty = storage_->IsEmpty();
  if (last_reported_empty_state_ != empty) {
    NotifyEmptyStateChanged(empty);
    last_reported_empty_state_ = empty;
  }
}

void SingleUserDocument::Undo() {
  absl::MutexLock lock(&mutex_);
  if (!UnsafeCanUndo()) {
    SLOG(SLOG_ERROR, "cannot undo");
    return;
  }
  undo_.Undo();
  MaybeNotifyEmptyStateChanged();
}

void SingleUserDocument::Redo() {
  absl::MutexLock lock(&mutex_);
  if (!UnsafeCanRedo()) {
    SLOG(SLOG_ERROR, "cannot redo");
    return;
  }
  undo_.Redo();
  MaybeNotifyEmptyStateChanged();
}

bool SingleUserDocument::CanUndo() const {
  absl::MutexLock lock(&mutex_);
  return UnsafeCanUndo();
}

bool SingleUserDocument::CanRedo() const {
  absl::MutexLock lock(&mutex_);
  return UnsafeCanRedo();
}

void SingleUserDocument::SetUndoEnabled(bool enabled) {
  absl::MutexLock lock(&mutex_);
  SLOG(SLOG_DOCUMENT, "$0 undo manager", enabled ? "enabling" : "disabling");
  undo_.SetEnabled(enabled);
}

bool SingleUserDocument::UnsafeCanUndo() const { return undo_.CanUndo(); }

bool SingleUserDocument::UnsafeCanRedo() const { return undo_.CanRedo(); }

ink::proto::Snapshot SingleUserDocument::GetSnapshot(
    SnapshotQuery query) const {
  const bool include_undo = query == INCLUDE_UNDO_STACK;
  absl::MutexLock lock(&mutex_);
  if (include_undo) {
    if (!storage_->RemoveDeadElements(
            MakeSTLRange(undo_.ReferencedElements()))) {
      SLOG(SLOG_ERROR, "could not remove dead elements");
    }
  }
  ink::proto::Snapshot snapshot;
  if (storage_->SupportsSnapshot()) {
    storage_->WriteToProto(&snapshot,
                           include_undo
                               ? DocumentStorage::INCLUDE_DEAD_ELEMENTS
                               : DocumentStorage::DO_NOT_INCLUDE_DEAD_ELEMENTS);
  }
  if (include_undo) {
    undo_.WriteToProto(&snapshot);
  }
  return snapshot;
}

Status SingleUserDocument::AddPageImpl(
    const ink::proto::PerPageProperties& page) {
  absl::MutexLock lock(&mutex_);
  return storage_->AddPage(page);
}

Status SingleUserDocument::ClearPagesImpl() {
  absl::MutexLock lock(&mutex_);
  return storage_->ClearPages();
}

Status SingleUserDocument::SetPagePropertiesImpl(
    const proto::PageProperties& page_properties,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  return storage_->SetPageProperties(page_properties);
}

Status SingleUserDocument::UndoableSetPageBoundsImpl(
    const ink::proto::Rect& bounds,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  std::unique_ptr<SetPageBoundsAction> action(
      new SetPageBoundsAction(storage_, ElementDispatch(), MutationDispatch(),
                              PagePropertiesDispatch()));
  INK_RETURN_UNLESS(action->Apply(bounds, source_details));
  undo_.Push(std::move(action));
  return OkStatus();
}

proto::PageProperties SingleUserDocument::GetPageProperties() const {
  absl::MutexLock lock(&mutex_);
  return storage_->GetPageProperties();
}

Status SingleUserDocument::AddBelowImpl(
    const std::vector<ink::proto::ElementBundle>& elements,
    const UUID& below_element_with_uuid,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  std::unique_ptr<AddAction> a(
      new AddAction(storage_, ElementDispatch(), MutationDispatch()));
  INK_RETURN_UNLESS(
      a->Apply(elements, below_element_with_uuid, source_details));
  undo_.Push(std::move(a));
  MaybeNotifyEmptyStateChanged();
  return OkStatus();
}

Status SingleUserDocument::RemoveImpl(
    const std::vector<UUID>& uuids,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  std::unique_ptr<RemoveAction> a(
      new RemoveAction(storage_, ElementDispatch(), MutationDispatch()));
  auto status = a->Apply(uuids, source_details);
  if (status.ok() || status::IsIncomplete(status)) {
    undo_.Push(std::move(a));
    MaybeNotifyEmptyStateChanged();
  }
  return status;
}

Status SingleUserDocument::RemoveAllImpl(
    ink::proto::ElementIdList* removed,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  std::unique_ptr<ClearAction> a(
      new ClearAction(storage_, ElementDispatch(), MutationDispatch()));
  INK_RETURN_UNLESS(a->Apply(source_details));
  auto ids = a->AffectedUUIDs();
  for (const auto& id : ids) {
    removed->add_uuid(id);
  }
  undo_.Push(std::move(a));
  MaybeNotifyEmptyStateChanged();
  return OkStatus();
}

Status SingleUserDocument::ReplaceImpl(
    const std::vector<proto::ElementBundle>& elements_to_add,
    const std::vector<UUID>& uuids_to_add_below,
    const std::vector<UUID>& uuids_to_remove,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  auto action = absl::make_unique<ReplaceAction>(storage_, ElementDispatch(),
                                                 MutationDispatch());
  INK_RETURN_UNLESS(action->Apply(elements_to_add, uuids_to_add_below,
                                  uuids_to_remove, source_details));
  undo_.Push(std::move(action));
  MaybeNotifyEmptyStateChanged();
  return OkStatus();
}

Status SingleUserDocument::SetElementTransformsImpl(
    const std::vector<UUID> uuids,
    const std::vector<proto::AffineTransform> transforms,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  std::unique_ptr<SetTransformAction> a(
      new SetTransformAction(storage_, ElementDispatch(), MutationDispatch()));

  Status status = a->Apply(uuids, transforms, source_details);
  if (status.ok() || status::IsIncomplete(status)) {
    undo_.Push(std::move(a));
    MaybeNotifyEmptyStateChanged();
  }
  return status;
}

// For each mutation in the ElementMutations, creates a StorageAction (of type
// T), and Apply() it with the value (of type V) returned by value_member_func.
//
// The StorageActions will be added to the undo stack.
//
// If any Apply() returns a failure Status, stops the iteration and returns the
// Status.
template <typename ActionType>
Status SingleUserDocument::ApplyRepeatedStorageAction(
    const std::vector<UUID> uuids,
    const std::vector<typename ActionType::ValueType> values,
    const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  auto a = absl::make_unique<ActionType>(storage_, ElementDispatch(),
                                         MutationDispatch());

  Status status = a->Apply(uuids, values, source_details);
  if (status.ok() || status::IsIncomplete(status)) {
    undo_.Push(std::move(a));
    MaybeNotifyEmptyStateChanged();
  }
  return status;
}

Status SingleUserDocument::SetElementVisibilityImpl(
    const std::vector<UUID> uuids, const std::vector<bool> visibilities,
    const proto::SourceDetails& source_details) {
  return ApplyRepeatedStorageAction<SetVisibilityAction>(uuids, visibilities,
                                                         source_details);
}

Status SingleUserDocument::SetElementOpacityImpl(
    const std::vector<UUID> uuids, const std::vector<int32> opacities,
    const proto::SourceDetails& source_details) {
  return ApplyRepeatedStorageAction<SetOpacityAction>(uuids, opacities,
                                                      source_details);
}

Status SingleUserDocument::ChangeZOrderImpl(
    const std::vector<UUID> uuids, const std::vector<UUID> below_uuids,
    const proto::SourceDetails& source_details) {
  return ApplyRepeatedStorageAction<ChangeZOrderAction>(uuids, below_uuids,
                                                        source_details);
}

Status SingleUserDocument::ActiveLayerChangedImpl(
    const UUID& uuid, const proto::SourceDetails& source_details) {
  absl::MutexLock lock(&mutex_);
  auto a = absl::make_unique<SetActiveLayerAction>(
      storage_, ElementDispatch(), MutationDispatch(), ActiveLayerDispatch());
  Status status = a->Apply(uuid, source_details);
  if (status.ok() || status::IsIncomplete(status)) {
    undo_.Push(std::move(a));
  }
  return status;
}

std::string SingleUserDocument::ToString() const {
  return Substitute("<SingleUserDocument with $0>", *storage_);
}

};  // namespace ink
