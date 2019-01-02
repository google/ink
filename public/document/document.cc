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

#include "ink/public/document/document.h"

#include <cstdint>  // uint64_t

#include "ink/engine/colors/colors.h"
#include "ink/engine/public/proto_traits.h"
#include "ink/engine/public/proto_validators.h"
#include "ink/engine/public/types/status.h"
#include "ink/proto/helpers.h"
#include "ink/public/fingerprint/fingerprint.h"

#if defined(PROTOBUF_INTERNAL_IMPL)
using proto2::RepeatedFieldBackInserter;
#else
#include "google/protobuf/repeated_field.h"
using google::protobuf::RepeatedFieldBackInserter;
#endif

namespace ink {

using proto::BackgroundImageInfo;
using proto::Color;
using proto::ElementBundle;
using proto::ElementIdList;
using proto::PageProperties;
using proto::Rect;
using proto::Snapshot;
using proto::SourceDetails;

/**
   If you are implementing a subclass, read these docs on the expected
   data flow and contract with the superclass. (If you want to skip the
   data flow and move straight to the instructions, see "To implement a
   Document subclass" below.)

   A Document subclass keeps a record of elements which are rendered in the
   engine in a manner that allows persistence and reconstruction of the
   persisted engine state. To manage this, it tracks creation, mutation,
   and removal of "Elements" as they move through the engine. It implements
   IElementListener which has events which exactly mirror this data flow.

   Events can originate in any of three places:
   1) The SEngine API. In this case, the SEngine forwards the calls immediately
      to the Document.
      SourceDetails.Origin = HOST.
   2) The Document itself (e.g., another user's stroke via a Brix document)
      SourceDetails.Origin = HOST
   3) The Engine (e.g., the user draws something on the screen)
      SourceDetails.Origin = ENGINE

   SEngine, Document, and the engine (SceneController) communicate via a
   couple of IElementListeners:
   a) SEngine owns and listens to a Document.
   b) SEngine registers Document to receive events from the SceneController.

   Follow the data:
   1) SEngine.addElement() (the flow is identical for mutate and remove).
   2) Calls Document.Add() (step 1 will be skipped if the Document is adding
      an Element directly, e.g., Brix.)
   3) Document persists element.
   4) Document calls its listeners (in this case, the root controller, via the
      controller's UnsafeSceneHelper).
   5) The root controller launches a background process to deserialize the
      element bundle and add the resulting ProcessedElement to the SceneGraph.
   6) The SceneGraph notifies its various SceneGraphListeners (such as various
      tools, the renderer, and element-manipulation tasks) and finally notifies
      PublicEvents.
   7) Because the document is hooked up to PublicEvents by the SEngine, it
      receives the ElementAdded event, but does nothing because the
      SourceDetails indicate that it is a HOST event. (I.e., the document has
      already handled it.)

   or:
   1) The user draws a stroke (flow same for mutate/remove)
   2) The SceneGraph notifies its various SceneGraphListeners (such as various
      tools, the renderer, and element-manipulation tasks) and finally notifies
      PublicEvents.
   3) Because the document is hooked up to PublicEvents by the SEngine, it
      receives the ElementAdded event and, because it has SourceDetails of
      ENGINE, the document will persist the element and notify the root
      controller's UnsafeSceneHelper, which ignores the event due to its
      ENGINE source.

   To implement a Document subclass:
   - Write AddBelowImpl(), RemoveImpl(), RemoveAllImpl(),
     SetElementTransformsImpl(), SetElementVisibilityImpl(),
     SetElementOpacityImpl(), and ChangeZOrderImpl().
*/

Document::Document()
    : document_dispatch_(new EventDispatch<IDocumentListener>()),
      page_props_dispatch_(new EventDispatch<IPagePropertiesListener>()),
      element_dispatch_(new EventDispatch<IElementListener>()),
      mutation_dispatch_(new EventDispatch<IMutationListener>()),
      active_layer_dispatch_(new EventDispatch<IActiveLayerListener>()) {}

Status Document::AddBelow(const ElementBundle& element,
                          const UUID& below_element_with_uuid) {
  _thread_validator.CheckIfOnSameThread();
  if (!ValidateProtoForAdd(element)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Unable to validate proto for add.");
  }
  INK_RETURN_UNLESS(
      AddBelowImpl({element}, below_element_with_uuid, GetHostSource()));
  return OkStatus();
}

Status Document::AddMultipleBelow(
    const std::vector<proto::ElementBundle>& elements,
    const UUID& below_element_with_uuid) {
  _thread_validator.CheckIfOnSameThread();

  for (const auto& elem : elements) {
    if (!ValidateProtoForAdd(elem)) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Unable to validate proto for add.");
    }
  }

  INK_RETURN_UNLESS(
      AddBelowImpl(elements, below_element_with_uuid, GetHostSource()));
  return OkStatus();
}

Status Document::SetPageBounds(const proto::Rect& unsafe_bounds) {
  _thread_validator.CheckIfOnSameThread();
  const float w = unsafe_bounds.xhigh() - unsafe_bounds.xlow();
  const float h = unsafe_bounds.yhigh() - unsafe_bounds.ylow();
  // 0x0 is valid meaning "go infinite", and +Wx+H is valid, but anything else
  // is garbage.
  if (w < 0 || h < 0 || (h == 0 && w != 0) || (w == 0 && h != 0)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "invalid dimensions ($0,$1)->($2,$3)",
                       unsafe_bounds.xlow(), unsafe_bounds.ylow(),
                       unsafe_bounds.xhigh(), unsafe_bounds.yhigh());
  }
  auto page_properties = GetPageProperties();
  *page_properties.mutable_bounds() = unsafe_bounds;
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  // Because this action is not undoable--does not use storage--we must notify,
  // instead of depending on a storage action to do so.
  NotifyPageBoundsChanged(page_properties.bounds(), GetHostSource());
  return OkStatus();
}

Status Document::SetBackgroundImage(
    const proto::BackgroundImageInfo& unsafe_background_image) {
  _thread_validator.CheckIfOnSameThread();
  if (!unsafe_background_image.has_uri() ||
      unsafe_background_image.uri().empty()) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "missing background image uri");
  }
  if (unsafe_background_image.has_bounds()) {
    auto unsafe_bounds = unsafe_background_image.bounds();
    if (unsafe_bounds.xhigh() - unsafe_bounds.xlow() <= 0 ||
        unsafe_bounds.yhigh() - unsafe_bounds.ylow() <= 0) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "invalid dimensions ($0,$1)->($2,$3)",
                         unsafe_bounds.xlow(), unsafe_bounds.ylow(),
                         unsafe_bounds.xhigh(), unsafe_bounds.yhigh());
    }
  }
  auto page_properties = GetPageProperties();
  *page_properties.mutable_background_image() = unsafe_background_image;
  page_properties.clear_background_color();
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  NotifyBackgroundImageChanged(unsafe_background_image, GetHostSource());
  return OkStatus();
}

Status Document::SetBackgroundColor(
    const proto::BackgroundColor& unsafe_background_color) {
  _thread_validator.CheckIfOnSameThread();
  auto page_properties = GetPageProperties();
  page_properties.clear_background_image();
  page_properties.mutable_background_color()->set_argb(
      Vec4ToUintARGB(UintToVec4RGBA(unsafe_background_color.rgba())));
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  NotifyBackgroundColorChanged(page_properties.background_color(),
                               GetHostSource());
  return OkStatus();
}

Status Document::SetPageBorder(const proto::Border& unsafe_page_border) {
  _thread_validator.CheckIfOnSameThread();
  // Empty URI is ok and means "clear border".
  if (unsafe_page_border.has_uri() &&
      (unsafe_page_border.scale() < kMinPageBorderScale ||
       unsafe_page_border.scale() > kMaxPageBorderScale)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "rejecting suspicious page border scale $0",
                       unsafe_page_border.scale());
  }
  auto page_properties = GetPageProperties();
  *page_properties.mutable_border() = unsafe_page_border;
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  NotifyBorderChanged(unsafe_page_border, GetHostSource());
  return OkStatus();
}

Status Document::SetGrid(const proto::GridInfo& unsafe_grid_info) {
  _thread_validator.CheckIfOnSameThread();
  if (unsafe_grid_info.size_world() <= 0) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "rejecting grid world size $0",
                       unsafe_grid_info.size_world());
  }
  auto page_properties = GetPageProperties();
  *page_properties.mutable_grid_info() = unsafe_grid_info;
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  NotifyGridChanged(unsafe_grid_info, GetHostSource());
  return OkStatus();
}

Status Document::SetPageProperties(const PageProperties& page_properties) {
  _thread_validator.CheckIfOnSameThread();
  INK_RETURN_UNLESS(SetPagePropertiesImpl(page_properties, GetHostSource()));
  if (page_properties.has_bounds()) {
    NotifyPageBoundsChanged(page_properties.bounds(), GetHostSource());
  }
  if (page_properties.has_background_color()) {
    NotifyBackgroundColorChanged(page_properties.background_color(),
                                 GetHostSource());
  }
  if (page_properties.has_background_image()) {
    NotifyBackgroundImageChanged(page_properties.background_image(),
                                 GetHostSource());
  }
  if (page_properties.has_border()) {
    NotifyBorderChanged(page_properties.border(), GetHostSource());
  }
  if (page_properties.has_grid_info()) {
    NotifyGridChanged(page_properties.grid_info(), GetHostSource());
  }
  return OkStatus();
}

Status Document::Remove(const std::vector<UUID>& uuids) {
  _thread_validator.CheckIfOnSameThread();
  return RemoveImpl(uuids, GetHostSource());
}

Status Document::RemoveAll() {
  _thread_validator.CheckIfOnSameThread();
  ElementIdList id_list;
  INK_RETURN_UNLESS(RemoveAllImpl(&id_list, GetHostSource()));
  return ClearPages();
}

Status Document::AddPage(const ink::proto::PerPageProperties& page) {
  return AddPageImpl(page);
}

Status Document::ClearPages() { return ClearPagesImpl(); }

void Document::NotifyAdd(const ElementBundle& element,
                         const UUID& below_element_with_uuid,
                         const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  element_dispatch_->Send(
      &IElementListener::ElementsAdded,
      ProtoHelpers::SingleElementAdd(element, below_element_with_uuid), source);
}

void Document::NotifyTransformMutation(
    const proto::ElementTransformMutations& mutations,
    const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  element_dispatch_->Send(&IElementListener::ElementsTransformMutated,
                          mutations, source);
}

void Document::NotifyRemove(const ElementIdList& removed_ids,
                            const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  element_dispatch_->Send(&IElementListener::ElementsRemoved, removed_ids,
                          source);
}

void Document::NotifyPageBoundsChanged(const ink::proto::Rect& bounds,
                                       const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  page_props_dispatch_->Send(&IPagePropertiesListener::PageBoundsChanged,
                             bounds, source);
  proto::mutations::Mutation mutation;
  *(mutation.add_chunk()->mutable_set_world_bounds()->mutable_bounds()) =
      bounds;
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void Document::NotifyBackgroundColorChanged(const ink::proto::Color& color,
                                            const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  page_props_dispatch_->Send(&IPagePropertiesListener::BackgroundColorChanged,
                             color, source);
  proto::mutations::Mutation mutation;
  mutation.add_chunk()
      ->mutable_set_background_color()
      ->set_rgba_non_premultiplied(
          Vec4ToUintRGBA(UintToVec4ARGB(color.argb())));
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void Document::NotifyBackgroundImageChanged(
    const ink::proto::BackgroundImageInfo& image, const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  page_props_dispatch_->Send(&IPagePropertiesListener::BackgroundImageChanged,
                             image, source);
  proto::mutations::Mutation mutation;
  *(mutation.add_chunk()
        ->mutable_set_background_image()
        ->mutable_background_image_info()) = image;
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void Document::NotifyBorderChanged(const ink::proto::Border& border,
                                   const SourceDetails& source) {
  _thread_validator.CheckIfOnSameThread();
  page_props_dispatch_->Send(&IPagePropertiesListener::BorderChanged, border,
                             source);
  proto::mutations::Mutation mutation;
  *(mutation.add_chunk()->mutable_set_border()->mutable_border()) = border;
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void Document::NotifyGridChanged(const ink::proto::GridInfo& grid_info,
                                 const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  page_props_dispatch_->Send(&IPagePropertiesListener::GridChanged, grid_info,
                             source_details);
  proto::mutations::Mutation mutation;
  *(mutation.add_chunk()->mutable_set_grid()->mutable_grid()) = grid_info;
  mutation_dispatch_->Send(&IMutationListener::OnMutation, mutation);
}

void Document::NotifyUndoRedoStateChanged(bool can_undo, bool can_redo) {
  _thread_validator.CheckIfOnSameThread();
  document_dispatch_->Send(&IDocumentListener::UndoRedoStateChanged, can_undo,
                           can_redo);
}

void Document::NotifyEmptyStateChanged(bool empty) {
  _thread_validator.CheckIfOnSameThread();
  DocumentDispatch()->Send(&IDocumentListener::EmptyStateChanged, empty);
}

void Document::AddDocumentListener(IDocumentListener* listener) {
  listener->RegisterOnDispatch(document_dispatch_);
}

void Document::RemoveDocumentListener(IDocumentListener* listener) {
  listener->Unregister(document_dispatch_);
}

void Document::AddElementListener(IElementListener* listener) {
  listener->RegisterOnDispatch(element_dispatch_);
}

void Document::RemoveElementListener(IElementListener* listener) {
  listener->Unregister(element_dispatch_);
}

void Document::AddPagePropertiesListener(IPagePropertiesListener* listener) {
  listener->RegisterOnDispatch(page_props_dispatch_);
}

void Document::RemovePagePropertiesListener(IPagePropertiesListener* listener) {
  listener->Unregister(page_props_dispatch_);
}

void Document::SetMutationEventsEnabled(bool enabled) {
  SLOG(SLOG_DOCUMENT, "$0 mutation events", enabled ? "enabling" : "disabling");
  mutation_dispatch_->SetEnabled(enabled);
}

void Document::AddMutationListener(IMutationListener* listener) {
  listener->RegisterOnDispatch(mutation_dispatch_);
}

void Document::RemoveMutationListener(IMutationListener* listener) {
  listener->Unregister(mutation_dispatch_);
}

void Document::AddActiveLayerListener(IActiveLayerListener* listener) {
  listener->RegisterOnDispatch(active_layer_dispatch_);
}

void Document::RemoveActiveLayerListener(IActiveLayerListener* listener) {
  listener->Unregister(active_layer_dispatch_);
}

// IElementListener
void Document::ElementsAdded(
    const proto::ElementBundleAdds& unsafe_element_adds,
    const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) {
    // Ignore anything with origin HOST since it was already processed.
    return;
  }

  std::vector<proto::ElementBundle> elements;
  for (const proto::ElementBundleAdd& add :
       unsafe_element_adds.element_bundle_add()) {
    if (!ValidateProtoForAdd(add.element_bundle())) {
      SLOG(SLOG_ERROR, "Unable to validate proto for add.");
      return;
    }
    elements.push_back(add.element_bundle());
  }

  AddBelowImpl(elements, unsafe_element_adds.element_bundle_add(0).below_uuid(),
               source_details)
      .IgnoreError();
}

Status Document::SetElementTransforms(
    const proto::ElementTransformMutations& mutations) {
  return ApplyMutations(mutations, GetHostSource());
}

template <typename MutationsType>
Status Document::ApplyMutationsWithHostCheck(
    const MutationsType& unsafe_mutations,
    const SourceDetails& source_details) {
  if (source_details.origin() == SourceDetails::HOST) {
    // Ignore anything with origin HOST since it was already processed.
    return OkStatus();
  }
  return ApplyMutations(unsafe_mutations, source_details);
}

template <typename MutationsType>
Status Document::ApplyMutations(const MutationsType& unsafe_mutations,
                                const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();

  if (!ValidateProto(unsafe_mutations)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Unable to validate proto.");
  }

  using Traits = ProtoTraits<MutationsType>;

  auto num_mutations = unsafe_mutations.mutation_size();
  std::vector<UUID> uuids;
  uuids.reserve(num_mutations);
  std::vector<typename Traits::ValueType> values;
  values.reserve(num_mutations);

  for (int i = 0; i < num_mutations; ++i) {
    const auto& mutation = unsafe_mutations.mutation(i);
    uuids.emplace_back(mutation.uuid());
    values.push_back(Traits::GetValue(mutation));
  }

  return Traits::ApplyToDocument(this, uuids, values, source_details);
}

void Document::ElementsTransformMutated(
    const proto::ElementTransformMutations& unsafe_mutations,
    const SourceDetails& source_details) {
  ApplyMutationsWithHostCheck(unsafe_mutations, source_details).IgnoreError();
}

void Document::ElementsVisibilityMutated(
    const proto::ElementVisibilityMutations& unsafe_mutations,
    const SourceDetails& source_details) {
  ApplyMutationsWithHostCheck(unsafe_mutations, source_details).IgnoreError();
}

void Document::ElementsOpacityMutated(
    const proto::ElementOpacityMutations& unsafe_mutations,
    const SourceDetails& source_details) {
  ApplyMutationsWithHostCheck(unsafe_mutations, source_details).IgnoreError();
}

void Document::ElementsZOrderMutated(
    const proto::ElementZOrderMutations& unsafe_mutations,
    const SourceDetails& source_details) {
  ApplyMutationsWithHostCheck(unsafe_mutations, source_details).IgnoreError();
}

void Document::ActiveLayerChanged(const UUID& uuid,
                                  const proto::SourceDetails& source_details) {
  if (source_details.origin() == SourceDetails::HOST) {
    // Ignore anything with origin HOST since it was already processed.
    return;
  }

  _thread_validator.CheckIfOnSameThread();

  ActiveLayerChangedImpl(uuid, source_details).IgnoreError();
}

void Document::ElementsRemoved(const ElementIdList& removed_ids,
                               const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) {
    // Ignore anything with origin HOST since it was already processed.
    return;
  }
  std::vector<UUID> uuids(removed_ids.uuid().begin(), removed_ids.uuid().end());
  RemoveImpl(uuids, source_details).IgnoreError();
}

void Document::ElementsReplaced(
    const proto::ElementBundleReplace& unsafe_replace,
    const proto::SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) {
    // Ignore anything with origin HOST since it was already processed.
    return;
  }

  std::vector<proto::ElementBundle> elements_to_add;
  std::vector<UUID> uuids_to_add_below;
  elements_to_add.reserve(
      unsafe_replace.elements_to_add().element_bundle_add_size());
  uuids_to_add_below.reserve(
      unsafe_replace.elements_to_add().element_bundle_add_size());
  for (const auto& element_bundle_add :
       unsafe_replace.elements_to_add().element_bundle_add()) {
    if (!ValidateProtoForAdd(element_bundle_add.element_bundle())) {
      SLOG(SLOG_ERROR, "Unable to validate proto for add.");
      return;
    }
    elements_to_add.push_back(element_bundle_add.element_bundle());
    uuids_to_add_below.push_back(element_bundle_add.below_uuid());
  }

  std::vector<UUID> uuids_to_remove(
      unsafe_replace.elements_to_remove().uuid().begin(),
      unsafe_replace.elements_to_remove().uuid().end());
  ReplaceImpl(elements_to_add, uuids_to_add_below, uuids_to_remove,
              source_details)
      .IgnoreError();
}

void Document::PageBoundsChanged(const proto::Rect& bounds,
                                 const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) return;

  UndoableSetPageBoundsImpl(bounds, source_details).IgnoreError();
}

void Document::BackgroundColorChanged(const proto::Color& color,
                                      const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) return;

  // The engine should not be changing the background color!
  ASSERT(source_details.origin() != SourceDetails::ENGINE);
}

void Document::BackgroundImageChanged(const proto::BackgroundImageInfo& image,
                                      const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) return;

  // The engine should not be changing the background image!
  ASSERT(source_details.origin() != SourceDetails::ENGINE);
}

void Document::BorderChanged(const proto::Border& border,
                             const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) return;

  // The engine should not be changing the page border!
  ASSERT(source_details.origin() != SourceDetails::ENGINE);
}

void Document::GridChanged(const proto::GridInfo& /*grid_info*/,
                           const SourceDetails& source_details) {
  _thread_validator.CheckIfOnSameThread();
  if (source_details.origin() == SourceDetails::HOST) return;

  // The engine should not be changing the grid!
  ASSERT(source_details.origin() != SourceDetails::ENGINE);
}

Snapshot Document::GetSnapshot(SnapshotQuery query) const {
  SLOG(SLOG_ERROR, "This document does not implement GetSnapshot!");
  Snapshot s;
  return s;
}

PageProperties Document::GetPageProperties() const {
  SLOG(SLOG_ERROR, "This document does not implement GetPageProperties!");
  PageProperties p;
  return p;
}

// Subclasses may have a way to do this more efficiently.
size_t Document::GetElementCount() const {
  return GetSnapshot(SnapshotQuery::DO_NOT_INCLUDE_UNDO_STACK).element_size();
}

void Document::SetPreferredThread() { _thread_validator.Reset(); }

uint64_t Document::GetFingerprint() const {
  return ink::GetFingerprint(
      GetSnapshot(SnapshotQuery::DO_NOT_INCLUDE_UNDO_STACK));
}

};  // namespace ink
