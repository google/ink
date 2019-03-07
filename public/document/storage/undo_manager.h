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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_UNDO_MANAGER_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_UNDO_MANAGER_H_

#include <deque>

#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/imutation_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/public/document/idocument_listener.h"
#include "ink/public/document/storage/document_storage.h"
#include "ink/public/document/storage/storage_action.h"

namespace ink {

// A class to manage an undo/redo stack.
class UndoManager {
 public:
  UndoManager(
      std::shared_ptr<EventDispatch<IDocumentListener>> document_dispatch,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
      std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch,
      std::shared_ptr<EventDispatch<IActiveLayerListener>> layer_dispatch,
      std::shared_ptr<DocumentStorage> storage);
  void Push(std::unique_ptr<StorageAction> action);
  bool Undo();
  bool Redo();

  bool CanUndo() const;
  bool CanRedo() const;

  // See storage_action.h
  std::vector<UUID> ReferencedElements() const;

  void WriteToProto(ink::proto::Snapshot* snapshot) const;
  void ReadFromProto(const ink::proto::Snapshot& snapshot);

  // If false, all pushed StorageActions are ignored and Undo() and Redo() don't
  // do anything.
  void SetEnabled(bool enabled);

 private:
  void MaybeNotifyUndoRedoStateChanged();
  std::unique_ptr<StorageAction> StorageActionFromProto(
      const ink::proto::StorageAction& proto) const;

  std::deque<std::unique_ptr<StorageAction>> undoables_;
  std::deque<std::unique_ptr<StorageAction>> redoables_;
  std::shared_ptr<EventDispatch<IDocumentListener>> document_dispatch_;
  std::shared_ptr<EventDispatch<IElementListener>> element_dispatch_;
  std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch_;
  std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch_;
  std::shared_ptr<EventDispatch<IActiveLayerListener>> layer_dispatch_;
  std::shared_ptr<DocumentStorage> storage_;

  bool last_undo_state_;
  bool last_redo_state_;
  bool enabled_;

  friend class UndoManagerTest;
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_UNDO_MANAGER_H_
