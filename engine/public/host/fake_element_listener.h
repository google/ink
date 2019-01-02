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

#ifndef INK_ENGINE_PUBLIC_HOST_FAKE_ELEMENT_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_FAKE_ELEMENT_LISTENER_H_

#include "ink/engine/public/host/ielement_listener.h"

#include "ink/engine/public/proto_traits.h"

namespace ink {

class FakeElementListener : public IElementListener {
 public:
  void ElementsAdded(const proto::ElementBundleAdds& unsafe_adds,
                     const proto::SourceDetails& sourceDetails) override {
    for (int i = 0; i < unsafe_adds.element_bundle_add_size(); ++i) {
      added_.push_back(std::make_pair(
          unsafe_adds.element_bundle_add(i).element_bundle().uuid(),
          unsafe_adds.element_bundle_add(i).below_uuid()));
    }
  }
  void ElementsRemoved(const proto::ElementIdList& removedIds,
                       const proto::SourceDetails& sourceDetails) override {
    for (int i = 0; i < removedIds.uuid_size(); ++i) {
      removed_.push_back(removedIds.uuid(i));
    }
  }
  void ElementsReplaced(const proto::ElementBundleReplace& replace,
                        const proto::SourceDetails& source_details) override {
    for (const auto& add : replace.elements_to_add().element_bundle_add())
      added_by_replace_.emplace_back(add.element_bundle().uuid(),
                                     add.below_uuid());
    for (const auto& remove : replace.elements_to_remove().uuid())
      removed_by_replace_.emplace_back(remove);
  }

  void ElementsTransformMutated(
      const proto::ElementTransformMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {
    transform_mutations_.push_back(unsafe_mutations);
  }

  void ElementsVisibilityMutated(
      const proto::ElementVisibilityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {
    visibility_mutations_.push_back(unsafe_mutations);
  }

  void ElementsOpacityMutated(
      const proto::ElementOpacityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {
    opacity_mutations_.push_back(unsafe_mutations);
  }

  void ElementsZOrderMutated(
      const proto::ElementZOrderMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override {
    z_order_mutations_.push_back(unsafe_mutations);
  }

  void Clear() {
    added_.clear();
    removed_.clear();
    added_by_replace_.clear();
    removed_by_replace_.clear();

    transform_mutations_.clear();
    visibility_mutations_.clear();
    opacity_mutations_.clear();
    z_order_mutations_.clear();
  }

  std::vector<std::pair<UUID, UUID>> ElementsAdded() const { return added_; }
  std::vector<UUID> ElementsRemoved() const { return removed_; }
  std::vector<std::pair<UUID, UUID>> ElementsAddedByReplace() const {
    return added_by_replace_;
  }
  std::vector<UUID> ElementsRemovedByReplace() const {
    return removed_by_replace_;
  }
  std::vector<proto::ElementTransformMutations> ElementsTransformMutated()
      const {
    return transform_mutations_;
  }
  std::vector<proto::ElementVisibilityMutations> ElementsVisibilityMutated()
      const {
    return visibility_mutations_;
  }
  std::vector<proto::ElementOpacityMutations> ElementsOpacityMutated() const {
    return opacity_mutations_;
  }
  std::vector<proto::ElementZOrderMutations> ElementsZOrderMutated() const {
    return z_order_mutations_;
  }

 private:
  // UUID of added element, UUID of element is should be added beneath
  std::vector<std::pair<UUID, UUID>> added_;
  std::vector<UUID> removed_;
  std::vector<std::pair<UUID, UUID>> added_by_replace_;
  std::vector<UUID> removed_by_replace_;
  std::vector<proto::ElementTransformMutations> transform_mutations_;
  std::vector<proto::ElementVisibilityMutations> visibility_mutations_;
  std::vector<proto::ElementOpacityMutations> opacity_mutations_;
  std::vector<proto::ElementZOrderMutations> z_order_mutations_;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_FAKE_ELEMENT_LISTENER_H_
