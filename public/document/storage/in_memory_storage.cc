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

#include "ink/public/document/storage/in_memory_storage.h"

#include <algorithm>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/range.h"
#include "ink/public/fingerprint/fingerprint.h"

namespace ink {

InMemoryStorage::InMemoryStorage() {}

bool InMemoryStorage::IsKnownId(const UUID& id) const {
  bool res = uuids_.Contains(id);
  ASSERT(res == (uuid_to_bundle_.find(id) != uuid_to_bundle_.end()));
  ASSERT(res == (uuid_to_liveness_.find(id) != uuid_to_liveness_.end()));
  return res;
}

bool InMemoryStorage::IsAlive(const UUID& uuid) const {
  return IsKnownId(uuid) && (uuid_to_liveness_.at(uuid) == Liveness::kAlive);
}

bool InMemoryStorage::IsEmpty() const {
  return std::none_of(uuid_to_liveness_.begin(), uuid_to_liveness_.end(),
                      [](const std::pair<UUID, Liveness>& kv) {
                        return kv.second == Liveness::kAlive;
                      });
}

Status InMemoryStorage::GetBundle(const UUID& id,
                                  BundleDataAttachments data_attachments,
                                  proto::ElementBundle* result) const {
  if (!IsKnownId(id)) {
    return ErrorStatus(StatusCode::NOT_FOUND,
                       "not attaching bundle for id $0, id not found", id);
  }

  result->set_uuid(id);
  const auto& bundle = uuid_to_bundle_.at(id);

  bool missing_transform = false;
  if (bundle.has_transform()) {
    *result->mutable_transform() = bundle.transform();
  } else {
    missing_transform = data_attachments.attach_transform;
  }

  bool missing_element = false;
  if (bundle.has_element()) {
    *result->mutable_element() = bundle.element();
  } else {
    missing_element = data_attachments.attach_element;
  }

  bool missing_outline = false;
  if (bundle.has_uncompressed_element()) {
    *result->mutable_uncompressed_element() = bundle.uncompressed_element();
  } else {
    missing_outline = data_attachments.attach_outline;
  }

  if (bundle.has_group_uuid()) {
    result->set_group_uuid(bundle.group_uuid());
  }

  if (bundle.has_visibility()) {
    result->set_visibility(bundle.visibility());
  }

  if (bundle.has_opacity()) {
    result->set_opacity(bundle.opacity());
  }

  if (!(missing_element || missing_outline || missing_transform)) {
    return OkStatus();
  }

  std::string msg = Substitute("while getting bundle for id $0:", id);
  if (missing_element) {
    msg.append("\n  missing compressed mesh");
  }
  if (missing_outline) {
    msg.append("\n  missing outline");
  }
  if (missing_transform) {
    msg.append("\n  missing transform");
  }
  return ErrorStatus(StatusCode::INCOMPLETE, msg);
}

S_WARN_UNUSED_RESULT Status
InMemoryStorage::AddPage(const ink::proto::PerPageProperties& page) {
  pages_.push_back(page);
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::ClearPages() {
  pages_.clear();
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::AddImpl(
    const std::vector<const ink::proto::ElementBundle*>& bundles,
    const std::vector<UUID>& add_below_uuids) {
  // We should either have a single add_below_uuid (in the case of adding
  // multiple elements in the same place), or the same number as the number of
  // elements (in the case of adding elements in arbitrary locations). This
  // should have already been validated in DocumentStorage::Add().
  ASSERT(add_below_uuids.size() == 1 ||
         add_below_uuids.size() == bundles.size());

  // check that all bundle ids are not already stored
  bool already_exists = false;
  for (const auto& bundle : bundles) {
    if (IsKnownId(bundle->uuid())) {
      if (uuid_to_liveness_[bundle->uuid()] == Liveness::kAlive) {
        already_exists = true;
        continue;
      }
      ASSERT(bundle->element().SerializeAsString() ==
             uuid_to_bundle_[bundle->uuid()].element().SerializeAsString());
    }
  }

  // check that below_element_with_uuid refers to a known id
  for (const auto& add_below_uuid : add_below_uuids) {
    if (add_below_uuid != kInvalidUUID && !uuids_.Contains(add_below_uuid)) {
      return ErrorStatus(StatusCode::FAILED_PRECONDITION,
                         "cannot add below unknown id $0", add_below_uuid);
    }
  }

  // add
  for (size_t i = 0; i < bundles.size(); i++) {
    const auto& bundle = *bundles[i];
    const auto& add_below_uuid =
        add_below_uuids[add_below_uuids.size() == 1 ? 0 : i];

    if (IsKnownId(bundle.uuid())) {
      uuid_to_liveness_[bundle.uuid()] = Liveness::kAlive;
      uuids_.Remove(bundle.uuid());  // Re-added at correct z-index below.
    } else {
      uuid_to_bundle_.emplace(bundle.uuid(), bundle);
      uuid_to_liveness_.emplace(bundle.uuid(), Liveness::kAlive);
    }
    if (add_below_uuid == kInvalidUUID) {
      uuids_.AddToTop(bundle.uuid());
    } else {
      uuids_.AddBelow(bundle.uuid(), add_below_uuid);
    }
  }

  return already_exists ? ErrorStatus(StatusCode::ALREADY_EXISTS,
                                      "one or more elements already exist")
                        : OkStatus();
}

S_WARN_UNUSED_RESULT Status
InMemoryStorage::RemoveImpl(const std::vector<const UUID*>& uuids) {
  for (const auto& id : uuids) {
    uuids_.Remove(*id);
    uuid_to_bundle_.erase(*id);
    uuid_to_liveness_.erase(*id);
  }
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::SetLivenessImpl(
    const std::vector<const UUID*>& uuids, Liveness liveness) {
  for (const auto& id : uuids) {
    if (!IsKnownId(*id)) {
      SLOG(SLOG_WARNING, "cannot set liveness for unknown id $0", *id);
      continue;
    }
    uuid_to_liveness_[*id] = liveness;
  }
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::SetTransformsImpl(
    const std::vector<const UUID*>& uuids,
    const std::vector<const proto::AffineTransform*>& transforms) {
  size_t num_successes = 0;
  for (size_t i = 0; i < uuids.size(); i++) {
    const auto& id = *uuids[i];
    const auto& transform = *transforms[i];
    if (!IsKnownId(id)) {
      SLOG(SLOG_WARNING, "cannot set transform for unknown id $0", id);
      continue;
    }
    *uuid_to_bundle_[id].mutable_transform() = transform;
    num_successes++;
  }
  if (num_successes < uuids.size()) {
    return ErrorStatus(
        num_successes == 0 ? StatusCode::NOT_FOUND : StatusCode::INCOMPLETE,
        "Failed to set all transforms. $0 of $1 successful.", num_successes,
        uuids.size());
  }
  return OkStatus();
}

Status InMemoryStorage::GetBundles(
    const std::vector<UUID>& uuids, BundleDataAttachments data_attachments,
    LivenessFilter liveness_filter,
    std::vector<proto::ElementBundle>* result) const {
  for (const auto& id : uuids) {
    ASSERT(IsKnownId(id));
    auto liveness = uuid_to_liveness_.at(id);
    bool liveness_filter_passes = false;
    switch (liveness_filter) {
      case LivenessFilter::kOnlyAlive:
        liveness_filter_passes = liveness == Liveness::kAlive;
        break;
      case LivenessFilter::kOnlyDead:
        liveness_filter_passes = liveness == Liveness::kDead;
        break;
      case LivenessFilter::kDeadOrAlive:
        liveness_filter_passes = true;
        break;
    }
    if (!liveness_filter_passes) continue;

    proto::ElementBundle bundle;
    if (GetBundle(id, data_attachments, &bundle)) {
      result->emplace_back(std::move(bundle));
    }
  }
  return OkStatus();
}

Status InMemoryStorage::GetBundlesImpl(
    const std::vector<const UUID*>& uuids,
    BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
    std::vector<proto::ElementBundle>* result) const {
  result->clear();

  std::vector<UUID> sorted_ids;
  sorted_ids.reserve(uuids.size());
  for (const auto& id : uuids) {
    if (IsKnownId(*id)) {
      sorted_ids.emplace_back(*id);
    }
  }
  uuids_.Sort(sorted_ids.begin(), sorted_ids.end());

  return GetBundles(sorted_ids, data_attachments, liveness_filter, result);
}

Status InMemoryStorage::GetAllBundles(
    BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
    std::vector<proto::ElementBundle>* result) const {
  result->clear();
  return GetBundles(uuids_.SortedElements().AsValueVector<UUID>(),
                    data_attachments, liveness_filter, result);
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::RemoveDeadElementsImpl(
    const std::vector<const UUID*>& keep_alive) {
  std::unordered_set<UUID> keep_alive_set;
  for (const auto& id : keep_alive) {
    keep_alive_set.emplace(*id);
  }

  std::unordered_set<UUID> dead_elements;
  for (const auto& ai : uuid_to_liveness_) {
    if (ai.second == Liveness::kDead && keep_alive_set.count(ai.first) == 0) {
      dead_elements.emplace(ai.first);
    }
  }

  return Remove(MakeSTLRange(dead_elements));
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::SetPageProperties(
    const proto::PageProperties& page_properties) {
  page_properties_ = page_properties;
  return OkStatus();
}

proto::PageProperties InMemoryStorage::GetPageProperties() const {
  return page_properties_;
}

Status InMemoryStorage::SetActiveLayer(const UUID& uuid) {
  active_layer_ = uuid;
  return OkStatus();
}

UUID InMemoryStorage::GetActiveLayer() const { return active_layer_; }

void InMemoryStorage::WriteToProto(ink::proto::Snapshot* proto,
                                   SnapshotQuery q) const {
  *(proto->mutable_page_properties()) = page_properties_;
  for (const auto& page : pages_) {
    *proto->add_per_page_properties() = page;
  }
  if (active_layer_ != kInvalidUUID) {
    proto->set_active_layer_uuid(active_layer_);
  }
  Fingerprinter fingerprinter;
  for (const auto& id : uuids_.SortedElements().AsValueVector<UUID>()) {
    ASSERT(IsKnownId(id));
    proto::ElementBundle* bundleProto;
    bool is_alive = uuid_to_liveness_.at(id) == Liveness::kAlive;
    if (is_alive) {
      bundleProto = proto->add_element();
      proto->add_element_state_index(ink::proto::ElementState::ALIVE);
    } else if (q == INCLUDE_DEAD_ELEMENTS) {
      bundleProto = proto->add_dead_element();
      proto->add_element_state_index(ink::proto::ElementState::DEAD);
    } else {
      continue;
    }
    const auto& bundle = uuid_to_bundle_.at(id);
    *bundleProto = bundle;
    bundleProto->set_uuid(id);
    if (is_alive) {
      fingerprinter.Note(*bundleProto);
    }
  }
  proto->set_fingerprint(fingerprinter.GetFingerprint());
}

Status InMemoryStorage::ReadFromProto(const ink::proto::Snapshot& proto) {
  uuids_.Clear();
  uuid_to_bundle_.clear();
  uuid_to_liveness_.clear();
  page_properties_ = proto.page_properties();
  INK_RETURN_UNLESS(ClearPages());
  for (auto& page : proto.per_page_properties()) {
    INK_RETURN_UNLESS(AddPage(page));
  }

  active_layer_ = kInvalidUUID;
  if (proto.has_active_layer_uuid()) {
    active_layer_ = proto.active_layer_uuid();
  }

  const bool has_state_index = proto.element_state_index_size() > 0;
  const auto& state_index = proto.element_state_index();
  const int expected_dead_element_count = std::count_if(
      state_index.begin(), state_index.end(),
      [](int state) { return state == ink::proto::ElementState::DEAD; });
  const int expected_live_element_count = std::count_if(
      state_index.begin(), state_index.end(),
      [](int state) { return state == ink::proto::ElementState::ALIVE; });

  if (has_state_index &&
      expected_dead_element_count != proto.dead_element_size()) {
    // This is a WARNING, because it reflects the state created due to a bug
    // prior to the CL that created this comment. The state is recoverable and
    // consistent, so not an ERROR. (See  b/111655675.)
    SLOG(SLOG_WARNING,
         "Index refers to $0 dead elements, but $1 are present. Ignoring "
         "index.",
         expected_dead_element_count, proto.dead_element_size());
  }
  if (has_state_index && expected_live_element_count != proto.element_size()) {
    // This state is not known to exist, and we need to know about it.
    SLOG(SLOG_ERROR,
         "Index refers to $0 live elements, but $1 are present. Ignoring "
         "index.",
         expected_live_element_count, proto.element_size());
  }
  if (has_state_index &&
      expected_dead_element_count == proto.dead_element_size() &&
      expected_live_element_count == proto.element_size()) {
    int alive_index = 0;
    int dead_index = 0;
    for (const auto& state : proto.element_state_index()) {
      if (state == ink::proto::ElementState::ALIVE) {
        const auto& element = proto.element(alive_index++);
        SLOG(SLOG_DOCUMENT, "Adding live element $0", element.uuid());
        INK_RETURN_UNLESS(Add(MakePointerRange(&element), kInvalidUUID));
      } else if (state == ink::proto::ElementState::DEAD) {
        const auto& element = proto.dead_element(dead_index++);
        SLOG(SLOG_DOCUMENT, "Adding dead element $0", element.uuid());
        uuid_to_bundle_.emplace(element.uuid(), element);
        uuid_to_liveness_.emplace(element.uuid(), Liveness::kDead);
        uuids_.AddToTop(element.uuid());
      } else {
        return ErrorStatus(StatusCode::INTERNAL,
                           "Encountered unknown liveness state $0.", state);
      }
    }
  } else {
    for (const auto& element : proto.element()) {
      SLOG(SLOG_DOCUMENT, "Adding live element $0", element.uuid());
      INK_RETURN_UNLESS(Add(MakePointerRange(&element), kInvalidUUID));
    }
  }
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::SetVisibilities(
    const std::vector<UUID>& uuids, const std::vector<bool>& visibilities) {
  EXPECT(uuids.size() == visibilities.size());
  int num_successes = 0;
  for (int i = 0; i < uuids.size(); ++i) {
    const auto& uuid = uuids[i];
    if (!IsKnownId(uuid)) {
      SLOG(SLOG_WARNING, "cannot set visibility for unknown id $0", uuid);
      continue;
    }
    uuid_to_bundle_[uuid].set_visibility(visibilities[i]);
    num_successes++;
  }
  if (num_successes < uuids.size()) {
    return ErrorStatus(
        num_successes == 0 ? StatusCode::NOT_FOUND : StatusCode::INCOMPLETE,
        "Failed to set all visibilities. $0 of $1 successful.", num_successes,
        uuids.size());
  }
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::SetOpacities(
    const std::vector<UUID>& uuids, const std::vector<int>& opacities) {
  EXPECT(uuids.size() == opacities.size());
  int num_successes = 0;
  for (int i = 0; i < uuids.size(); ++i) {
    const auto& uuid = uuids[i];
    if (!IsKnownId(uuid)) {
      SLOG(SLOG_WARNING, "cannot set opacity for unknown id $0", uuid);
      continue;
    }
    uuid_to_bundle_[uuid].set_opacity(util::Clamp(0, 255, opacities[i]));
    num_successes++;
  }
  if (num_successes < uuids.size()) {
    return ErrorStatus(
        num_successes == 0 ? StatusCode::NOT_FOUND : StatusCode::INCOMPLETE,
        "Failed to set all opacities. $0 of $1 successful.", num_successes,
        uuids.size());
  }
  return OkStatus();
}

S_WARN_UNUSED_RESULT Status InMemoryStorage::ChangeZOrders(
    const std::vector<UUID>& uuids, const std::vector<UUID>& below_uuids,
    std::vector<UUID>* old_below_uuids) {
  EXPECT(uuids.size() == below_uuids.size());

  int num_successes = 0;
  for (int i = 0; i < uuids.size(); ++i) {
    const auto& uuid = uuids[i];
    if (!IsKnownId(uuid)) {
      SLOG(SLOG_WARNING, "cannot set z-order for unknown uuid, $0", uuid);
      continue;
    }
    const auto& below_uuid = below_uuids[i];
    if (below_uuid != kInvalidUUID && !IsKnownId(below_uuid)) {
      SLOG(SLOG_WARNING, "cannot set z-order below unknown uuid: $0",
           below_uuid);
      continue;
    }

    // NOTE: this does exactly what ElementIndex warns us not to do: we are
    // alternating reads and writes.
    UUID old_below_id;
    Status st = FindBundleAboveUUID(uuid, &old_below_id);
    if (!st) {
      SLOG(SLOG_WARNING, "cannot find old z-order for $0: $1", uuid, st);
      continue;
    }

    uuids_.Remove(uuid);
    if (below_uuid == kInvalidUUID) {
      uuids_.AddToTop(uuid);
    } else {
      uuids_.AddBelow(uuid, below_uuid);
    }
    if (old_below_uuids) {
      old_below_uuids->emplace_back(old_below_id);
    }
    num_successes++;
  }
  if (num_successes < uuids.size()) {
    return ErrorStatus(
        num_successes == 0 ? StatusCode::NOT_FOUND : StatusCode::INCOMPLETE,
        "Failed to change z-orders. $0 of $1 successful.", num_successes,
        uuids.size());
  }
  return OkStatus();
}

Status InMemoryStorage::FindBundleAboveUUID(const UUID& uuid,
                                            UUID* above_uuid) {
  if (!IsKnownId(uuid)) {
    return ErrorStatus(StatusCode::NOT_FOUND, "Unknown UUID: $0", uuid);
  }
  auto old_z_index = uuids_.ZIndexOf(uuid);
  auto elsrng = uuids_.SortedElements();
  auto next_itr = elsrng.begin() + old_z_index + 1;
  *above_uuid = next_itr == elsrng.end() ? kInvalidUUID : *next_itr;

  return OkStatus();
}

}  // namespace ink
