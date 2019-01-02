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

#ifndef INK_PUBLIC_DOCUMENT_DOCUMENT_H_
#define INK_PUBLIC_DOCUMENT_DOCUMENT_H_

#include <cstdint>  // uint64_t
#include <memory>
#include <utility>  // std::pair
#include <vector>

#include "ink/engine/public/host/iactive_layer_listener.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/imutation_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/util/dbg/current_thread.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/range.h"
#include "ink/engine/util/security.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/document/bundle_data_attachments.h"
#include "ink/public/document/idocument_listener.h"

namespace ink {

// Public interface to the Sketchology Document.
//
// Every SEngine has a Document which is used to manage things like:
// - persistence
// - undo/redo
// - element listeners (and other)
//
// You may get the document by calling sengine->document().
//
// If you are implementing a subclass, implement AddBelowImpl(), RemoveImpl(),
// RemoveAllImpl(), SetElementTransformsImpl(), SetElementVisibilityImpl(),
// SetElementOpacityImpl(), and ChangeZOrderImpl(). Optionally implement
// ReplaceImpl() and SupportsReplace(). Optionally look at the
// additional documentation in document.cc.
class Document : public IElementListener,
                 public IPagePropertiesListener,
                 public IActiveLayerListener {
 public:
  static constexpr float kMinPageBorderScale = .1;
  static constexpr float kMaxPageBorderScale = 10;

  Document();
  ~Document() override {}

  virtual std::string ToString() const { return "<Document>"; }

 public:
  // ***************************************************************************
  // These functions are called by the engine API user. They permit the
  // creation, manipulation, and querying of scene content.
  // ***************************************************************************
  // Close files, release resources, etc.
  virtual void Close() {}

  /*
   * By default, as this document is mutated, its storage will fire off mutation
   * events (i.e. OnMutation(proto::mutations::Mutation), which will be
   * received by the current Host. But if you're mutating the document by, say,
   * reading a Snapshot into it, then you probably don't want to be notified
   * about those mutations. Use this function to turn off mutation notifications
   * before modifying the document, and to turn them back on.
   */
  virtual void SetMutationEventsEnabled(bool enabled);

  // If you implement undo/redo, then your implementation must dispatch the
  // appropriate IElementListener events for undo and redo, e.g. undoing an add
  // must dispatch an ElementsRemoved event, and redoing an add must dispatch an
  // ElementAdded event.  Use the ElementDispatch accessor to dispatch events to
  // the document's registered listeners.
  virtual bool SupportsUndo() const { return false; }
  virtual void Undo() {}
  virtual void Redo() {}
  virtual bool CanUndo() const { return false; }
  virtual bool CanRedo() const { return false; }
  void NotifyUndoRedoStateChanged(bool can_undo, bool can_redo);
  virtual void SetUndoEnabled(bool enabled) {}
  virtual bool IsEmpty() = 0;
  void NotifyEmptyStateChanged(bool empty);

  // Multi-page support is experimental and only currently supported by the
  // single user document.
  virtual bool SupportsPaging() { return false; }

  // Add element to the document.
  S_WARN_UNUSED_RESULT Status Add(const proto::ElementBundle& element) {
    return AddBelow(element, kInvalidUUID);
  }

  // Add multiple elements to the document.
  S_WARN_UNUSED_RESULT Status
  AddMultiple(const std::vector<proto::ElementBundle>& element) {
    return AddMultipleBelow(element, kInvalidUUID);
  }

  // Add element to the document below the element with the specified UUID.
  S_WARN_UNUSED_RESULT Status AddBelow(const proto::ElementBundle& element,
                                       const UUID& below_element_with_uuid);

  // Add multiple elements to the document below the element with the specified
  // UUID.
  S_WARN_UNUSED_RESULT Status
  AddMultipleBelow(const std::vector<proto::ElementBundle>& element,
                   const UUID& below_element_with_uuid);

  // Set the transform for an Element. (I.e., mutate it.)
  S_WARN_UNUSED_RESULT Status
  SetElementTransforms(const proto::ElementTransformMutations& mutations);

  // Remove elements from the document.
  S_WARN_UNUSED_RESULT Status Remove(const std::vector<UUID>& uuids);

  // Remove all elements and pages from the document.
  S_WARN_UNUSED_RESULT Status RemoveAll();

  // If bounds is the Rect from (0,0) to (0,0), will set to infinite bounds.
  S_WARN_UNUSED_RESULT Status SetPageBounds(const proto::Rect& unsafe_bounds);

  // If backgroundImage.has_display_bounds(), changes the page bounds to
  // backgroundImage.display_bounds().
  S_WARN_UNUSED_RESULT Status
  SetBackgroundImage(const proto::BackgroundImageInfo& unsafe_background_image);

  S_WARN_UNUSED_RESULT Status
  SetBackgroundColor(const proto::BackgroundColor& unsafe_background_color);

  // Requires page bounds to be set
  S_WARN_UNUSED_RESULT Status
  SetPageBorder(const proto::Border& unsafe_page_border);

  S_WARN_UNUSED_RESULT Status SetGrid(const proto::GridInfo& unsafe_grid_info);

  // DEPRECATED
  // This will be removed after iOS and Android wrappers have been converted to
  // use the specific setters for bounds, color, image, and border, above.
  S_WARN_UNUSED_RESULT Status
  SetPageProperties(const proto::PageProperties& page_properties);

  // Experimental API for multipage support. Adds a page to the document.
  // DO NOT USE - Internal to the engine only.
  S_WARN_UNUSED_RESULT Status AddPage(const proto::PerPageProperties& page);
  // Experimental API for multipage support. Removes all pages from the
  // document.
  // DO NOT USE - Internal to the engine only.
  S_WARN_UNUSED_RESULT Status ClearPages();

 public:
  // ***************************************************************************
  // These functions are only implemented by Documents that support querying,
  // i.e., Documents that actually do storage (which, in practice, means only
  // SingleUserDocument).
  // ***************************************************************************
  virtual bool SupportsQuerying() { return false; }
  virtual proto::PageProperties GetPageProperties() const;

  // Retrieves a 64-bit (MD5-based) fingerprint of this document.
  // This is compatible with the fingerprint generated by the SEngine's
  // image export.
  virtual uint64_t GetFingerprint() const;

  // GetSnapshot builds a proto representation of the contents of this Document,
  // suitable for serialization and storage.
  enum SnapshotQuery {
    INCLUDE_UNDO_STACK,        // This will cause dead elements to be persisted,
                               // potentially taking more space.
    DO_NOT_INCLUDE_UNDO_STACK  // This is all you need for rendering the current
                               // document state, or tests that examine state.
  };
  virtual proto::Snapshot GetSnapshot(
      SnapshotQuery query = SnapshotQuery::INCLUDE_UNDO_STACK) const;

  // GetElementCount returns the number of scene elements (strokes).
  virtual size_t GetElementCount() const;

 public:
  // ***************************************************************************
  // These functions are internal, to be called only by the scene graph and its
  // helpers.
  // They allow the scene graph to tell the document to persist elements created
  // by user gestures (i.e. strokes), and they allow the document to tell the
  // scene graph about elements created and manipulated via host function calls.
  // ***************************************************************************
  void AddDocumentListener(IDocumentListener* listener);
  void RemoveDocumentListener(IDocumentListener* listener);
  void AddElementListener(IElementListener* listener);
  void RemoveElementListener(IElementListener* listener);
  void AddPagePropertiesListener(IPagePropertiesListener* listener);
  void RemovePagePropertiesListener(IPagePropertiesListener* listener);
  void AddMutationListener(IMutationListener* listener);
  void RemoveMutationListener(IMutationListener* listener);
  void AddActiveLayerListener(IActiveLayerListener* listener);
  void RemoveActiveLayerListener(IActiveLayerListener* listener);

  // IElementListener
  void ElementsAdded(const proto::ElementBundleAdds& unsafe_element_adds,
                     const proto::SourceDetails& source_details) override;
  void ElementsRemoved(const proto::ElementIdList& removed_ids,
                       const proto::SourceDetails& source_details) override;
  void ElementsReplaced(const proto::ElementBundleReplace& unsafe_replace,
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

  // IActiveLayerListener
  void ActiveLayerChanged(const UUID& uuid,
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

  // Choose the current thread to be the required thread for subsequent
  // mutations.
  void SetPreferredThread();

  template <typename MutationsType>
  S_WARN_UNUSED_RESULT inline Status ApplyMutations(
      const MutationsType& unsafe_mutations) {
    return ApplyMutations(unsafe_mutations, GetHostSource());
  }

 protected:
  virtual S_WARN_UNUSED_RESULT Status
  AddPageImpl(const proto::PerPageProperties& page) {
    if (SupportsPaging()) {
      RUNTIME_ERROR("This document does not implement AddPageImpl.");
    }
    return ErrorStatus(StatusCode::UNIMPLEMENTED,
                       "AddPageImpl is unimplemented by this Document");
  }
  virtual S_WARN_UNUSED_RESULT Status ClearPagesImpl() {
    if (SupportsPaging()) {
      RUNTIME_ERROR("This document does not implement ClearPagesImpl.");
    }
    return ErrorStatus(StatusCode::UNIMPLEMENTED,
                       "ClearPagesImpl is unimplemented by this Document");
  }

  // Add elements to the document below the element with the specified UUID.
  // Return true on success.
  virtual S_WARN_UNUSED_RESULT Status
  AddBelowImpl(const std::vector<ink::proto::ElementBundle>& elements,
               const UUID& below_element_with_uuid,
               const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status
  SetElementTransformsImpl(const std::vector<UUID> uuids,
                           const std::vector<proto::AffineTransform> transforms,
                           const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status SetElementVisibilityImpl(
      const std::vector<UUID> uuids, const std::vector<bool> visibilities,
      const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status SetElementOpacityImpl(
      const std::vector<UUID> uuids, const std::vector<int32> opacities,
      const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status ChangeZOrderImpl(
      const std::vector<UUID> uuids, const std::vector<UUID> below_uuids,
      const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status ActiveLayerChangedImpl(
      const UUID& uuid, const proto::SourceDetails& source_details) = 0;

  // Remove elements from the document.
  // Return true if all uuids were removed.
  // If false is returned, no elements were removed.
  virtual S_WARN_UNUSED_RESULT Status
  RemoveImpl(const std::vector<UUID>& uuids,
             const proto::SourceDetails& source_details) = 0;

  // Remove all elements from the document, and populate the given
  // ElementIdList with the UUIDs of elements so removed.
  // Return true on success.
  virtual S_WARN_UNUSED_RESULT Status
  RemoveAllImpl(proto::ElementIdList* removed,
                const proto::SourceDetails& source_details) = 0;

  // Add the elements in elements_to_add such that they are below the
  // corresponding element in uuids_to_add_below, and remove the elements in
  // uuids_to_remove. All of this should be considered a single action w.r.t.
  // the undo stack. Note that elements_to_add and uuids_to_add_below must be
  // the same size.
  virtual S_WARN_UNUSED_RESULT Status
  ReplaceImpl(const std::vector<proto::ElementBundle>& elements_to_add,
              const std::vector<UUID>& uuids_to_add_below,
              const std::vector<UUID>& uuids_to_remove,
              const proto::SourceDetails& source_details) = 0;

  // Set the page properties.
  // Return true on success.
  virtual S_WARN_UNUSED_RESULT Status
  SetPagePropertiesImpl(const proto::PageProperties& page_properties,
                        const proto::SourceDetails& source_details) = 0;

  virtual S_WARN_UNUSED_RESULT Status
  UndoableSetPageBoundsImpl(const proto::Rect& bounds,
                            const proto::SourceDetails& source_details) = 0;

  std::shared_ptr<EventDispatch<IMutationListener>> MutationDispatch() const {
    return mutation_dispatch_;
  }

  std::shared_ptr<EventDispatch<IElementListener>> ElementDispatch() const {
    return element_dispatch_;
  }

  std::shared_ptr<EventDispatch<IDocumentListener>> DocumentDispatch() const {
    return document_dispatch_;
  }

  std::shared_ptr<EventDispatch<IActiveLayerListener>> ActiveLayerDispatch()
      const {
    return active_layer_dispatch_;
  }

  std::shared_ptr<EventDispatch<IPagePropertiesListener>>
  PagePropertiesDispatch() const {
    return page_props_dispatch_;
  }

 protected:
  // ***************************************************************************
  // These functions should be private, but the iOS Brix Document uses them.
  // And who wants to monkey with *that*?
  // ***************************************************************************
  void NotifyAdd(const proto::ElementBundle& element,
                 const UUID& below_element_with_uuid,
                 const proto::SourceDetails& source);
  void NotifyTransformMutation(const proto::ElementTransformMutations& mutation,
                               const proto::SourceDetails& source);
  void NotifyRemove(const proto::ElementIdList& removed_ids,
                    const proto::SourceDetails& source);
  void NotifyPageBoundsChanged(const proto::Rect& bounds,
                               const proto::SourceDetails& source);
  void NotifyBackgroundColorChanged(const proto::Color& color,
                                    const proto::SourceDetails& source);
  void NotifyBackgroundImageChanged(const proto::BackgroundImageInfo& image,
                                    const proto::SourceDetails& source);
  void NotifyBorderChanged(const proto::Border& border,
                           const proto::SourceDetails& source);
  void NotifyGridChanged(const proto::GridInfo& grid_info,
                         const proto::SourceDetails& source_details);

  static proto::SourceDetails GetHostSource() {
    proto::SourceDetails source;
    source.set_origin(proto::SourceDetails::HOST);
    return source;
  }

 private:
  std::shared_ptr<EventDispatch<IDocumentListener>> document_dispatch_;
  std::shared_ptr<EventDispatch<IPagePropertiesListener>> page_props_dispatch_;
  std::shared_ptr<EventDispatch<IElementListener>> element_dispatch_;
  std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch_;
  std::shared_ptr<EventDispatch<IActiveLayerListener>> active_layer_dispatch_;
  CurrentThreadValidator _thread_validator;

  // Make ProtoTraits a friend so that it can call the various SetElement*Impl
  // protected methods.
  template <typename T>
  friend class ProtoTraits;

  // Templatized method that applies the mutations in unsafe_mutation.
  //
  // Uses Traits to handle minor differences in the processing:
  //
  // 1) The type of the value being mutated.
  //
  // 2) The getter used to read the value from unsafe_mutation.
  //
  // 3) The specific document mutator function to call after the ElementMutation
  // has been split up into its constituent vectors.
  //
  // This method also also validates the unsafe_mutation and ensures that we are
  // on the correct thread.
  //
  // If source_details == HOST, do nothing.
  template <typename MutationsType>
  S_WARN_UNUSED_RESULT Status
  ApplyMutationsWithHostCheck(const MutationsType& unsafe_mutations,
                              const proto::SourceDetails& source_details);

  // Same as ApplyMutations without the check for source_details == HOST.
  template <typename MutationsType>
  S_WARN_UNUSED_RESULT Status
  ApplyMutations(const MutationsType& unsafe_mutations,
                 const proto::SourceDetails& source_details);
};

};  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_DOCUMENT_H_
