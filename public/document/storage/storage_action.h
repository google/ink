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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_ACTION_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_ACTION_H_

#include <map>
#include <memory>

#include "ink/engine/public/host/iactive_layer_listener.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/imutation_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/security.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/document/storage/document_storage.h"

namespace ink {

struct RemovedUUID {
  const UUID uuid;
  const UUID was_below_uuid;
  RemovedUUID(const UUID& uuid, const UUID& was_below_uuid)
      : uuid(uuid), was_below_uuid(was_below_uuid) {}
};

// A StorageAction is a reversible mutation of a DocumentStorage.
// This is useful for undo/redo.
//
// A StorageAction implements a state machine:
//
// +----------------+          +----------+  Undo()  +---------+
// |                |  Apply() |          +--------> |         |
// | kUninitialized +--------> | kApplied |          | kUndone |
// |                |          |          | <--------+         |
// +----------------+          +----------+  Redo()  +---------+
//
// Generally, if you're using StorageActions or UndoManager you don't want to
// mutate the Document store directly. This is because StorageActions expect
// element data to be availible in the store at arbitrarily later times.
// Ex:
//   AddAction add(storage)
//   add.Apply(bundle)
//   storage->Remove(bundle.id)
//   add.Undo() // Breaks!
//
//   The AddAction assumes the data for the element will still be availible,
//   but the call to storage->Remove(id) breaks this assumption
//
// If you want to mutate the store anyways (ex for storage compaction under
// memory pressure) you can determine which individual elements should not be
// mutated through the call StorageAction::AffectedUUIDs.
class StorageAction {
 public:
  enum class State { kUninitialized, kApplied, kUndone };

 public:
  // A UUIDOrder represents an ordered sequence of removed UUIDs. This is used
  // when adding elements, or when undoing an operation that removes elements.
  // The order will always be top-down, so that below-uuid relationships can be
  // reconstructed. For example, the UUIDOrder {{"B", "C"}, {"A", "B"}} means
  // "first add B beneath C, then add A beneath B".
  using UUIDOrder = std::vector<RemovedUUID>;

  StorageAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);
  virtual ~StorageAction() {}

  // StorageAction inheritors must provide an "Apply" function
  //   - Callers must call Apply before any other function!
  //   - Implementors must go from state kUninitialized to kApplied

  S_WARN_UNUSED_RESULT Status Undo();
  S_WARN_UNUSED_RESULT Status Redo();
  virtual std::vector<UUID> AffectedUUIDs() const = 0;
  State state() const;

  void WriteToProto(ink::proto::StorageAction* proto);
  void RestoreFromProto(const ink::proto::StorageAction& proto, State state);

  virtual std::string ToString() const { return "<Abstract StorageAction>"; }

 protected:
  virtual S_WARN_UNUSED_RESULT Status UndoImpl() = 0;
  virtual S_WARN_UNUSED_RESULT Status RedoImpl() = 0;

  void NotifyHostAdd(UUIDOrder uuid_order, const proto::SourceDetails& source);
  void NotifyHostRemove(std::vector<UUID> removed_uuids,
                        const proto::SourceDetails& source);

  virtual void WriteFieldsToProto(ink::proto::StorageAction* proto) const = 0;
  virtual void RestoreFieldsFromProto(
      const ink::proto::StorageAction& proto) = 0;

  State state_ = State::kUninitialized;
  std::shared_ptr<DocumentStorage> storage_;
  std::shared_ptr<EventDispatch<IElementListener>> element_dispatch_;
  std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch_;
};

////////////////////////////////////////////////////////////////////////////////

// Add any number of elements to storage.
class AddAction : public StorageAction {
 public:
  explicit AddAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);
  S_WARN_UNUSED_RESULT Status Apply(
      const std::vector<ink::proto::ElementBundle>& bundles,
      const UUID& below_element_with_uuid, const proto::SourceDetails& source);
  std::vector<UUID> AffectedUUIDs() const override;

  std::string ToString() const override { return "<AddAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;

 private:
  // Pairs (uuid, below_uuid) are used to preserve the order of elements added
  // for undo/redo.
  UUIDOrder GetUUIDOrder() const;

  std::vector<UUID> uuids_;
  UUID below_element_with_uuid_;
};

////////////////////////////////////////////////////////////////////////////////

// Remove one or more elements from storage.
class RemoveAction : public StorageAction {
 public:
  explicit RemoveAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);

  // Returns true if any of the given UUIDs were successfully removed.
  S_WARN_UNUSED_RESULT Status Apply(const std::vector<UUID>& uuids,
                                    const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override;

  std::string ToString() const override { return "<RemoveAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;

 private:
  UUIDOrder uuid_order_;
};

////////////////////////////////////////////////////////////////////////////////

// Remove one or more elements from storage, while simultaneously adding zero or
// more elements.
class ReplaceAction : public StorageAction {
 public:
  ReplaceAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);

  S_WARN_UNUSED_RESULT Status
  Apply(const std::vector<proto::ElementBundle>& elements_to_add,
        const std::vector<UUID>& uuids_to_add_below,
        const std::vector<UUID>& uuids_to_remove,
        const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override;

  std::string ToString() const override { return "<ReplaceAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;

 private:
  void NotifyHost(const UUIDOrder& removed_uuids, const UUIDOrder& added_uuids,
                  const proto::SourceDetails& source) const;

  UUIDOrder removed_uuid_order_;
  UUIDOrder added_uuid_order_;
};

////////////////////////////////////////////////////////////////////////////////

// Clear all elements from storage.
class ClearAction : public StorageAction {
 public:
  explicit ClearAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);
  S_WARN_UNUSED_RESULT Status Apply(const proto::SourceDetails& source);
  std::vector<UUID> AffectedUUIDs() const override;

  std::string ToString() const override { return "<ClearAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;

 private:
  std::vector<UUID> uuids_;
};

////////////////////////////////////////////////////////////////////////////////

// Set the transform for a list of elements in the storage.
class SetTransformAction : public StorageAction {
 public:
  explicit SetTransformAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch);
  S_WARN_UNUSED_RESULT Status
  Apply(const std::vector<UUID>& uuids,
        const std::vector<proto::AffineTransform>& new_transforms,
        const proto::SourceDetails& source);
  std::vector<UUID> AffectedUUIDs() const override;

  std::string ToString() const override { return "<SetTransformAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;

 private:
  void NotifyHost(std::vector<UUID> uuids,
                  std::vector<proto::AffineTransform> transforms,
                  const proto::SourceDetails& source);

  std::vector<UUID> uuids_;
  std::vector<proto::AffineTransform> from_transforms_;
  std::vector<proto::AffineTransform> to_transforms_;
};

template <>
inline std::string Str(const ink::StorageAction::State& s) {
  switch (s) {
    case ink::StorageAction::State::kUninitialized:
      return "kUninitialized";
    case ink::StorageAction::State::kApplied:
      return "kApplied";
    case ink::StorageAction::State::kUndone:
      return "kUndone";
  }
}

////////////////////////////////////////////////////////////////////////////////

class SetPageBoundsAction : public StorageAction {
 public:
  SetPageBoundsAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
      std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch);

  S_WARN_UNUSED_RESULT Status Apply(const proto::Rect& bounds,
                                    const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override { return {}; }

  std::string ToString() const override { return "<SetPageBoundsAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;

  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;

 private:
  S_WARN_UNUSED_RESULT Status SetBoundsAndNotify(
      const proto::Rect& bounds, const proto::SourceDetails& source);

  proto::Rect old_bounds_;
  proto::Rect new_bounds_;
  std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch_;
};

////////////////////////////////////////////////////////////////////////////////

class SetActiveLayerAction : public StorageAction {
 public:
  SetActiveLayerAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
      std::shared_ptr<EventDispatch<IActiveLayerListener>>
          active_layer_dispatch);

  S_WARN_UNUSED_RESULT Status Apply(const UUID& uuid,
                                    const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override {
    return {old_uuid_, new_uuid_};
  }

  std::string ToString() const override { return "<SetActiveLayerAction>"; }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;

  void WriteFieldsToProto(proto::StorageAction* proto) const override;
  void RestoreFieldsFromProto(const proto::StorageAction& proto) override;

 private:
  Status SetActiveLayerAndNotify(const UUID& uuid,
                                 const proto::SourceDetails& source);

  UUID old_uuid_;
  UUID new_uuid_;

  std::shared_ptr<EventDispatch<IActiveLayerListener>> active_layer_dispatch_;
};

////////////////////////////////////////////////////////////////////////////////

class SetVisibilityAction : public StorageAction {
 public:
  SetVisibilityAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
      : StorageAction(storage, element_dispatch, mutation_dispatch) {}

  S_WARN_UNUSED_RESULT Status Apply(const std::vector<UUID>& uuids,
                                    const std::vector<bool>& visibilities,
                                    const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override { return uuids_; }
  std::string ToString() const override { return "<SetVisibilityAction>"; }

  // Used by ApplyRepeatedStorageAction
  using ValueType = bool;
  ValueType GetValueFromMutation(
      const proto::ElementVisibilityMutations::Mutation& mutation) {
    return mutation.visibility();
  }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;

  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;

 private:
  void NotifyHost(const std::vector<UUID>& uuids,
                  const std::vector<bool>& visibilities,
                  const proto::SourceDetails& source);

  std::vector<UUID> uuids_;
  std::vector<bool> from_visibilities_;
  std::vector<bool> to_visibilities_;
};

class SetOpacityAction : public StorageAction {
 public:
  SetOpacityAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
      : StorageAction(storage, element_dispatch, mutation_dispatch) {}

  S_WARN_UNUSED_RESULT Status Apply(const std::vector<UUID>& uuids,
                                    const std::vector<int32>& opacities,
                                    const proto::SourceDetails& source);

  std::vector<UUID> AffectedUUIDs() const override { return uuids_; }
  std::string ToString() const override { return "<SetOpacityAction>"; }

  // Used by ApplyRepeatedStorageAction
  using ValueType = int32;
  ValueType GetValueFromMutation(
      const proto::ElementOpacityMutations::Mutation& mutation) {
    return mutation.opacity();
  }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;

  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;

 private:
  void NotifyHost(const std::vector<UUID>& uuids,
                  const std::vector<int32>& opacities,
                  const proto::SourceDetails& source);

  std::vector<UUID> uuids_;
  std::vector<int32> from_opacities_;
  std::vector<int32> to_opacities_;
};

class ChangeZOrderAction : public StorageAction {
 public:
  ChangeZOrderAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
      : StorageAction(storage, element_dispatch, mutation_dispatch) {}

  S_WARN_UNUSED_RESULT Status Apply(const std::vector<UUID>& uuids,
                                    const std::vector<UUID>& below_uuids,
                                    const proto::SourceDetails& source);
  std::vector<UUID> AffectedUUIDs() const override;
  std::string ToString() const override { return "<ChangeZOrderAction>"; }

  // Used by ApplyRepeatedStorageAction
  using ValueType = UUID;
  ValueType GetValueFromMutation(
      const proto::ElementZOrderMutations::Mutation& mutation) {
    return mutation.below_uuid();
  }

 protected:
  S_WARN_UNUSED_RESULT Status UndoImpl() override;
  S_WARN_UNUSED_RESULT Status RedoImpl() override;

  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override;
  void RestoreFieldsFromProto(const ink::proto::StorageAction& proto) override;

 private:
  void NotifyHost(const std::vector<UUID>& uuids,
                  const std::vector<UUID>& below_uuids,
                  const proto::SourceDetails& source);

  std::vector<UUID> uuids_;
  std::vector<UUID> to_below_uuids_;
  // Undo actions need to be applied in reverse, so we store the from values
  // reversed.
  std::vector<UUID> from_below_uuids_reversed_;
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_ACTION_H_
