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

#include "ink/public/document/storage/undo_manager.h"
#include "ink/public/document/storage/storage_action.h"

namespace ink {

UndoManager::UndoManager(
    std::shared_ptr<EventDispatch<IDocumentListener>> document_dispatch,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
    std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch,
    std::shared_ptr<EventDispatch<IActiveLayerListener>> layer_dispatch,
    std::shared_ptr<DocumentStorage> storage)
    : document_dispatch_(std::move(document_dispatch)),
      element_dispatch_(std::move(element_dispatch)),
      mutation_dispatch_(std::move(mutation_dispatch)),
      page_dispatch_(std::move(page_dispatch)),
      layer_dispatch_(std::move(layer_dispatch)),
      storage_(std::move(storage)),
      last_undo_state_(false),
      last_redo_state_(false),
      enabled_(true) {}

void UndoManager::MaybeNotifyUndoRedoStateChanged() {
  bool can_undo = CanUndo();
  bool can_redo = CanRedo();
  if (can_undo != last_undo_state_ || can_redo != last_redo_state_) {
    last_undo_state_ = can_undo;
    last_redo_state_ = can_redo;
    document_dispatch_->Send(&IDocumentListener::UndoRedoStateChanged, can_undo,
                             can_redo);
  }
}

void UndoManager::Push(std::unique_ptr<StorageAction> action) {
  if (!enabled_) return;
  redoables_.clear();
  undoables_.emplace_back(std::move(action));
  MaybeNotifyUndoRedoStateChanged();
}

bool UndoManager::Undo() {
  if (!CanUndo()) return false;

  auto success = undoables_.back()->Undo();
  if (success) {
    redoables_.emplace_back(std::move(undoables_.back()));
  } else {
    SLOG(SLOG_ERROR, "$0", success.error_message());
  }
  undoables_.pop_back();
  MaybeNotifyUndoRedoStateChanged();
  return success.ok();
}

bool UndoManager::Redo() {
  if (!CanRedo()) return false;

  auto success = redoables_.back()->Redo();
  if (success) {
    undoables_.emplace_back(std::move(redoables_.back()));
  } else {
    SLOG(SLOG_ERROR, "$0", success.error_message());
  }
  redoables_.pop_back();
  MaybeNotifyUndoRedoStateChanged();
  return success.ok();
}

bool UndoManager::CanUndo() const { return enabled_ && !undoables_.empty(); }

bool UndoManager::CanRedo() const { return enabled_ && !redoables_.empty(); }

std::vector<UUID> UndoManager::ReferencedElements() const {
  std::unordered_set<UUID> ids;

  for (const auto& u : undoables_) {
    auto affected = u->AffectedUUIDs();
    ids.insert(affected.begin(), affected.end());
  }
  for (const auto& u : redoables_) {
    auto affected = u->AffectedUUIDs();
    ids.insert(affected.begin(), affected.end());
  }

  return std::vector<UUID>(ids.begin(), ids.end());
}

void UndoManager::WriteToProto(ink::proto::Snapshot* snapshot) const {
  for (const auto& u : undoables_) {
    SLOG(SLOG_DOCUMENT, "Saving $0 into snapshot undoables", *u);
    u->WriteToProto(snapshot->add_undo_action());
  }
  for (const auto& r : redoables_) {
    SLOG(SLOG_DOCUMENT, "Saving $0 into snapshot redoables", *r);
    r->WriteToProto(snapshot->add_redo_action());
  }
}

std::unique_ptr<StorageAction> UndoManager::StorageActionFromProto(
    const ink::proto::StorageAction& proto) const {
  if (proto.has_add_action() || proto.has_add_multiple_action()) {
    return std::unique_ptr<StorageAction>(
        new AddAction(storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_remove_action()) {
    return std::unique_ptr<StorageAction>(
        new RemoveAction(storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_clear_action()) {
    return std::unique_ptr<StorageAction>(
        new ClearAction(storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_set_transform_action()) {
    return std::unique_ptr<StorageAction>(new SetTransformAction(
        storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_set_page_bounds_action()) {
    return std::unique_ptr<StorageAction>(new SetPageBoundsAction(
        storage_, element_dispatch_, mutation_dispatch_, page_dispatch_));
  }
  if (proto.has_set_visibility_action()) {
    return std::unique_ptr<StorageAction>(new SetVisibilityAction(
        storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_set_opacity_action()) {
    return std::unique_ptr<StorageAction>(
        new SetOpacityAction(storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_change_z_order_action()) {
    return std::unique_ptr<StorageAction>(new ChangeZOrderAction(
        storage_, element_dispatch_, mutation_dispatch_));
  }
  if (proto.has_set_active_layer_action()) {
    return std::unique_ptr<StorageAction>(new SetActiveLayerAction(
        storage_, element_dispatch_, mutation_dispatch_, layer_dispatch_));
  }
  if (proto.has_replace_action()) {
    return std::unique_ptr<StorageAction>(
        new ReplaceAction(storage_, element_dispatch_, mutation_dispatch_));
  }
  SLOG(SLOG_ERROR, "No known action found in StorageAction proto.");
  return nullptr;
}

void UndoManager::ReadFromProto(const ink::proto::Snapshot& snapshot) {
  undoables_.clear();
  redoables_.clear();
  for (const auto& undo_action : snapshot.undo_action()) {
    SLOG(SLOG_DOCUMENT, "Pushing something onto undo stack");
    auto action = StorageActionFromProto(undo_action);
    if (action) {
      action->RestoreFromProto(undo_action, StorageAction::State::kApplied);
      undoables_.emplace_back(std::move(action));
    }
  }
  for (const auto& redo_action : snapshot.redo_action()) {
    SLOG(SLOG_DOCUMENT, "Pushing something onto redo stack");
    auto action = StorageActionFromProto(redo_action);
    if (action) {
      action->RestoreFromProto(redo_action, StorageAction::State::kUndone);
      redoables_.emplace_back(std::move(action));
    }
  }
  MaybeNotifyUndoRedoStateChanged();
}

void UndoManager::SetEnabled(bool enabled) {
  enabled_ = enabled;
  MaybeNotifyUndoRedoStateChanged();
}
}  // namespace ink
