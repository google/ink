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

#ifndef INK_PUBLIC_DOCUMENT_SINGLE_USER_DOCUMENT_H_
#define INK_PUBLIC_DOCUMENT_SINGLE_USER_DOCUMENT_H_

#include <memory>
#include <string>

#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/public/document/document.h"
#include "ink/public/document/storage/document_storage.h"
#include "ink/public/document/storage/storage_action.h"
#include "ink/public/document/storage/undo_manager.h"

namespace ink {

// SingleUserDocument is thread-safe.
class SingleUserDocument : public Document {
 public:
  explicit SingleUserDocument(std::shared_ptr<DocumentStorage> storage);

  static S_WARN_UNUSED_RESULT Status CreateFromSnapshot(
      std::shared_ptr<DocumentStorage> storage,
      const ink::proto::Snapshot& snapshot, std::unique_ptr<Document>* out);

  bool SupportsUndo() const override { return true; }
  void Undo() override;
  void Redo() override;
  bool CanUndo() const override;
  bool CanRedo() const override;
  void SetUndoEnabled(bool enabled) override;

  bool SupportsQuerying() override { return true; }

  bool SupportsPaging() override { return true; }

  proto::PageProperties GetPageProperties() const override;

  ink::proto::Snapshot GetSnapshot(
      SnapshotQuery query = SnapshotQuery::INCLUDE_UNDO_STACK) const override;

  bool IsEmpty() override { return storage_->IsEmpty(); }

  std::string ToString() const override;

 protected:
  S_WARN_UNUSED_RESULT Status
  AddBelowImpl(const std::vector<ink::proto::ElementBundle>& elements,
               const UUID& below_element_with_uuid,
               const proto::SourceDetails& source_details) override;
  S_WARN_UNUSED_RESULT Status
  RemoveImpl(const std::vector<UUID>& uuid,
             const proto::SourceDetails& source_details) override;
  S_WARN_UNUSED_RESULT Status
  RemoveAllImpl(ink::proto::ElementIdList* removed,
                const proto::SourceDetails& source_details) override;
  S_WARN_UNUSED_RESULT Status
  ReplaceImpl(const std::vector<proto::ElementBundle>& elements_to_add,
              const std::vector<UUID>& uuids_to_add_below,
              const std::vector<UUID>& uuids_to_remove,
              const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status
  SetElementTransformsImpl(const std::vector<UUID> uuids,
                           const std::vector<proto::AffineTransform> transforms,
                           const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status SetElementVisibilityImpl(
      const std::vector<UUID> uuids, const std::vector<bool> visibilities,
      const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status SetElementOpacityImpl(
      const std::vector<UUID> uuids, const std::vector<int32> opacities,
      const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status ChangeZOrderImpl(
      const std::vector<UUID> uuids, const std::vector<UUID> below_uuids,
      const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status ActiveLayerChangedImpl(
      const UUID& uuid, const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status
  SetPagePropertiesImpl(const proto::PageProperties& page_properties,
                        const proto::SourceDetails& source_details) override;
  S_WARN_UNUSED_RESULT Status UndoableSetPageBoundsImpl(
      const ink::proto::Rect& bounds,
      const proto::SourceDetails& source_details) override;

  S_WARN_UNUSED_RESULT Status
  AddPageImpl(const ink::proto::PerPageProperties& page) override;
  S_WARN_UNUSED_RESULT Status ClearPagesImpl() override;

 private:
  template <typename ActionType>
  Status ApplyRepeatedStorageAction(
      const std::vector<UUID> uuids,
      const std::vector<typename ActionType::ValueType> values,
      const proto::SourceDetails& source_details);

  void MaybeNotifyEmptyStateChanged();
  bool UnsafeCanUndo() const;
  bool UnsafeCanRedo() const;

  mutable absl::Mutex mutex_;

  std::shared_ptr<DocumentStorage> storage_;
  UndoManager undo_;

  bool last_reported_empty_state_;
};

};  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_SINGLE_USER_DOCUMENT_H_
