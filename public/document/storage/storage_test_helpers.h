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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_TEST_HELPERS_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_TEST_HELPERS_H_

#include "ink/engine/public/types/uuid.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/document/storage/document_storage.h"

namespace ink {

proto::ElementBundle CreateBundle(UUID id);
proto::ElementBundle CreateBundle(UUID id,
                                  BundleDataAttachments data_attachments);
proto::AffineTransform CreateTransform(int ty);
proto::ElementBundle CreateGroup(UUID id);

// Copies the ElementBundle with only the fields filled in specified in
// data_attachments.
proto::ElementBundle CopyBundleWithAttachments(
    const proto::ElementBundle& bundle, BundleDataAttachments data_attachments);
std::vector<proto::ElementBundle> CopyBundlesWithAttachments(
    const std::vector<proto::ElementBundle>& bundles,
    BundleDataAttachments data_attachments);

bool StorageHasLiveBundle(DocumentStorage* store,
                          proto::ElementBundle expected_bundle);
bool StorageHasBundle(
    DocumentStorage* store, proto::ElementBundle expected_bundle,
    LivenessFilter liveness_filter = LivenessFilter::kDeadOrAlive);

// returns the number of live elements in the store
size_t StorageSize(DocumentStorage* store);

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_STORAGE_TEST_HELPERS_H_
