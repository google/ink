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

#ifndef INK_ENGINE_SCENE_UNSAFE_SCENE_HELPER_H_
#define INK_ENGINE_SCENE_UNSAFE_SCENE_HELPER_H_

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/proto_traits.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

class RootController;

// UnsafeSceneHelper propagates element changes from the document to the
// RootController, filtering those with sourceDetails == ENGINE.
// Incoming elements and mutations are considered untrusted (could be coming
// over the wire).
class UnsafeSceneHelper : public IElementListener,
                          public IPagePropertiesListener {
 public:
  void AddElement(const proto::ElementBundle& unsafe_bundle,
                  const UUID& belowUUID,
                  const proto::SourceDetails& sourceDetails);

  // IElementListener
  void ElementsAdded(const proto::ElementBundleAdds& unsafe_bundle_adds,
                     const proto::SourceDetails& sourceDetails) override;
  void ElementsRemoved(const proto::ElementIdList& removedIds,
                       const proto::SourceDetails& sourceDetails) override;
  void ElementsReplaced(const proto::ElementBundleReplace& replace,
                        const proto::SourceDetails& source_details) override;

  void ElementsTransformMutated(
      const proto::ElementTransformMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override;
  void ElementsVisibilityMutated(
      const proto::ElementVisibilityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override;
  void ElementsOpacityMutated(
      const proto::ElementOpacityMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override;
  void ElementsZOrderMutated(
      const proto::ElementZOrderMutations& unsafe_mutations,
      const proto::SourceDetails& source_details) override;

  // IPagePropertiesListener
  void PageBoundsChanged(const proto::Rect& bounds,
                         const proto::SourceDetails& sourceDetails) override;
  void BackgroundColorChanged(
      const proto::Color& color,
      const proto::SourceDetails& sourceDetails) override;
  void BackgroundImageChanged(
      const proto::BackgroundImageInfo& image,
      const proto::SourceDetails& sourceDetails) override;
  void BorderChanged(const proto::Border& border,
                     const proto::SourceDetails& sourceDetails) override;
  void GridChanged(const proto::GridInfo& grid_info,
                   const proto::SourceDetails& source_details) override;

 private:
  friend RootController;
  explicit UnsafeSceneHelper(RootController* root) : root_controller_(root) {}

  template <typename SceneValueType, typename Mutations>
  void MutateElements(
      const Mutations& unsafe_mutations,
      const proto::SourceDetails source_details,
      std::function<bool(const typename Mutations::Mutation& mutation,
                         typename std::vector<SceneValueType>::iterator)>
          read_value_func,
      std::function<void(RootController*, const std::vector<UUID>&,
                         const std::vector<SceneValueType>&,
                         const SourceDetails&)>
          root_controller_func);

  void SetBackgroundColor(const glm::vec4& color /* un-premultiplied RGBA */);
  void SetBackgroundImage(const Rect& bounds, const std::string& uri);
  void SetPageBounds(const Rect& bounds, const SourceDetails& source_details);
  void SetBorder(const std::string& uri, float scale);
  void ClearBorder();

  RootController* root_controller_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_UNSAFE_SCENE_HELPER_H_
