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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_IN_MEMORY_STORAGE_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_IN_MEMORY_STORAGE_H_

#include <string>
#include <unordered_map>

#include "ink/engine/public/types/status.h"
#include "ink/engine/scene/types/element_index.h"
#include "ink/engine/util/security.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/document/storage/document_storage.h"

namespace ink {

class InMemoryStorage : public DocumentStorage {
 public:
  InMemoryStorage();

  std::string ToString() const override { return "<InMemoryStorage>"; }

  bool SupportsSnapshot() const override { return true; }
  void WriteToProto(ink::proto::Snapshot* proto,
                    SnapshotQuery q) const override;
  Status ReadFromProto(const ink::proto::Snapshot& proto) override;

 protected:
  S_WARN_UNUSED_RESULT Status
  AddPage(const ink::proto::PerPageProperties& page) override;
  S_WARN_UNUSED_RESULT Status ClearPages() override;
  S_WARN_UNUSED_RESULT Status
  AddImpl(const std::vector<const ink::proto::ElementBundle*>& bundles,
          const std::vector<UUID>& add_below_uuids) override;
  S_WARN_UNUSED_RESULT Status
  RemoveImpl(const std::vector<const UUID*>& uuids) override;
  S_WARN_UNUSED_RESULT Status SetLivenessImpl(
      const std::vector<const UUID*>& uuids, Liveness liveness) override;
  S_WARN_UNUSED_RESULT Status SetTransformsImpl(
      const std::vector<const UUID*>& uuids,
      const std::vector<const proto::AffineTransform*>& transforms) override;
  S_WARN_UNUSED_RESULT Status GetBundlesImpl(
      const std::vector<const UUID*>& uuids,
      BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
      std::vector<proto::ElementBundle>* result) const override;
  S_WARN_UNUSED_RESULT Status GetAllBundles(
      BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
      std::vector<proto::ElementBundle>* result) const override;
  S_WARN_UNUSED_RESULT Status
  RemoveDeadElementsImpl(const std::vector<const UUID*>& keep_alive) override;

  S_WARN_UNUSED_RESULT bool IsAlive(const UUID& uuid) const override;
  S_WARN_UNUSED_RESULT bool IsEmpty() const override;

  S_WARN_UNUSED_RESULT Status
  SetPageProperties(const proto::PageProperties& page_properties) override;
  proto::PageProperties GetPageProperties() const override;

  S_WARN_UNUSED_RESULT Status
  SetVisibilities(const std::vector<UUID>& uuids,
                  const std::vector<bool>& visibilities) override;
  S_WARN_UNUSED_RESULT Status
  SetOpacities(const std::vector<UUID>& uuids,
               const std::vector<int>& opacities) override;
  S_WARN_UNUSED_RESULT Status ChangeZOrders(
      const std::vector<UUID>& uuids, const std::vector<UUID>& below_uuids,
      std::vector<UUID>* old_below_uuids) override;
  S_WARN_UNUSED_RESULT Status FindBundleAboveUUID(const UUID& uuid,
                                                  UUID* above_uuid) override;

  S_WARN_UNUSED_RESULT Status SetActiveLayer(const UUID& uuid) override;
  S_WARN_UNUSED_RESULT UUID GetActiveLayer() const override;

 private:
  Status GetBundles(const std::vector<UUID>& uuids,
                    BundleDataAttachments data_attachments,
                    LivenessFilter liveness_filter,
                    std::vector<proto::ElementBundle>* result) const;
  Status GetBundle(const UUID& id, BundleDataAttachments data_attachments,
                   proto::ElementBundle* result) const;
  bool IsKnownId(const UUID& id) const;

 private:
  ElementIndex<UUID> uuids_;
  std::unordered_map<UUID, proto::ElementBundle> uuid_to_bundle_;
  std::unordered_map<UUID, Liveness> uuid_to_liveness_;
  proto::PageProperties page_properties_;
  std::vector<ink::proto::PerPageProperties> pages_;

  UUID active_layer_ = kInvalidUUID;
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_IN_MEMORY_STORAGE_H_
