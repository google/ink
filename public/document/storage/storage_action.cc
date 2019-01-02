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

#include "ink/public/document/storage/storage_action.h"

#include <algorithm>
#include <iterator>

#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/engine/util/range.h"
#include "ink/proto/helpers.h"
#include "ink/proto/mutations_portable_proto.pb.h"
#include "ink/public/document/bundle_data_attachments.h"

#if defined(PROTOBUF_INTERNAL_IMPL)
using proto2::RepeatedFieldBackInserter;
#else
#include "google/protobuf/repeated_field.h"
using google::protobuf::RepeatedFieldBackInserter;
#endif

template <typename Iterable, typename RepeatedPtrField>
static void CopyToProto(const Iterable& it, RepeatedPtrField* field) {
  std::copy(it.begin(), it.end(), RepeatedFieldBackInserter(field));
}

template <typename Iterable, typename Vector>
static void CopyToVector(const Iterable& it, Vector* v) {
  v->assign(it.begin(), it.end());
}

namespace ink {

namespace {
// Given a z-ordered list of UUIDs, return a vector of pairs {uuid[N],
// uuid[N+1]}, where the last pair is {uuid[uuids.size()-1], below}.
template <typename T>
StorageAction::UUIDOrder OrderedElementPairs(const T& uuids, UUID below) {
  StorageAction::UUIDOrder uuid_order;
  for (int i = uuids.size() - 1; i >= 0; i--) {
    const UUID below_uuid = (i == uuids.size() - 1) ? below : uuids[i + 1];
    uuid_order.emplace_back(uuids[i], below_uuid);
  }
  return uuid_order;
}

// Returns the list of UUIDs to be removed from a list of UUID orders.
std::vector<UUID> GetUUIDsFromUUIDOrder(
    const StorageAction::UUIDOrder& uuid_order) {
  std::vector<UUID> uuids;
  uuids.reserve(uuid_order.size());
  for (const auto& remove_order : uuid_order)
    uuids.push_back(remove_order.uuid);
  return uuids;
}

// This helper function removes the given UUIDs from storage, and populates the
// UUIDOrder with the removed UUIDs and UUIDs of the element above them.
Status RemoveHelper(const std::vector<UUID>& uuids, DocumentStorage* storage,
                    StorageAction::UUIDOrder* uuid_order) {
  // Don't remove something that's already gone; it will create a bogus
  // action on the undo stack. (b/30693520)
  std::unordered_set<UUID> live_uuids;
  for (const UUID& uuid : uuids) {
    if (storage->IsAlive(uuid)) {
      live_uuids.emplace(uuid);
    } else {
      SLOG(SLOG_WARNING, "skipping already-dead $0", uuid);
    }
  }
  if (live_uuids.empty()) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "cowardly refusing to remove elements that are already not alive");
  }
  // Find the bundles to be removed in the sorted list of bundles -- if we undo,
  // they need to be inserted below the same elements they are below now.
  std::vector<proto::ElementBundle> all_bundles;
  INK_RETURN_UNLESS(storage->GetAllBundles(
      BundleDataAttachments::None(), LivenessFilter::kOnlyAlive, &all_bundles));
  uuid_order->reserve(live_uuids.size());
  for (int i = 0; i < all_bundles.size(); ++i) {
    auto live_uuid_it = live_uuids.find(all_bundles[i].uuid());
    if (live_uuid_it != live_uuids.end()) {
      UUID uuid_above = kInvalidUUID;
      if (i != all_bundles.size() - 1) uuid_above = all_bundles[i + 1].uuid();
      uuid_order->emplace_back(*live_uuid_it, uuid_above);

      // If we've already found all of the elements we're looking for, we don't
      // need to iterate over the remaining ones.
      if (uuid_order->size() == live_uuids.size()) break;
    }
  }
  return storage->SetLiveness(MakeSTLRange(live_uuids), Liveness::kDead);
}

}  // namespace

proto::SourceDetails HostSource() {
  proto::SourceDetails source;
  source.set_origin(proto::SourceDetails::HOST);
  return source;
}

void StorageAction::NotifyHostAdd(UUIDOrder uuid_order,
                                  const proto::SourceDetails& source) {
  std::vector<UUID> keys;
  std::unordered_map<UUID, UUID> uuid_to_below_uuid;
  for (const auto& removed_uuid : uuid_order) {
    keys.push_back(removed_uuid.uuid);
    uuid_to_below_uuid.emplace(removed_uuid.uuid, removed_uuid.was_below_uuid);
  }

  std::vector<proto::ElementBundle> bundles;
  // element and transform are required for a re-add from
  // storage to be possible, so require those attachments.
  if (storage_->GetBundles(MakeSTLRange(keys), {true, true, false},
                           LivenessFilter::kOnlyAlive, &bundles)) {
    proto::mutations::Mutation mutation;
    proto::ElementBundleAdds adds;
    // GetBundles returns elements in z-order, but we must add higher elements
    // first so that they'll be there when lower elements get added beneath
    // them.
    for (auto bi = bundles.rbegin(); bi != bundles.rend(); bi++) {
      const auto& bundle = *bi;
      const auto& below_uuid = uuid_to_below_uuid.at(bundle.uuid());

      auto* mutation_element = mutation.add_chunk()->mutable_add_element();
      *(mutation_element->mutable_element()) = bundle;
      mutation_element->set_below_element_with_uuid(below_uuid);

      auto* add = adds.add_element_bundle_add();
      *(add->mutable_element_bundle()) = bundle;
      add->set_below_uuid(below_uuid);
    }
    element_dispatch_->Send(&IElementListener::ElementsAdded, adds, source);
    mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
  }
}

void StorageAction::NotifyHostRemove(std::vector<UUID> removed_uuids,
                                     const proto::SourceDetails& source) {
  proto::ElementIdList removed_ids;
  proto::mutations::Mutation mutation;
  for (const auto& uuid : removed_uuids) {
    *removed_ids.add_uuid() = uuid;
    mutation.add_chunk()->mutable_remove_element()->set_uuid(uuid);
  }
  element_dispatch_->Send(&IElementListener::ElementsRemoved, removed_ids,
                          source);
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void SetTransformAction::NotifyHost(
    std::vector<UUID> uuids, std::vector<proto::AffineTransform> transforms,
    const proto::SourceDetails& source) {
  EXPECT(uuids.size() == transforms.size());

  proto::mutations::Mutation mutation;
  proto::ElementTransformMutations element_mutations;
  for (size_t i = 0; i < uuids.size(); i++) {
    auto* element_mutation = element_mutations.add_mutation();
    element_mutation->set_uuid(uuids[i]);
    *element_mutation->mutable_transform() = transforms[i];

    auto* mu_set_transform =
        mutation.add_chunk()->mutable_set_element_transform();
    mu_set_transform->set_uuid(uuids[i]);
    *mu_set_transform->mutable_transform() = transforms[i];
  }

  element_dispatch_->Send(&IElementListener::ElementsTransformMutated,
                          element_mutations, source);
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

StorageAction::StorageAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : storage_(std::move(storage)),
      element_dispatch_(std::move(element_dispatch)),
      mutation_dispatch_(std::move(mutation_dispatch)) {
  ASSERT(storage_);
  ASSERT(element_dispatch_);
  ASSERT(mutation_dispatch_);
}

Status StorageAction::Undo() {
  if (state_ != State::kApplied) {
    return ErrorStatus(
        StatusCode::FAILED_PRECONDITION,
        "invalid state transition for $0. Attempted undo() while in state $1",
        AddressStr(this), state_);
  }

  INK_RETURN_UNLESS(UndoImpl());
  state_ = State::kUndone;
  return OkStatus();
}

Status StorageAction::Redo() {
  if (state_ != State::kUndone) {
    return ErrorStatus(
        StatusCode::FAILED_PRECONDITION,
        "invalid state transition for $0. Attempted redo() while in state $1",
        AddressStr(this), state_);
  }

  INK_RETURN_UNLESS(RedoImpl());
  state_ = State::kApplied;
  return OkStatus();
}

void StorageAction::RestoreFromProto(const proto::StorageAction& proto,
                                     State state) {
  ASSERT(state != State::kUninitialized);
  RestoreFieldsFromProto(proto);
  state_ = state;
}

void StorageAction::WriteToProto(proto::StorageAction* proto) {
  ASSERT(state_ != State::kUninitialized);
  WriteFieldsToProto(proto);
}

StorageAction::State StorageAction::state() const { return state_; }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

AddAction::AddAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch) {}

Status AddAction::Apply(const std::vector<ink::proto::ElementBundle>& bundles,
                        const UUID& below_element_with_uuid,
                        const proto::SourceDetails& source) {
  for (int i = 0; i < bundles.size(); ++i) {
    uuids_.push_back(bundles[i].uuid());
  }

  below_element_with_uuid_ = below_element_with_uuid;
  INK_RETURN_UNLESS(
      storage_->Add(MakeSTLRange(bundles), below_element_with_uuid));
  state_ = State::kApplied;
  NotifyHostAdd(OrderedElementPairs(uuids_, below_element_with_uuid_), source);
  return OkStatus();
}

Status AddAction::UndoImpl() {
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids_), Liveness::kDead));
  NotifyHostRemove({uuids_}, HostSource());
  return OkStatus();
}

Status AddAction::RedoImpl() {
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids_), Liveness::kAlive));

  NotifyHostAdd(OrderedElementPairs(uuids_, below_element_with_uuid_),
                HostSource());
  return OkStatus();
}

std::vector<UUID> AddAction::AffectedUUIDs() const { return uuids_; }

void AddAction::RestoreFieldsFromProto(const proto::StorageAction& proto) {
  uuids_.clear();
  if (proto.has_add_action()) {
    const auto& add = proto.add_action();
    uuids_.emplace_back(add.uuid());
    below_element_with_uuid_ = add.below_element_with_uuid();
  } else {
    const auto& add = proto.add_multiple_action();
    for (int i = 0; i < add.uuid_size(); i++) {
      uuids_.emplace_back(add.uuid(i));
    }
    below_element_with_uuid_ = add.below_element_with_uuid();
  }
}

void AddAction::WriteFieldsToProto(proto::StorageAction* proto) const {
  if (uuids_.size() == 1) {
    // Use old Add proto to remain compatible with Keep server.
    proto->mutable_add_action()->set_uuid(uuids_[0]);
    proto->mutable_add_action()->set_below_element_with_uuid(
        below_element_with_uuid_);
  } else {
    for (auto& uuid : uuids_) {
      proto->mutable_add_multiple_action()->add_uuid(uuid);
    }
    proto->mutable_add_multiple_action()->set_below_element_with_uuid(
        below_element_with_uuid_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

RemoveAction::RemoveAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch) {}

Status RemoveAction::Apply(const std::vector<UUID>& uuids,
                           const proto::SourceDetails& source) {
  INK_RETURN_UNLESS(RemoveHelper(uuids, storage_.get(), &uuid_order_));
  NotifyHostRemove(uuids, source);
  state_ = State::kApplied;
  return uuid_order_.size() == uuids.size()
             ? OkStatus()
             : ErrorStatus(StatusCode::INCOMPLETE,
                           "$0 of the $1 given elements was already removed",
                           uuids.size() - uuid_order_.size(), uuids.size());
}

Status RemoveAction::UndoImpl() {
  std::vector<UUID> uuids = GetUUIDsFromUUIDOrder(uuid_order_);
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids), Liveness::kAlive));
  NotifyHostAdd(uuid_order_, HostSource());
  return OkStatus();
}

Status RemoveAction::RedoImpl() {
  std::vector<UUID> uuids = AffectedUUIDs();
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids), Liveness::kDead));
  NotifyHostRemove(uuids, HostSource());
  return OkStatus();
}

void RemoveAction::WriteFieldsToProto(proto::StorageAction* proto) const {
  for (const RemovedUUID& removed_uuid : uuid_order_) {
    proto->mutable_remove_action()->add_uuid(removed_uuid.uuid);
    proto->mutable_remove_action()->add_was_below_uuid(
        removed_uuid.was_below_uuid);
  }
}

void RemoveAction::RestoreFieldsFromProto(const proto::StorageAction& proto) {
  uuid_order_.clear();
  const auto& remove = proto.remove_action();
  if (remove.uuid_size() != remove.was_below_uuid_size()) {
    SLOG(SLOG_ERROR,
         "remove action cannot be restored from proto having $0 uuids and $1 "
         "was_below_uuids",
         remove.uuid_size(), remove.was_below_uuid_size());
    return;
  }
  for (int i = 0, n = remove.uuid_size(); i < n; i++) {
    uuid_order_.emplace_back(proto.remove_action().uuid(i),
                             proto.remove_action().was_below_uuid(i));
  }
  if (CheckLevel(SLOG_DOCUMENT)) {
    SLOG(SLOG_DOCUMENT, "Remove action restored with:");
    for (const auto& removed : uuid_order_) {
      SLOG(SLOG_DOCUMENT, "    $0 below $1", removed.uuid,
           removed.was_below_uuid);
    }
  }
}

std::vector<UUID> RemoveAction::AffectedUUIDs() const {
  return GetUUIDsFromUUIDOrder(uuid_order_);
}  // namespace ink

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ReplaceAction::ReplaceAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch) {}

Status ReplaceAction::Apply(
    const std::vector<proto::ElementBundle>& elements_to_add,
    const std::vector<UUID>& uuids_to_add_below,
    const std::vector<UUID>& uuids_to_remove,
    const proto::SourceDetails& source) {
  if (elements_to_add.size() != uuids_to_add_below.size())
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Size mismatch between elements to add and UUIDs to add below");

  // Insert the new elements before removing the old ones -- we do this because
  // we might be replacing sequential elements.
  added_uuid_order_.reserve(elements_to_add.size());
  for (int i = 0; i < elements_to_add.size(); ++i)
    added_uuid_order_.emplace_back(elements_to_add[i].uuid(),
                                   uuids_to_add_below[i]);
  INK_RETURN_UNLESS(
      storage_->Add(MakeSTLRange(elements_to_add), uuids_to_add_below));
  INK_RETURN_UNLESS(
      RemoveHelper(uuids_to_remove, storage_.get(), &removed_uuid_order_));

  NotifyHost(removed_uuid_order_, added_uuid_order_, source);
  state_ = State::kApplied;
  return OkStatus();
}

std::vector<UUID> ReplaceAction::AffectedUUIDs() const {
  std::vector<UUID> uuids;
  uuids.reserve(removed_uuid_order_.size() + added_uuid_order_.size());
  for (const auto uuid_pair : removed_uuid_order_)
    uuids.emplace_back(uuid_pair.uuid);
  for (const auto uuid_pair : added_uuid_order_)
    uuids.emplace_back(uuid_pair.uuid);
  return uuids;
}

Status ReplaceAction::UndoImpl() {
  std::vector<UUID> removed_uuids = GetUUIDsFromUUIDOrder(removed_uuid_order_);
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(removed_uuids), Liveness::kAlive));
  std::vector<UUID> added_uuids = GetUUIDsFromUUIDOrder(added_uuid_order_);
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(added_uuids), Liveness::kDead));

  NotifyHost(added_uuid_order_, removed_uuid_order_, HostSource());
  state_ = State::kUndone;
  return OkStatus();
}

Status ReplaceAction::RedoImpl() {
  std::vector<UUID> removed_uuids = GetUUIDsFromUUIDOrder(removed_uuid_order_);
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(removed_uuids), Liveness::kDead));
  std::vector<UUID> added_uuids = GetUUIDsFromUUIDOrder(added_uuid_order_);
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(added_uuids), Liveness::kAlive));

  NotifyHost(removed_uuid_order_, added_uuid_order_, HostSource());
  state_ = State::kApplied;
  return OkStatus();
}

void ReplaceAction::RestoreFieldsFromProto(
    const ink::proto::StorageAction& proto) {
  removed_uuid_order_.clear();
  const auto& replace_proto = proto.replace_action();
  if (replace_proto.removed_uuid_size() !=
      replace_proto.removed_was_below_uuid_size()) {
    SLOG(
        SLOG_ERROR,
        "Replace action cannot be restored from proto that has $0 removed_uuids"
        "and $1 removed_was_below_uuids",
        replace_proto.removed_uuid_size(),
        replace_proto.removed_was_below_uuid_size());
    return;
  }
  if (replace_proto.added_uuid_size() !=
      replace_proto.added_was_below_uuid_size()) {
    SLOG(SLOG_ERROR,
         "Replace action cannot be restored from proto that has $0 added_uuids"
         "and $1 added_was_below_uuids",
         replace_proto.added_uuid_size(),
         replace_proto.added_was_below_uuid_size());
    return;
  }
  for (int i = 0, n = replace_proto.removed_uuid_size(); i < n; i++) {
    removed_uuid_order_.emplace_back(replace_proto.removed_uuid(i),
                                     replace_proto.removed_was_below_uuid(i));
  }
  for (int i = 0, n = replace_proto.added_uuid_size(); i < n; i++) {
    added_uuid_order_.emplace_back(replace_proto.added_uuid(i),
                                   replace_proto.added_was_below_uuid(i));
  }
}

void ReplaceAction::WriteFieldsToProto(ink::proto::StorageAction* proto) const {
  for (const auto& uuid_pair : removed_uuid_order_) {
    proto->mutable_replace_action()->add_removed_uuid(uuid_pair.uuid);
    proto->mutable_replace_action()->add_removed_was_below_uuid(
        uuid_pair.was_below_uuid);
  }
  for (const auto& uuid_pair : removed_uuid_order_) {
    proto->mutable_replace_action()->add_added_uuid(uuid_pair.uuid);
    proto->mutable_replace_action()->add_added_was_below_uuid(
        uuid_pair.was_below_uuid);
  }
}

void ReplaceAction::NotifyHost(
    const UUIDOrder& removed_uuids, const UUIDOrder& added_uuids,
    const proto::SourceDetails& source_details) const {
  std::vector<UUID> bundles_to_fetch;
  bundles_to_fetch.reserve(added_uuids.size());
  std::unordered_map<UUID, UUID> added_uuid_to_below_uuid;
  for (const auto& uuid_pair : added_uuids) {
    bundles_to_fetch.emplace_back(uuid_pair.uuid);
    added_uuid_to_below_uuid[uuid_pair.uuid] = uuid_pair.was_below_uuid;
  }

  std::vector<proto::ElementBundle> bundles;
  if (storage_->GetBundles(MakeSTLRange(bundles_to_fetch), {true, true, false},
                           LivenessFilter::kOnlyAlive, &bundles)) {
    proto::ElementBundleReplace replace;
    proto::mutations::Mutation mutation;
    for (auto& bundle : bundles) {
      ASSERT(added_uuid_to_below_uuid.count(bundle.uuid()) > 0);
      UUID add_below_uuid = added_uuid_to_below_uuid[bundle.uuid()];
      ProtoHelpers::AddElementBundleAdd(bundle, add_below_uuid,
                                        replace.mutable_elements_to_add());
      auto* add_element = mutation.add_chunk()->mutable_add_element();
      *add_element->mutable_element() = std::move(bundle);
      add_element->set_below_element_with_uuid(add_below_uuid);
    }
    for (const auto& uuid_pair : removed_uuids) {
      replace.mutable_elements_to_remove()->add_uuid(uuid_pair.uuid);
      mutation.add_chunk()->mutable_remove_element()->set_uuid(uuid_pair.uuid);
    }

    element_dispatch_->Send(&IElementListener::ElementsReplaced, replace,
                            source_details);
    mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ClearAction::ClearAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch) {}

Status ClearAction::Apply(const proto::SourceDetails& source) {
  // Get a list of what's in the scene
  std::vector<ink::proto::ElementBundle> existing_elements;
  INK_RETURN_UNLESS(storage_->GetAllBundles(BundleDataAttachments::None(),
                                            LivenessFilter::kOnlyAlive,
                                            &existing_elements));
  if (existing_elements.empty()) {
    return ErrorStatus(StatusCode::NOT_FOUND,
                       "Clear action failed. No elements found in storage.");
  }

  uuids_.clear();
  uuids_.reserve(existing_elements.size());
  for (auto& element : existing_elements) {
    uuids_.emplace_back(element.uuid());
  }

  // apply the remove
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids_), Liveness::kDead));

  NotifyHostRemove(uuids_, source);

  state_ = State::kApplied;
  return OkStatus();
}

Status ClearAction::UndoImpl() {
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids_), Liveness::kAlive));
  NotifyHostAdd(OrderedElementPairs(uuids_, kInvalidUUID), HostSource());
  return OkStatus();
}

Status ClearAction::RedoImpl() {
  INK_RETURN_UNLESS(
      storage_->SetLiveness(MakeSTLRange(uuids_), Liveness::kDead));
  NotifyHostRemove(uuids_, HostSource());
  return OkStatus();
}

std::vector<UUID> ClearAction::AffectedUUIDs() const { return uuids_; }

void ClearAction::WriteFieldsToProto(proto::StorageAction* proto) const {
  CopyToProto(uuids_, proto->mutable_clear_action()->mutable_uuid());
}

void ClearAction::RestoreFieldsFromProto(const proto::StorageAction& proto) {
  CopyToVector(proto.clear_action().uuid(), &uuids_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SetTransformAction::SetTransformAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch) {}

Status SetTransformAction::Apply(
    const std::vector<UUID>& uuids,
    const std::vector<proto::AffineTransform>& new_transforms,
    const proto::SourceDetails& source) {
  EXPECT(uuids.size() == new_transforms.size());
  std::unordered_map<UUID, proto::AffineTransform> uuid_to_new_transform;
  for (size_t i = 0; i < uuids.size(); i++) {
    uuid_to_new_transform.emplace(uuids[i], new_transforms[i]);
  }

  // Get a list of what's currently set
  std::unordered_map<UUID, proto::AffineTransform> uuid_to_current_transform;
  INK_RETURN_UNLESS(storage_->GetTransforms(MakeSTLRange(uuids),
                                            LivenessFilter::kOnlyAlive,
                                            &uuid_to_current_transform));
  if (uuid_to_current_transform.size() != uuids.size()) {
    SLOG(SLOG_WARNING, "could not get the set of current transforms!");
    SLOG(SLOG_WARNING, "requested $0 transforms, found $1", uuids.size(),
         uuid_to_current_transform.size());
  }
  if (uuid_to_current_transform.empty()) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "SetTransformAction failed. No elements found to transform.");
  }

  // Initialize our member vars based on what we could read and what we're
  // trying to set.
  uuids_.reserve(uuid_to_current_transform.size());
  from_transforms_.reserve(uuid_to_current_transform.size());
  to_transforms_.reserve(uuid_to_current_transform.size());
  // Iterate over the given uuids in given order.
  for (const auto& uuid : uuids) {
    // But skip invalid ones.
    const auto& current_transform_it = uuid_to_current_transform.find(uuid);
    if (current_transform_it == uuid_to_current_transform.end()) continue;
    uuids_.emplace_back(uuid);
    from_transforms_.emplace_back(uuid_to_current_transform[uuid]);
    to_transforms_.emplace_back(uuid_to_new_transform[uuid]);
  }

  // Apply the transform
  INK_RETURN_UNLESS(storage_->SetTransforms(MakeSTLRange(uuids_),
                                            MakeSTLRange(to_transforms_)));

  NotifyHost(uuids_, to_transforms_, source);

  state_ = State::kApplied;
  return uuid_to_current_transform.size() == uuids.size()
             ? OkStatus()
             : ErrorStatus(StatusCode::INCOMPLETE,
                           "$0 of the $1 elements to transform were not found",
                           uuids.size() - uuid_to_current_transform.size(),
                           uuids.size());
}

Status SetTransformAction::UndoImpl() {
  INK_RETURN_UNLESS(storage_->SetTransforms(MakeSTLRange(uuids_),
                                            MakeSTLRange(from_transforms_)));
  NotifyHost(uuids_, from_transforms_, HostSource());
  return OkStatus();
}

Status SetTransformAction::RedoImpl() {
  INK_RETURN_UNLESS(storage_->SetTransforms(MakeSTLRange(uuids_),
                                            MakeSTLRange(to_transforms_)));
  NotifyHost(uuids_, to_transforms_, HostSource());
  return OkStatus();
}

std::vector<UUID> SetTransformAction::AffectedUUIDs() const { return uuids_; }

void SetTransformAction::WriteFieldsToProto(proto::StorageAction* proto) const {
  auto* action = proto->mutable_set_transform_action();
  CopyToProto(uuids_, action->mutable_uuid());
  CopyToProto(from_transforms_, action->mutable_from_transform());
  CopyToProto(to_transforms_, action->mutable_to_transform());
}

void SetTransformAction::RestoreFieldsFromProto(
    const proto::StorageAction& proto) {
  auto action = proto.set_transform_action();
  CopyToVector(action.uuid(), &uuids_);
  CopyToVector(action.from_transform(), &from_transforms_);
  CopyToVector(action.to_transform(), &to_transforms_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SetPageBoundsAction::SetPageBoundsAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
    std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch),
      page_dispatch_(page_dispatch) {}

Status SetPageBoundsAction::Apply(const proto::Rect& bounds,
                                  const proto::SourceDetails& source) {
  new_bounds_ = bounds;
  proto::PageProperties properties = storage_->GetPageProperties();
  old_bounds_ = properties.bounds();
  SLOG(SLOG_BOUNDS, "applying SetPageBoundsAction from $0 to $1", old_bounds_,
       bounds);
  *properties.mutable_bounds() = bounds;
  INK_RETURN_UNLESS(SetBoundsAndNotify(new_bounds_, source));

  state_ = State::kApplied;
  return OkStatus();
}

Status SetPageBoundsAction::UndoImpl() {
  return SetBoundsAndNotify(old_bounds_, HostSource());
}

Status SetPageBoundsAction::RedoImpl() {
  return SetBoundsAndNotify(new_bounds_, HostSource());
}

void SetPageBoundsAction::WriteFieldsToProto(
    ink::proto::StorageAction* proto) const {
  *proto->mutable_set_page_bounds_action()->mutable_old_bounds() = old_bounds_;
  *proto->mutable_set_page_bounds_action()->mutable_new_bounds() = new_bounds_;
}
void SetPageBoundsAction::RestoreFieldsFromProto(
    const ink::proto::StorageAction& proto) {
  old_bounds_ = proto.set_page_bounds_action().old_bounds();
  new_bounds_ = proto.set_page_bounds_action().new_bounds();
}

Status SetPageBoundsAction::SetBoundsAndNotify(
    const proto::Rect& bounds, const proto::SourceDetails& source) {
  proto::PageProperties properties = storage_->GetPageProperties();
  *properties.mutable_bounds() = bounds;
  INK_RETURN_UNLESS(storage_->SetPageProperties(properties));
  page_dispatch_->Send(&IPagePropertiesListener::PageBoundsChanged, bounds,
                       source);

  proto::mutations::Mutation mutation;
  *(mutation.add_chunk()->mutable_set_world_bounds()->mutable_bounds()) =
      bounds;
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);

  return OkStatus();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SetActiveLayerAction::SetActiveLayerAction(
    std::shared_ptr<DocumentStorage> storage,
    std::shared_ptr<EventDispatch<IElementListener>> element_dispatch,
    std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch,
    std::shared_ptr<EventDispatch<IActiveLayerListener>> active_layer_dispatch)
    : StorageAction(storage, element_dispatch, mutation_dispatch),
      active_layer_dispatch_(active_layer_dispatch) {}

Status SetActiveLayerAction::Apply(const UUID& uuid,
                                   const proto::SourceDetails& source) {
  new_uuid_ = uuid;
  old_uuid_ = storage_->GetActiveLayer();

  INK_RETURN_UNLESS(SetActiveLayerAndNotify(new_uuid_, source));
  state_ = State::kApplied;

  return OkStatus();
}

Status SetActiveLayerAction::UndoImpl() {
  return SetActiveLayerAndNotify(old_uuid_, HostSource());
}

Status SetActiveLayerAction::RedoImpl() {
  return SetActiveLayerAndNotify(new_uuid_, HostSource());
}

void SetActiveLayerAction::WriteFieldsToProto(
    proto::StorageAction* proto) const {
  proto->mutable_set_active_layer_action()->set_from_uuid(old_uuid_);
  proto->mutable_set_active_layer_action()->set_to_uuid(new_uuid_);
}

void SetActiveLayerAction::RestoreFieldsFromProto(
    const proto::StorageAction& proto) {
  old_uuid_ = proto.set_active_layer_action().from_uuid();
  new_uuid_ = proto.set_active_layer_action().to_uuid();
}

Status SetActiveLayerAction::SetActiveLayerAndNotify(
    const UUID& uuid, const proto::SourceDetails& source) {
  INK_RETURN_UNLESS(storage_->SetActiveLayer(uuid));
  active_layer_dispatch_->Send(&IActiveLayerListener::ActiveLayerChanged, uuid,
                               source);

  return OkStatus();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Copies:
//
//    requested_uuids  -> uuids_out
//    requested_values -> to_values_out
//    bundle.memfunc() -> from_values_out
//
// such that the three output vectors for ordered triples of
// (uuid_out[i], to_values_out[i], from_value_out[i]) show the old and new
// value of bundle.memfunc() for the bundles with the listed UUIDs.
//
// Any requested_uuids that are not in the bundles list will be pruned out
// and not be present in the output arrays.
//
// This code makes some assumptions:
//
// 1. requested_uuids.size() == requested_values.size().
// 2. bundles contains ONLY bundles with UUIDs in requested_uuids.
//
// NOTE: Because vector<bool>, this code uses push_back for the value vectors
// which has a performance cost with non-fundamental types. If you need to use
// this with non-fundamental types, you should rewrite this with emplace_back
// and include a specialization for 'bool' that uses push_back.
template <typename T, T (proto::ElementBundle::*MEMFUNC)() const>
absl::enable_if_t<std::is_fundamental<T>::value> PruneToExistingBundles(
    const std::vector<proto::ElementBundle>& bundles,
    const std::vector<UUID>& requested_uuids,
    const std::vector<T>& requested_values, std::vector<UUID>* uuids_out,
    std::vector<T>* from_values_out, std::vector<T>* to_values_out) {
  // Map from requested UUID to requested value.
  std::unordered_map<UUID, T> uuids_to_value;
  for (int i = 0; i < requested_uuids.size(); ++i) {
    uuids_to_value[requested_uuids[i]] = requested_values[i];
  }

  for (const auto& bundle : bundles) {
    uuids_out->emplace_back(bundle.uuid());
    from_values_out->push_back((bundle.*MEMFUNC)());
    to_values_out->push_back(uuids_to_value[bundle.uuid()]);
  }
}

Status SetVisibilityAction::Apply(const std::vector<UUID>& uuids_in,
                                  const std::vector<bool>& visibilities_in,
                                  const proto::SourceDetails& source) {
  EXPECT(uuids_in.size() == visibilities_in.size());

  std::vector<proto::ElementBundle> bundles;
  Status st = storage_->GetBundles(MakeSTLRange(uuids_in),
                                   BundleDataAttachments::None(),
                                   LivenessFilter::kOnlyAlive, &bundles);
  if (!st.ok()) return st;

  if (bundles.size() == uuids_in.size()) {
    uuids_ = uuids_in;
    to_visibilities_ = visibilities_in;
    transform(
        begin(bundles), end(bundles), std::back_inserter(from_visibilities_),
        [](const proto::ElementBundle& bundle) { return bundle.visibility(); });
  } else {
    PruneToExistingBundles<bool, &proto::ElementBundle::visibility>(
        bundles, uuids_in, visibilities_in, &uuids_, &from_visibilities_,
        &to_visibilities_);
  }

  if (uuids_.empty()) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "SetVisibilityAction failed. No elements found to transform.");
  }

  EXPECT(uuids_.size() == to_visibilities_.size());
  EXPECT(to_visibilities_.size() == from_visibilities_.size());

  INK_RETURN_UNLESS(storage_->SetVisibilities(uuids_, to_visibilities_));
  NotifyHost(uuids_, to_visibilities_, source);
  state_ = State::kApplied;
  return uuids_in.size() == uuids_.size()
             ? OkStatus()
             : ErrorStatus(
                   StatusCode::INCOMPLETE,
                   "$0 of the $1 elements for SetVisibilityAction not found.",
                   uuids_in.size() - uuids_.size(), uuids_in.size());
}

Status SetVisibilityAction::UndoImpl() {
  INK_RETURN_UNLESS(storage_->SetVisibilities(uuids_, from_visibilities_));
  state_ = State::kUndone;
  NotifyHost(uuids_, from_visibilities_, HostSource());
  return OkStatus();
}

Status SetVisibilityAction::RedoImpl() {
  INK_RETURN_UNLESS(storage_->SetVisibilities(uuids_, to_visibilities_));
  state_ = State::kApplied;
  NotifyHost(uuids_, to_visibilities_, HostSource());
  return OkStatus();
}

void SetVisibilityAction::NotifyHost(const std::vector<UUID>& uuids,
                                     const std::vector<bool>& visibilities,
                                     const proto::SourceDetails& source) {
  proto::mutations::Mutation mutation;
  proto::ElementVisibilityMutations element_mutations;
  for (int i = 0; i < uuids.size(); ++i) {
    auto* set_visibility = mutation.add_chunk()->mutable_set_visibility();
    set_visibility->set_uuid(uuids[i]);
    set_visibility->set_visibility(visibilities[i]);

    auto visibility_mutation = element_mutations.add_mutation();
    visibility_mutation->set_uuid(uuids[i]);
    visibility_mutation->set_visibility(visibilities[i]);
  }

  element_dispatch_->Send(&IElementListener::ElementsVisibilityMutated,
                          element_mutations, source);
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void SetVisibilityAction::WriteFieldsToProto(
    ink::proto::StorageAction* proto) const {
  auto* action = proto->mutable_set_visibility_action();

  for (int i = 0; i < uuids_.size(); ++i) {
    action->add_uuid(uuids_[i]);
    action->add_to_visibility(to_visibilities_[i]);
    action->add_from_visibility(from_visibilities_[i]);
  }
}

void SetVisibilityAction::RestoreFieldsFromProto(
    const ink::proto::StorageAction& proto) {
  uuids_.clear();
  to_visibilities_.clear();
  from_visibilities_.clear();

  const auto& action = proto.set_visibility_action();
  for (int i = 0; i < action.uuid_size(); ++i) {
    uuids_.emplace_back(action.uuid(i));
    to_visibilities_.push_back(action.to_visibility(i));
    from_visibilities_.push_back(action.from_visibility(i));
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Status SetOpacityAction::Apply(const std::vector<UUID>& uuids_in,
                               const std::vector<int32>& opacities_in,
                               const proto::SourceDetails& source) {
  EXPECT(uuids_in.size() == opacities_in.size());

  std::vector<proto::ElementBundle> bundles;
  Status st = storage_->GetBundles(MakeSTLRange<std::vector<UUID>>(uuids_in),
                                   BundleDataAttachments::None(),
                                   LivenessFilter::kOnlyAlive, &bundles);
  if (!st.ok()) return st;

  if (bundles.size() == uuids_in.size()) {
    uuids_ = uuids_in;
    to_opacities_ = opacities_in;
    transform(
        begin(bundles), end(bundles), std::back_inserter(from_opacities_),
        [](const proto::ElementBundle& bundle) { return bundle.opacity(); });
  } else {
    PruneToExistingBundles<int, &proto::ElementBundle::opacity>(
        bundles, uuids_in, opacities_in, &uuids_, &from_opacities_,
        &to_opacities_);
  }

  if (uuids_.empty()) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "SetOpacityAction failed. No elements found to transform.");
  }

  EXPECT(uuids_.size() == to_opacities_.size());
  EXPECT(to_opacities_.size() == from_opacities_.size());

  INK_RETURN_UNLESS(storage_->SetOpacities(uuids_, to_opacities_));
  state_ = State::kApplied;
  NotifyHost(uuids_, to_opacities_, source);
  return uuids_in.size() == uuids_.size()
             ? OkStatus()
             : ErrorStatus(
                   StatusCode::INCOMPLETE,
                   "$0 of the $1 elements for SetOpacityAction not found.",
                   uuids_in.size() - uuids_.size(), uuids_in.size());
}

Status SetOpacityAction::UndoImpl() {
  INK_RETURN_UNLESS(storage_->SetOpacities(uuids_, from_opacities_));
  state_ = State::kUndone;
  NotifyHost(uuids_, from_opacities_, HostSource());
  return OkStatus();
}

Status SetOpacityAction::RedoImpl() {
  INK_RETURN_UNLESS(storage_->SetOpacities(uuids_, to_opacities_));
  state_ = State::kApplied;
  NotifyHost(uuids_, to_opacities_, HostSource());
  return OkStatus();
}

void SetOpacityAction::WriteFieldsToProto(
    ink::proto::StorageAction* proto) const {
  auto* action = proto->mutable_set_opacity_action();

  for (int i = 0; i < uuids_.size(); ++i) {
    action->add_uuid(uuids_[i]);
    action->add_from_opacity(from_opacities_[i]);
    action->add_to_opacity(to_opacities_[i]);
  }
}

void SetOpacityAction::RestoreFieldsFromProto(
    const ink::proto::StorageAction& proto) {
  uuids_.clear();
  to_opacities_.clear();
  from_opacities_.clear();

  auto& action = proto.set_opacity_action();
  for (int i = 0; i < action.uuid_size(); ++i) {
    uuids_.emplace_back(action.uuid(i));
    to_opacities_.emplace_back(action.to_opacity(i));
    from_opacities_.emplace_back(action.from_opacity(i));
  }
}

void SetOpacityAction::NotifyHost(const std::vector<UUID>& uuids,
                                  const std::vector<int32>& opacities,
                                  const proto::SourceDetails& source) {
  proto::mutations::Mutation mutation;
  proto::ElementOpacityMutations element_mutations;
  for (int i = 0; i < uuids.size(); ++i) {
    auto* set_opacity = mutation.add_chunk()->mutable_set_opacity();
    set_opacity->set_uuid(uuids[i]);
    set_opacity->set_opacity(opacities[i]);

    auto* opacity_mutation = element_mutations.add_mutation();
    opacity_mutation->set_uuid(uuids[i]);
    opacity_mutation->set_opacity(opacities[i]);
  }

  element_dispatch_->Send(&IElementListener::ElementsOpacityMutated,
                          element_mutations, source);
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Status ChangeZOrderAction::Apply(const std::vector<UUID>& uuids_in,
                                 const std::vector<UUID>& below_uuids_in,
                                 const proto::SourceDetails& source) {
  EXPECT(uuids_in.size() == below_uuids_in.size());

  uuids_.reserve(uuids_in.size());
  to_below_uuids_.reserve(below_uuids_in.size());

  for (int i = 0; i < uuids_in.size(); ++i) {
    if (!storage_->IsAlive(uuids_in[i]) ||
        (below_uuids_in[i] != kInvalidUUID &&
         !storage_->IsAlive(below_uuids_in[i]))) {
      continue;
    }
    uuids_.emplace_back(uuids_in[i]);
    to_below_uuids_.emplace_back(below_uuids_in[i]);
  }

  if (uuids_.empty()) {
    return ErrorStatus(
        StatusCode::NOT_FOUND,
        "ChangeZOrderAction failed. No elements found to transform.");
  }

  std::vector<UUID> old_below_uuids;
  INK_RETURN_UNLESS(
      storage_->ChangeZOrders(uuids_, to_below_uuids_, &old_below_uuids));
  state_ = State::kApplied;

  // Undo actions have to be applied in reverse, so we store the from_uuids
  // already reversed.
  from_below_uuids_reversed_.clear();
  from_below_uuids_reversed_.reserve(old_below_uuids.size());
  copy(old_below_uuids.crbegin(), old_below_uuids.crend(),
       back_inserter(from_below_uuids_reversed_));

  NotifyHost(uuids_, to_below_uuids_, source);
  return uuids_in.size() == uuids_.size()
             ? OkStatus()
             : ErrorStatus(
                   StatusCode::INCOMPLETE,
                   "$0 of the $1 elements for ChangeZOrderAction not found.",
                   uuids_in.size() - uuids_.size(), uuids_in.size());
}

std::vector<UUID> ChangeZOrderAction::AffectedUUIDs() const {
  // The affected UUIDs is the union of all of uuids_, to_below_uuids_, and
  // from_below_uuids_reversed_.
  std::unordered_set<UUID> affected_uuids(uuids_.begin(), uuids_.end());
  copy(to_below_uuids_.begin(), to_below_uuids_.end(),
       std::inserter(affected_uuids, std::next(affected_uuids.begin())));
  copy(from_below_uuids_reversed_.begin(), from_below_uuids_reversed_.end(),
       std::inserter(affected_uuids, std::next(affected_uuids.begin())));
  return std::vector<UUID>(affected_uuids.begin(), affected_uuids.end());
}

Status ChangeZOrderAction::UndoImpl() {
  // We have to apply the undo actions in reverse.
  // (The from_uuids are already reversed.)
  std::vector<UUID> ruuids;
  copy(uuids_.crbegin(), uuids_.crend(), back_inserter(ruuids));

  INK_RETURN_UNLESS(
      storage_->ChangeZOrders(ruuids, from_below_uuids_reversed_, nullptr));
  state_ = State::kUndone;
  NotifyHost(ruuids, from_below_uuids_reversed_, HostSource());
  return OkStatus();
}

Status ChangeZOrderAction::RedoImpl() {
  INK_RETURN_UNLESS(storage_->ChangeZOrders(uuids_, to_below_uuids_, nullptr));
  state_ = State::kApplied;
  NotifyHost(uuids_, to_below_uuids_, HostSource());
  return OkStatus();
}

void ChangeZOrderAction::WriteFieldsToProto(
    ink::proto::StorageAction* proto) const {
  auto* action = proto->mutable_change_z_order_action();

  for (int i = 0; i < uuids_.size(); ++i) {
    action->add_uuid(uuids_[i]);
    action->add_from_below_uuid(from_below_uuids_reversed_[i]);
    action->add_to_below_uuid(to_below_uuids_[i]);
  }
}

void ChangeZOrderAction::RestoreFieldsFromProto(
    const ink::proto::StorageAction& proto) {
  uuids_.clear();
  from_below_uuids_reversed_.clear();
  to_below_uuids_.clear();

  auto& action = proto.change_z_order_action();
  for (int i = 0; i < action.uuid_size(); ++i) {
    uuids_.emplace_back(action.uuid(i));
    from_below_uuids_reversed_.emplace_back(action.from_below_uuid(i));
    to_below_uuids_.emplace_back(action.to_below_uuid(i));
  }
}

void ChangeZOrderAction::NotifyHost(const std::vector<UUID>& uuids,
                                    const std::vector<UUID>& below_uuids,
                                    const proto::SourceDetails& source) {
  proto::mutations::Mutation mutation;
  proto::ElementZOrderMutations element_mutations;
  for (int i = 0; i < uuids.size(); ++i) {
    auto* change_z_order = mutation.add_chunk()->mutable_change_z_order();
    change_z_order->set_uuid(uuids[i]);
    change_z_order->set_below_uuid(below_uuids[i]);

    auto* z_order_mutation = element_mutations.add_mutation();
    z_order_mutation->set_uuid(uuids[i]);
    z_order_mutation->set_below_uuid(below_uuids[i]);
  }

  element_dispatch_->Send(&IElementListener::ElementsZOrderMutated,
                          element_mutations, source);
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

}  // namespace ink
