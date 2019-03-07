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

// A Service that listens to interesting Engine events and fans them out using
// public protos.

#ifndef INK_ENGINE_PUBLIC_HOST_PUBLIC_EVENTS_H_
#define INK_ENGINE_PUBLIC_HOST_PUBLIC_EVENTS_H_

#include "logs/proto/research/ink/ink_event_portable_proto.pb.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/public/host/imutation_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/host/iscene_change_listener.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
class PublicEvents : public IElementListener,
                     public IEngineListener,
                     public IMutationListener,
                     public IPagePropertiesListener,
                     public ISceneChangeListener {
 public:
  PublicEvents();

  void AddElementListener(IElementListener* listener);
  void RemoveElementListener(IElementListener* listener);
  void AddEngineListener(IEngineListener* listener);
  void RemoveEngineListener(IEngineListener* listener);
  void AddMutationListener(IMutationListener* listener);
  void RemoveMutationListener(IMutationListener* listener);
  void AddPagePropertiesListener(IPagePropertiesListener* listener);
  void RemovePagePropertiesListener(IPagePropertiesListener* listener);
  void AddSceneChangeListener(ISceneChangeListener* listener);
  void RemoveSceneChangeListener(ISceneChangeListener* listener);

  // IEngineListener
  void ImageExportComplete(uint32_t width_px, uint32_t height_px,
                           const std::vector<uint8_t>& img_bytes,
                           uint64_t fingerprint) override;
  void PdfSaveComplete(const std::string& pdf_bytes) override;
  void ToolEvent(const proto::ToolEvent& tool_event) override;
  void SequencePointReached(int32_t id) override;
  void UndoRedoStateChanged(bool canUndo, bool canRedo) override;
  void FlagChanged(const proto::Flag& flag, bool enabled) override;
  void LoggingEventFired(
      const ::logs::proto::research::ink::InkEvent& event) override;
  void CameraMovementStateChanged(bool is_moving) override;
  void BlockingStateChanged(bool is_blocked) override;

  // IElementListener
  void ElementsAdded(const proto::ElementBundleAdds& element_bundle_adds,
                     const proto::SourceDetails& source_details) override;
  void ElementsRemoved(const proto::ElementIdList& removed_ids,
                       const proto::SourceDetails& source_details) override;
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
                         const proto::SourceDetails& source_details) override;
  void BackgroundColorChanged(
      const proto::Color& color,
      const proto::SourceDetails& source_details) override;
  void BackgroundImageChanged(
      const proto::BackgroundImageInfo& image,
      const proto::SourceDetails& source_details) override;
  void BorderChanged(const proto::Border& border,
                     const proto::SourceDetails& source_details) override;
  void GridChanged(const proto::GridInfo& grid_info,
                   const proto::SourceDetails& source_details) override;

  // IMutationListener
  void OnMutation(const proto::mutations::Mutation& unsafe_mutation) override;

  // ISceneChangeListener
  void SceneChanged(
      const proto::scene_change::SceneChangeEvent& layer_change) override;

 private:
  std::shared_ptr<EventDispatch<IElementListener>> element_dispatch_;
  std::shared_ptr<EventDispatch<IEngineListener>> engine_dispatch_;
  std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch_;
  std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_props_dispatch_;
  std::shared_ptr<EventDispatch<ISceneChangeListener>> scene_change_dispatch_;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_PUBLIC_EVENTS_H_
