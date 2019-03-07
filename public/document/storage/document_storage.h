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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_DOCUMENT_STORAGE_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_DOCUMENT_STORAGE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/engine/util/range.h"
#include "ink/engine/util/security.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/document/bundle_data_attachments.h"

// Interface definitions for the document storage API

namespace ink {

// Liveness is filtering mechanism.
//
// By setting an element to not-alive instead of removing from storage it you
// can avoid duplicating a element in memory and/or paying storage read/write
// costs when implementing undo/redo.
//
// Ex An undo of an add would set liveness to false, and the corresponding
// redo could simply set liveness to true, avoiding unnecessary copies.
// (Esp important for large changes, like remove all)
enum class Liveness { kAlive, kDead };

enum class LivenessFilter { kOnlyAlive, kOnlyDead, kDeadOrAlive };

// A storage API for documents
//
// The class is designed to be subclassed. It has a templated public API that
// accepts different range types, which forwards to a virtual dispatch friendly
// internal API.
//
// DocumentStorage's goal is to be a simple interface around differing Element
// storage strategies. DocumentStorage assumes a dumb physical storage, that
// cannot change except in response to an API call through that DocumentStorage.
// (similar to a file API). This is in contrast to multiuser storage (ie brix)
// where the data may change independently at any point. If you want multiuser
// storage, look to subclass of Document instead of DocumentStorage.
// (Document has an event-based API that explicitly supports this).
//
// The relationship between Document and DocumentStorage is something like this:
//
////////////////////////////////////////////////////////////////////////////////
//                               +----------+
//                               | Document |
//                               +----^-----+
//                                    |
//                               is a |
//                                    |
//                +-------------------+------------------+
//                |                   |                  |
//    +-----------+-------------+     |         +--------+------------+
//    | INK+BrixDocument (objc) |     |         | PassthroughDocument |
//    +-------------------------+     |         +---------------------+
//                                    |
//                         +----------+---------+
//                         | SingleUserDocument |
//                         +----------+---------+
//                                    |
//                              has a |
//                                    |
//                          +---------v-------+
//                          | DocumentStorage |
//                          +-----------------+
//
////////////////////////////////////////////////////////////////////////////////
//
//          +-----------------+
//          | DocumentStorage |
//          +--------^--------+
//                   |
//              is a |
//                   |
//             +-----+
//             |
// +-----------+--+
// |   InMemory   |
// +--------------+
//
////////////////////////////////////////////////////////////////////////////////
//
// Primary features of the DocumentStorage API:
// - A single suite of tests over all storage layers
// - No-copying, and no-memory duplication necessary for impls of undo/redo.
//   See SetLiveness.
// - Supports data compaction of non-referenced elements (ex under memory
//   pressure) See RemoveDeadElements.
// - Consistency of multi-element error handling. (eg transactions)
class DocumentStorage {
 public:
  virtual ~DocumentStorage() {}

  // Add elements to storage
  //
  // Input:
  //   1) A range of ElementBundles arranged in z-order, last is topmost
  //   2) A UUID to put the range below
  //
  // Properties:
  //  - Each bundle must have a valid UUID
  //  - Each UUID must not already exist in the document
  //  - below_element_with_uuid must refer to either an existing
  //    uuid or kInvalidUUID
  //  - To put the element at the top, pass kInvalidUUID as the
  //    below_element_with_uuid.
  //  - The order of bundles is preserved. The last item in bundles will be
  //    directly below "below_element_with_uuid"
  //      Ex:
  //      1. Existing = "item1"
  //         Add({"item 2", "item4", "item3"}, below item1)
  //         Result order = item2->item4->item3->item1
  //      2. Existing = "item1"
  //         Add({"item 2", "item4", "item3"}, below kInvalidUUID)
  //         Result order = item1->item2->item4->item3
  //
  // Result:
  //   - OkStatus: all elements have been added
  //   - else: no elements have been added
  template <typename TBundleRange>
  S_WARN_UNUSED_RESULT Status
  Add(TBundleRange bundles, /* Range<proto::ElementBundle> */
      const UUID& below_element_with_uuid) {
    std::vector<UUID> add_below = {below_element_with_uuid};
    INK_RETURN_UNLESS(ValidateBundlesForAdd(bundles, add_below));
    return AddImpl(bundles.template AsPointerVector<proto::ElementBundle>(),
                   add_below);
  }

  // This overload functions similarly to the one above, save that it allows for
  // inserting the elements in arbitrary locations in the z-order, corresponding
  // to add_below_uuids. Note that the size ofthe bundle range must be the same
  // as the size of add_below_uuids.
  template <typename TBundleRange>
  S_WARN_UNUSED_RESULT Status Add(TBundleRange bundles,
                                  const std::vector<UUID>& add_below_uuids) {
    if (bundles.size() != add_below_uuids.size()) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Cannot add, size mismatch between bundle list and "
                         "add-below-id list.");
    }

    INK_RETURN_UNLESS(ValidateBundlesForAdd(bundles, add_below_uuids));
    return AddImpl(bundles.template AsPointerVector<proto::ElementBundle>(),
                   add_below_uuids);
  }

  // Remove all uuids in the range, if they exist.
  //
  // Result:
  //   - true: all known elements have been removed
  //   - false: an error occurred removing an element
  template <typename TUUIDRange>
  S_WARN_UNUSED_RESULT Status Remove(TUUIDRange uuids) {
    return RemoveImpl(uuids.template AsPointerVector<UUID>());
  }

  // Set liveness all uuids in the range, if they exist.
  //
  // Result:
  //   - true: liveness has been set for all known elements.
  //   - false: an error occurred.
  template <typename TUUIDRange>
  S_WARN_UNUSED_RESULT Status SetLiveness(TUUIDRange uuids, Liveness liveness) {
    return SetLivenessImpl(uuids.template AsPointerVector<UUID>(), liveness);
  }

  // Set transforms for all uuids in the range, if they exist.
  //
  // Result:
  //   - true: transform has been set for all known elements.
  //   - false: an error occurred.`
  template <typename TUUIDRange, typename TTransformRange>
  S_WARN_UNUSED_RESULT Status SetTransforms(TUUIDRange uuids,
                                            TTransformRange transforms) {
    ASSERT(uuids.size() == transforms.size());
    return SetTransformsImpl(
        uuids.template AsPointerVector<UUID>(),
        transforms.template AsPointerVector<ink::proto::AffineTransform>());
  }

  // Get bundles for all uuids in the range, if they exist.
  //
  // Args:
  //   - uuids: range of uuids to look for
  //   - data_attachments: Specifies which data to attach to the result bundle
  //     Elements with only partial data are not returned.
  //     Ex if attach_transform and element 3 has no transform
  //     res = true, {element1, element2}
  //   - liveness_filter: returns only data where the filter matches
  //   - result: The elements, sorted by z-index. (Last item == topmost)
  //
  // Result:
  //   - OkStatus: result contains all the found elements.
  //     NOTE SizeOf(uuids) != SizeOf(result) in some cases (an unknown uuid,
  //     or missing data)
  //   - else: an error occurred.
  template <typename TUUIDRange>
  S_WARN_UNUSED_RESULT Status
  GetBundles(TUUIDRange uuids, BundleDataAttachments data_attachments,
             LivenessFilter liveness_filter,
             std::vector<proto::ElementBundle>* result) const {
    return GetBundlesImpl(uuids.template AsPointerVector<UUID>(),
                          data_attachments, liveness_filter, result);
  }

  // Same as GetBundles over all uuids known to the storage
  virtual S_WARN_UNUSED_RESULT Status GetAllBundles(
      BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
      std::vector<proto::ElementBundle>* result) const = 0;

  // Returns true if an element with the given UUID exists and is alive.
  virtual S_WARN_UNUSED_RESULT bool IsAlive(const UUID& uuid) const = 0;

  // Remove all elements that are:
  //  - Not in "keep_alive"
  //  - Liveness == kDead
  //
  // keep_alive is useful for specifying outstanding memory references into the
  // document. (Garbage collection)
  // Ex an undo stack could depend on a dead element being availible later to
  // avoid unnecessary reads into memory, but still want to discard unreferenced
  // elements under memory pressure.
  //
  // Result:
  //   - true: all dead elements are removed
  //   - false: an error occurred.
  template <typename TUUIDRange>
  S_WARN_UNUSED_RESULT Status RemoveDeadElements(TUUIDRange keep_alive) {
    return RemoveDeadElementsImpl(keep_alive.template AsPointerVector<UUID>());
  }

  // Get transforms for all uuids in the range, if they exist.
  //
  // Result:
  //   - true: result contains the transforms for the found uuids.
  //     NOTE SizeOf(uuids) != SizeOf(result) in some cases (an unknown uuid)
  //   - false: an error occurred.
  template <typename TUUIDRange>
  S_WARN_UNUSED_RESULT Status
  GetTransforms(TUUIDRange uuids, LivenessFilter liveness_filter,
                std::unordered_map<UUID, ink::proto::AffineTransform>* result) {
    std::vector<proto::ElementBundle> read_bundles;
    INK_RETURN_UNLESS(GetBundles(uuids, {true, false, false}, liveness_filter,
                                 &read_bundles));
    result->clear();
    for (const auto& bundle : read_bundles) {
      result->emplace(bundle.uuid(), bundle.transform());
    }
    return OkStatus();
  }

  virtual S_WARN_UNUSED_RESULT Status
  SetPageProperties(const proto::PageProperties& page_properties) = 0;
  virtual proto::PageProperties GetPageProperties() const = 0;

  virtual S_WARN_UNUSED_RESULT Status SetActiveLayer(const UUID& uuid) = 0;
  virtual S_WARN_UNUSED_RESULT UUID GetActiveLayer() const = 0;

  // Experimental API for multipage support. Adds a page to the document.
  virtual S_WARN_UNUSED_RESULT Status
  AddPage(const ink::proto::PerPageProperties& page) = 0;
  // Experimental API for multipage support. Removes all pages from the
  // document.
  virtual S_WARN_UNUSED_RESULT Status ClearPages() = 0;

  enum SnapshotQuery { INCLUDE_DEAD_ELEMENTS, DO_NOT_INCLUDE_DEAD_ELEMENTS };

  virtual bool SupportsSnapshot() const { return false; }
  virtual void WriteToProto(ink::proto::Snapshot* proto,
                            SnapshotQuery q) const {
    RUNTIME_ERROR(
        "This DocumentStorage does not know how to write to a snapshot.");
  }
  virtual S_WARN_UNUSED_RESULT Status
  ReadFromProto(const ink::proto::Snapshot& proto) {
    RUNTIME_ERROR("This DocumentStorage does not know how to read a snapshot.");
  }

  virtual std::string ToString() const = 0;

  // Probe the storage to determine if it's empty.
  //
  // Result:
  //   - true: no live elements were found in the storage.
  //   - false: at least one live element was found in the storage.
  virtual bool IsEmpty() const = 0;

  virtual S_WARN_UNUSED_RESULT Status
  SetVisibilities(const std::vector<UUID>& uuids,
                  const std::vector<bool>& visibilities) = 0;
  virtual S_WARN_UNUSED_RESULT Status SetOpacities(
      const std::vector<UUID>& uuids, const std::vector<int>& opacities) = 0;

  // Returns the old "below uuids" in old_below_uuids. This is necessary in
  // order to implement Undo.
  //
  // The old below_uuids values will be emplaced_back
  // into old_below_uuids. old_below_uuids will not be cleared.
  // old_below_uuids may be nullptr.
  virtual S_WARN_UNUSED_RESULT Status ChangeZOrders(
      const std::vector<UUID>& uuids, const std::vector<UUID>& below_uuids,
      std::vector<UUID>* old_below_uuids) = 0;
  virtual S_WARN_UNUSED_RESULT Status FindBundleAboveUUID(const UUID& uuid,
                                                          UUID* above_uuid) = 0;

 protected:
  virtual S_WARN_UNUSED_RESULT Status
  AddImpl(const std::vector<const ink::proto::ElementBundle*>& bundles,
          const std::vector<UUID>& add_below_uuids) = 0;
  virtual S_WARN_UNUSED_RESULT Status
  RemoveImpl(const std::vector<const UUID*>& uuids) = 0;
  virtual S_WARN_UNUSED_RESULT Status
  SetLivenessImpl(const std::vector<const UUID*>& uuids, Liveness liveness) = 0;
  virtual S_WARN_UNUSED_RESULT Status SetTransformsImpl(
      const std::vector<const UUID*>& uuids,
      const std::vector<const ink::proto::AffineTransform*>& transforms) = 0;
  virtual S_WARN_UNUSED_RESULT Status GetBundlesImpl(
      const std::vector<const UUID*>& uuids,
      BundleDataAttachments data_attachments, LivenessFilter liveness_filter,
      std::vector<proto::ElementBundle>* result) const = 0;
  virtual S_WARN_UNUSED_RESULT Status
  RemoveDeadElementsImpl(const std::vector<const UUID*>& keep_alive) = 0;

 private:
  // Performs safety checks on the elements to be added. This must be called
  // before AddImpl(). Returns an error if any of the bundles share a UUID, or
  // if any of the add-below UUIDs belong to any of the bundles.
  // Note that the subclass is expected to check that the bundles' UUIDs don't
  // already exist in the scene, and the add-below UUIDs do exist.
  // Type parameter TBundleRange is expected to be an ink::Range that iterates
  // over proto::ElementBundles.
  // Type parameter TUuidRange is expected to be an ink::Range that iterates
  // over UUIDs.
  template <typename TBundleRange>
  S_WARN_UNUSED_RESULT Status ValidateBundlesForAdd(
      TBundleRange bundles, const std::vector<UUID>& add_below_uuids) {
    std::unordered_set<UUID> id_set;
    size_t n_bundles = 0;
    for (const auto& bundle : bundles) {
      id_set.emplace(bundle.uuid());
      n_bundles++;
    }
    if (id_set.size() != n_bundles) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Cannot add, not all ids are unique.");
    }
    for (const auto& add_below_uuid : add_below_uuids) {
      if (id_set.count(add_below_uuid) > 0) {
        return ErrorStatus(
            StatusCode::INVALID_ARGUMENT,
            "Cannot add, below_id cannot refer to an element in bundles.");
      }
    }
    return OkStatus();
  }
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_DOCUMENT_STORAGE_H_
