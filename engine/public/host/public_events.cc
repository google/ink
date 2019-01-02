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

#include "ink/engine/public/host/public_events.h"

namespace ink {

PublicEvents::PublicEvents()
    : element_dispatch_(new EventDispatch<IElementListener>()),
      engine_dispatch_(new EventDispatch<IEngineListener>()),
      mutation_dispatch_(new EventDispatch<IMutationListener>()),
      page_props_dispatch_(new EventDispatch<IPagePropertiesListener>()),
      scene_change_dispatch_(new EventDispatch<ISceneChangeListener>()) {}

void PublicEvents::AddElementListener(IElementListener* listener) {
  listener->RegisterOnDispatch(element_dispatch_);
}

void PublicEvents::RemoveElementListener(IElementListener* listener) {
  listener->Unregister(element_dispatch_);
}

void PublicEvents::AddEngineListener(IEngineListener* listener) {
  listener->RegisterOnDispatch(engine_dispatch_);
}

void PublicEvents::RemoveEngineListener(IEngineListener* listener) {
  listener->Unregister(engine_dispatch_);
}

void PublicEvents::AddMutationListener(IMutationListener* listener) {
  listener->RegisterOnDispatch(mutation_dispatch_);
}

void PublicEvents::RemoveMutationListener(IMutationListener* listener) {
  listener->Unregister(mutation_dispatch_);
}

void PublicEvents::AddPagePropertiesListener(
    IPagePropertiesListener* listener) {
  listener->RegisterOnDispatch(page_props_dispatch_);
}

void PublicEvents::RemovePagePropertiesListener(
    IPagePropertiesListener* listener) {
  listener->Unregister(page_props_dispatch_);
}

void PublicEvents::AddSceneChangeListener(ISceneChangeListener* listener) {
  listener->RegisterOnDispatch(scene_change_dispatch_);
}

void PublicEvents::RemoveSceneChangeListener(ISceneChangeListener* listener) {
  listener->Unregister(scene_change_dispatch_);
}

// IElementListener
void PublicEvents::ElementsAdded(
    const proto::ElementBundleAdds& element_bundle_adds,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsAdded, element_bundle_adds,
                          source_details);
}

void PublicEvents::ElementsTransformMutated(
    const proto::ElementTransformMutations& mutations,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsTransformMutated,
                          mutations, source_details);
}

void PublicEvents::ElementsVisibilityMutated(
    const proto::ElementVisibilityMutations& mutations,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsVisibilityMutated,
                          mutations, source_details);
}

void PublicEvents::ElementsOpacityMutated(
    const proto::ElementOpacityMutations& mutations,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsOpacityMutated, mutations,
                          source_details);
}

void PublicEvents::ElementsZOrderMutated(
    const proto::ElementZOrderMutations& mutations,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsZOrderMutated, mutations,
                          source_details);
}

void PublicEvents::ElementsRemoved(const proto::ElementIdList& removed_ids,
                                   const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsRemoved, removed_ids,
                          source_details);
}

void PublicEvents::ElementsReplaced(
    const proto::ElementBundleReplace& replace,
    const proto::SourceDetails& source_details) {
  element_dispatch_->Send(&IElementListener::ElementsReplaced, replace,
                          source_details);
}

// IEngineListener
void PublicEvents::ImageExportComplete(uint32_t width_px, uint32_t height_px,
                                       const std::vector<uint8_t>& img_bytes,
                                       uint64_t fingerprint) {
  engine_dispatch_->Send(&IEngineListener::ImageExportComplete, width_px,
                         height_px, img_bytes, fingerprint);
}

void PublicEvents::PdfSaveComplete(const std::string& pdf_bytes) {
  engine_dispatch_->Send(&IEngineListener::PdfSaveComplete, pdf_bytes);
}

void PublicEvents::UndoRedoStateChanged(bool canUndo, bool canRedo) {
  engine_dispatch_->Send(&IEngineListener::UndoRedoStateChanged, canUndo,
                         canRedo);
}

void PublicEvents::FlagChanged(const proto::Flag& flag, bool enabled) {
  engine_dispatch_->Send(&IEngineListener::FlagChanged, flag, enabled);
}

void PublicEvents::ToolEvent(const proto::ToolEvent& tool_event) {
  engine_dispatch_->Send(&IEngineListener::ToolEvent, tool_event);
}

void PublicEvents::SequencePointReached(int32_t id) {
  engine_dispatch_->Send(&IEngineListener::SequencePointReached, id);
}

void PublicEvents::LoggingEventFired(
    const ::logs::proto::research::ink::InkEvent& event) {
  engine_dispatch_->Send(&IEngineListener::LoggingEventFired, event);
}

// IPagePropertiesListener
void PublicEvents::PageBoundsChanged(
    const proto::Rect& bounds, const proto::SourceDetails& source_details) {
  page_props_dispatch_->Send(&IPagePropertiesListener::PageBoundsChanged,
                             bounds, source_details);
}

void PublicEvents::BackgroundColorChanged(
    const proto::Color& color, const proto::SourceDetails& source_details) {
  page_props_dispatch_->Send(&IPagePropertiesListener::BackgroundColorChanged,
                             color, source_details);
}

void PublicEvents::BackgroundImageChanged(
    const proto::BackgroundImageInfo& image,
    const proto::SourceDetails& source_details) {
  page_props_dispatch_->Send(&IPagePropertiesListener::BackgroundImageChanged,
                             image, source_details);
}

void PublicEvents::BorderChanged(const proto::Border& border,
                                 const proto::SourceDetails& source_details) {
  page_props_dispatch_->Send(&IPagePropertiesListener::BorderChanged, border,
                             source_details);
}

void PublicEvents::GridChanged(const proto::GridInfo& grid_info,
                               const proto::SourceDetails& source_details) {
  page_props_dispatch_->Send(&IPagePropertiesListener::GridChanged, grid_info,
                             source_details);
}
void PublicEvents::OnMutation(
    const proto::mutations::Mutation& unsafe_mutation) {
  mutation_dispatch_->Send(&IMutationListener::OnMutation, unsafe_mutation);
}

void PublicEvents::SceneChanged(
    const proto::scene_change::SceneChangeEvent& layer_change) {
  scene_change_dispatch_->Send(&ISceneChangeListener::SceneChanged,
                               layer_change);
}

}  // namespace ink
