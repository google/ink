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

// A PassthroughDocument is a conduit for messages to pass between the host and
// the scene graph. It works by doing nothing, but returning true,  for each of
// the mutation Impl functions, in the presumption that storage has already
// taken place in the host (which means, in practice, via a Brix model
// mutation).
#ifndef INK_PUBLIC_DOCUMENT_PASSTHROUGH_DOCUMENT_H_
#define INK_PUBLIC_DOCUMENT_PASSTHROUGH_DOCUMENT_H_

#include <string>

#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/public/document/document.h"

namespace ink {

class PassthroughDocument : public Document {
 public:
  PassthroughDocument() {}

  std::string ToString() const override { return "<PassthroughDocument>"; };

  proto::PageProperties GetPageProperties() const override {
    return page_properties_;
  }

 protected:
  Status AddPageImpl(const ink::proto::PerPageProperties& page) override {
    return OkStatus();
  }
  Status ClearPagesImpl() override { return OkStatus(); }
  Status AddBelowImpl(const std::vector<ink::proto::ElementBundle>& elements,
                      const UUID& below_element_with_uuid,
                      const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status RemoveImpl(const std::vector<UUID>& uuid,
                    const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status RemoveAllImpl(ink::proto::ElementIdList* removed,
                       const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status ReplaceImpl(const std::vector<proto::ElementBundle>& elements_to_add,
                     const std::vector<UUID>& uuids_to_add_below,
                     const std::vector<UUID>& uuids_to_remove,
                     const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status SetElementTransformsImpl(
      const std::vector<UUID> uuids,
      const std::vector<proto::AffineTransform> transforms,
      const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status SetElementVisibilityImpl(
      const std::vector<UUID> uuids, const std::vector<bool> visibilities,
      const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status SetElementOpacityImpl(
      const std::vector<UUID> uuids, const std::vector<int32> opacities,
      const proto::SourceDetails& source_details) override {
    return OkStatus();
  }

  Status ChangeZOrderImpl(const std::vector<UUID> uuids,
                          const std::vector<UUID> below_uuids,
                          const proto::SourceDetails& source_details) override {
    return OkStatus();
  }
  Status ActiveLayerChangedImpl(const UUID& uuid,
                                const proto::SourceDetails& source_details) {
    return OkStatus();
  }
  Status SetPagePropertiesImpl(
      const proto::PageProperties& page_properties,
      const proto::SourceDetails& source_details) override {
    page_properties_ = page_properties;
    return OkStatus();
  }
  Status UndoableSetPageBoundsImpl(
      const ink::proto::Rect& bounds,
      const proto::SourceDetails& source_details) override {
    *page_properties_.mutable_bounds() = bounds;
    return OkStatus();
  }

  bool IsEmpty() override {
    // We cannot accurately track this, so just report non-empty. It's safer.
    return false;
  }

 private:
  proto::PageProperties page_properties_;
};

};  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_PASSTHROUGH_DOCUMENT_H_
