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

#include "ink/public/document/storage/storage_test_helpers.h"

#include "testing/base/public/gmock.h"
#include "third_party/absl/strings/substitute.h"

using ink::proto::AffineTransform;
using ink::proto::ElementBundle;
using testing::EquivToProto;

namespace ink {

AffineTransform CreateTransform(int ty) {
  AffineTransform t;
  t.set_ty(ty);
  return t;
}

ElementBundle CreateBundle(UUID id, BundleDataAttachments data_attachments) {
  ElementBundle e;
  e.set_uuid(id);
  if (data_attachments.attach_element) {
    e.mutable_element()->set_deprecated_uuid(id);
  }
  if (data_attachments.attach_outline) {
    proto::UncompressedStroke us;
    us.set_rgba(1234);
    proto::UncompressedElement ue;
    *ue.mutable_uncompressed_stroke() = us;
    *e.mutable_uncompressed_element() = ue;
  }
  if (data_attachments.attach_transform) {
    // Use the last character of the UUID as an arbitrary (deterministic)
    // transform amount.
    EXPECT(!id.empty());
    *e.mutable_transform() = CreateTransform(id.back());
  }
  return e;
}

ElementBundle CreateBundle(UUID id) {
  return CreateBundle(id, BundleDataAttachments::None());
}

ElementBundle CreateGroup(UUID id) {
  auto e = CreateBundle(id);
  e.mutable_element()->mutable_attributes()->set_is_group(true);
  return e;
}

bool StorageHasLiveBundle(DocumentStorage* store,
                          proto::ElementBundle expected_bundle) {
  return StorageHasBundle(store, expected_bundle, LivenessFilter::kOnlyAlive);
}

bool StorageHasBundle(DocumentStorage* store, ElementBundle expected_bundle,
                      LivenessFilter liveness_filter) {
  std::vector<proto::ElementBundle> read_bundles;
  UUID id = expected_bundle.uuid();
  BundleDataAttachments data_attachments(
      expected_bundle.has_transform(), expected_bundle.has_element(),
      expected_bundle.has_uncompressed_element());
  EXPECT_TRUE(store->GetBundles(MakePointerRange(&id), data_attachments,
                                liveness_filter, &read_bundles));
  if (read_bundles.empty()) return false;

  EXPECT_EQ(1u, read_bundles.size());
  EXPECT_THAT(read_bundles[0], EquivToProto(expected_bundle));
  return true;
}

size_t StorageSize(DocumentStorage* store) {
  std::vector<proto::ElementBundle> read_bundles;
  EXPECT_TRUE(
      store->GetAllBundles({}, LivenessFilter::kOnlyAlive, &read_bundles));
  return read_bundles.size();
}

std::vector<ElementBundle> CopyBundlesWithAttachments(
    const std::vector<ElementBundle>& bundles,
    BundleDataAttachments data_attachments) {
  std::vector<ElementBundle> ans;
  for (auto& b : bundles) {
    ans.emplace_back(CopyBundleWithAttachments(b, data_attachments));
  }
  return ans;
}

ElementBundle CopyBundleWithAttachments(
    const ElementBundle& bundle, BundleDataAttachments data_attachments) {
  ElementBundle e;
  e.set_uuid(bundle.uuid());
  if (data_attachments.attach_element) {
    e.mutable_element()->set_deprecated_uuid(
        bundle.element().deprecated_uuid());
  }
  if (data_attachments.attach_outline) {
    *e.mutable_uncompressed_element() = bundle.uncompressed_element();
  }
  if (data_attachments.attach_transform) {
    *e.mutable_transform() = bundle.transform();
  }
  return e;
}

}  // namespace ink
