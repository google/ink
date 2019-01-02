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

#ifndef INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_H_
#define INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_H_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/spatial_index_factory_interface.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/poly_store.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/graph/element_notifier.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_index.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/engine/scene/types/id_map.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/scene/types/transform_map.h"
#include "ink/engine/scene/types/updatable.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/funcs/uuid_generator.h"

namespace ink {

// SceneGraph is the main element store for the engine.
//-----------------------------------------------------------------------------
// Main features are:
//  - Add/Remove/Mutate elements
//  - Spatial queries & filtering
//  - Draw order sorting (zIndex)
//  - Testing rendering responsibility. (RenderedByMain)
//
// In general scene graph owns the element, only exposing an opaque identifier
// (element_id). API surfaces are provided to inspect/modify the element
// without transferring ownership. Ex to draw an element you would call
// graph.getMesh() to get a drawable component at the appropriate fidelity.
//
// Element Z-Index
// ----------------------------------------------------------------------------
// The Z-index of an element is based on which group it is in and the relative
// ordering of the groups. Think of this as a tree.
// An element in a group that is at a lower z-index will be drawn before an
// element of a group with a higher z-index.
// As of now, group chaining isn't supported. That is to say, any given POLY
// will be either in the root OR in a group that is attached directly to the
// root.
//
// Element Identity
// ----------------------------------------------------------------------------
// There are 2 ways of identifying elements in the engine -
// element_id and UUID.
//
// Important element_id properties:
//  - element_ids are reused between engine instances
//  - There are ~2^30 available (not guaranteed -- see types/element_id.h)
//  - element_ids are cheap! (sort, copy, compare, etc)
//
// Important UUID properties:
//  - uuids are unique between all engines
//  - There are ~2^128 available (See uuid_generator.cc)
//  - uuids are expensive!
//
// element_id are canonical. All engine internals not explicitly dealing with
// element_id/UUID mapping or a public API should use only element_ids.
// (They're fast, and unique enough).
//
// UUIDs are provided as a convenience through the public API as a way to
// preserve identity across multiple engines or multiple sessions.
//
// All ids must come from scene_graph to maintain uniqueness guarantees
class SceneGraph {
 public:
  using SharedDeps =
      service::Dependencies<PolyStore, IElementListener,
                            spatial::SpatialIndexFactoryInterface>;

  // This struct contains the data necessary to add an element to the scene.
  struct ElementAdd {
    // The optimized mesh data for use in the scene graph.
    std::unique_ptr<ProcessedElement> processed_element;
    // The serialization of the element to be passed on to the document.
    std::unique_ptr<SerializedElement> serialized_element;
    // The element below which the new element should be added. A value of
    // kInvalidElementId indicates the element should be added at the top of its
    // group. Note that the group is specified in the ProcessedElement.
    ElementId id_to_add_below;

    ElementAdd() : ElementAdd(nullptr, nullptr) {}
    ElementAdd(std::unique_ptr<ProcessedElement> processed_element_in,
               std::unique_ptr<SerializedElement> serialized_element_in,
               ElementId id_to_add_below_in = kInvalidElementId)
        : processed_element(std::move(processed_element_in)),
          serialized_element(std::move(serialized_element_in)),
          id_to_add_below(id_to_add_below_in) {}
  };

  SceneGraph(std::shared_ptr<PolyStore> poly_store,
             std::shared_ptr<IElementListener> element_listener,
             std::shared_ptr<spatial::SpatialIndexFactoryInterface>
                 spatial_index_factory);
  ~SceneGraph();

  bool GetNextPolyId(const UUID& uuid, ElementId* result);
  bool GetNextGroupId(const UUID& uuid, GroupId* result);
  UUID GenerateUUID() { return uuid_generator_.GenerateUUID(); }

  // Returns the element id corresponding to a uuid.
  // If the uuid is not known, returns kInvalidElementId.
  ElementId ElementIdFromUUID(const UUID& id) const;
  // The following functions call ElementIdFromUUID but asserts that the type
  // is as expected. These should be preferred over ElementIdFromUUID.
  ElementId PolyIdFromUUID(const UUID& id) const;
  GroupId GroupIdFromUUID(const UUID& id) const;
  // Returns the uuid corresponding to an element id.
  // If the element id is not known, returns kInvalidUUID.
  UUID UUIDFromElementId(const ElementId& id) const;
  // If the element id is not known, returns an ElementMetadata whose id is
  // kInvalidUUID.
  ElementMetadata GetElementMetadata(ElementId id) const;
  // Return the element's group id.
  GroupId GetParentGroupId(ElementId id) const;

  // This adds an element to the scene. This function should only be called with
  // a non-GROUP element. The following callbacks are dispatched to be run off
  // thread (as a task):
  //   SceneGraphListener::PreElementAdded()
  //     After the transforms are saved, but before the points are moved into
  //     the poly store or the spatial index is created.
  //   SceneGraphListener::OnElementAdded()
  //     After the element has been fully added.
  // The following callbacks are run immediately:
  //   ElementNotifier::OnElementAdded()
  //     After the element has been fully added.
  void AddStroke(ElementAdd element_to_add);

  // Adds the given elements to the scene, generating only one callback to the
  // IElementListener. In the event that multiple elements are added below the
  // same reference element (i.e., they have the ID in id_to_add_below), the
  // elements that occur sooner in the list will be placed below the
  // elements later in the list.
  void AddStrokes(std::vector<ElementAdd> elements_to_add);

  // This will associate the given group id with the passed in group states.
  // This may generate up to two ElementNotifier::OnElementsMutated calls,
  // one if the world transform has changed and one if the spatial index has
  // changed.
  // If a Group is marked as Clippable, the bounds will be used as a Scissoring
  // boundary for the purposes of rendering.
  // Note that bounds must be specified in group-coordinates, not
  // world-coordinates.
  // The group_type passed in will be added to the ElementMetadata for that
  // Element.
  void AddOrUpdateGroup(GroupId group_id, glm::mat4 group_to_world_transform,
                        Rect bounds, bool clippable, GroupType group_type,
                        SourceDetails source_details);

  // Move the group specified by group_id so that it comes before before_group
  // in the stacking order. If group_id and before_group are not siblings, the
  // behavior is undefined. If before_group == kInvalidElementId, group_id will
  // be moved to be last in the stacking order. If group_id == before_group,
  // nothing happens.
  void MoveGroup(GroupId group_id, GroupId before_group);

  // Sets the parent for the given element id. The element's obj to group
  // transform (relative to the new group) is changed such that its world
  // transform stays the same.
  void SetParent(ElementId element_id, GroupId group_id);

  // Removes elements from the scene.
  //
  // beginElements, endElements are input_iterators<element_id>
  template <typename ElementInputIt>
  void RemoveElements(ElementInputIt begin_elements,
                      ElementInputIt end_elements, const SourceDetails& source);
  // Like above but for a single element
  void RemoveElement(ElementId id, SourceDetails source);
  // Like above but removes all elements.
  void RemoveAllElements(SourceDetails source);

  // Removes all selectable elements, i.e., those that the user can create and
  // manipulate. This excludes things like PDF background polygons. This is the
  // function you want when implementing a user-accessible "clear scene" action.
  void RemoveAllSelectableElements();

  // Adds and removes elements as a single action, generating only one callback
  // to the IElementListener. All new elements are inserted before any deletions
  // occur.
  // Note: Groups may not be replaced.
  // Note: While IElementListener receives only one callback, the
  // SceneGraphListeners will receive multiple callbacks, as per usual.
  void ReplaceElements(std::vector<ElementAdd> elements_to_add,
                       const std::vector<ElementId>& elements_to_remove,
                       const SourceDetails source_details);

  // Sets the transform for each element
  //
  // beginElements, endElements are input_iterators<element_id>
  // beginTransforms, endTransforms are input_iterators<glm::mat4>
  //
  // std::distance(beginTransforms, endTransforms) must equal
  // std::distance(beginElements, endElements)
  //
  // You may set a transform on an element that does not exist yet
  // (ie elementExists(id) == false) in which case the transform will be
  // applied whenever the element is added.
  template <typename ElementInputIt, typename MatInputIt>
  void TransformElements(ElementInputIt begin_elements,
                         ElementInputIt end_elements,
                         MatInputIt begin_transforms, MatInputIt end_transforms,
                         const SourceDetails& source);
  // Like above but for a single element
  void TransformElement(ElementId id, glm::mat4 new_transform,
                        const SourceDetails& source_details);

  // Sets the persisted visibility for each element.
  // NOTE: Be caseful not to confuse this with the old ElementVisibility
  //       value which has been replaced by RenderedByMain.
  template <typename ElementInputIt, typename BoolInputIt>
  void SetVisibilities(ElementInputIt begin_elements,
                       ElementInputIt end_elements,
                       BoolInputIt begin_visibilities,
                       BoolInputIt end_visibilities,
                       const SourceDetails& source_details);

  // Sets the persisted opacity for each element.
  template <typename ElementInputIt, typename BoolInputIt>
  void SetOpacities(ElementInputIt begin_elements, ElementInputIt end_elements,
                    BoolInputIt begin_opacities, BoolInputIt end_opacities,
                    const SourceDetails& source_details);

  // Moves each element below the associated element in opacities.
  template <typename ElementInputIt>
  void ChangeZOrders(ElementInputIt begin_elements, ElementInputIt end_elements,
                     ElementInputIt begin_below_elements,
                     ElementInputIt end_below_elements,
                     const SourceDetails& source_details);

  // Sets rendered_by_main flag for all elements.
  //
  // beginElements, endElements are input_iterators<element_id>
  //
  // You may set rendered_by_main for an element that does not exist yet
  // (ie elementExists(id) == false) in which case the value will be
  // applied after the element is added.
  template <typename InputIt>
  void SetElementRenderedByMain(InputIt begin_elements, InputIt end_elements,
                                bool rendered_by_main);

  // Like above but for a single element
  void SetElementRenderedByMain(ElementId id, bool rendered_by_main);

  // Set the color of the element with the given ID to the given premultiplied
  // color. This may not be a meaningful operation for some elements, in which
  // case the side effect is undefined.
  void SetColor(ElementId id, glm::vec4 rgba, SourceDetails source);
  glm::vec4 GetColor(ElementId id);
  bool GetTextureUri(ElementId id, std::string* uri);

  void Update(const Camera& cam);

  void AddDrawable(std::shared_ptr<IDrawable> drawable);
  void RemoveDrawable(IDrawable* drawable);
  const std::vector<std::shared_ptr<IDrawable>>& GetDrawables() const;

  bool GetMesh(ElementId id, OptimizedMesh** mesh) const;

  // The spatial index can change over the lifetime of an element -- if you
  // fetch it, you'll very likely want to listen for the notification from
  // SceneGraphListener::OnElementsMutated()
  std::shared_ptr<spatial::SpatialIndex> GetSpatialIndex(ElementId id) const;
  void SetSpatialIndex(ElementId id,
                       std::shared_ptr<spatial::SpatialIndex> index);
  template <typename ElementIter, typename SpatialIndexIter>
  void SetSpatialIndices(ElementIter element_begin, ElementIter element_end,
                         SpatialIndexIter index_begin,
                         SpatialIndexIter index_end);

  // returns the minimum bounding Rect of "elements"
  Rect Mbr(const std::vector<ElementId>& elements) const;

  // returns the minimum bounding Rect of all elements in the scenegraph,
  // regardless of visibility.
  Rect Mbr() const;

  // returns the minimum bounding Rect of all elements that are children of the
  // group.
  Rect MbrForGroup(GroupId group_id) const;

  // returns the minimum bounding Rect of element
  //
  // result is in object coords. That is, the result is unaffected by
  // any transform set on the object
  Rect MbrObjCoords(ElementId element) const;

  // Predicates for filtering groups and elements when traversing the scene
  // graph.
  using GroupFilter = std::function<bool(GroupId)>;
  using ElementFilter = std::function<bool(ElementId)>;
  // Visit a graph element. Implementors are provided with the element's parent
  // group.
  using ElementVisitor = std::function<void(GroupId, ElementId)>;

  // Populate the collection pointed at by "output" element_ids that are
  // children of groups that pass the given group filter, and which themselves
  // pass the given element filter.
  //
  // Elements are sorted by z-index, back to front
  //
  // output is an output_iterator<element_id>
  template <typename OutputIt>
  void ElementsInScene(GroupFilter group_filter, ElementFilter element_filter,
                       OutputIt output) const;

  // Populate the collection pointed at by "output" with every
  // element_id in the scene that should be rendered by the main renderer.
  //
  // Elements are sorted by z-index, back to front
  //
  // output is an output_iterator<element_id>
  template <typename OutputIt>
  void ElementsInScene(OutputIt output) const;

  // Populate the collection pointed at by "output" with every group child
  // of the root group.
  template <typename OutputIt>
  void GroupChildrenOfRoot(OutputIt output) const;

  // Fetches the elements that match the provided query.
  // output is an output_iterator<element_id>. Groups are only
  // expanded if their spatial index matches the bounds of the query.
  template <typename OutputIt>
  void ElementsInRegion(const RegionQuery& query, OutputIt output) const;

  struct GroupedElements {
    Rect bounds;
    GroupId group_id;  // can be kInvalidElementId for root.
    std::vector<ElementId> poly_ids;
  };
  using GroupedElementsList = std::vector<GroupedElements>;
  using ElementIdIndex = ElementIndex<ElementId, ElementIdHasher>;
  using GroupElementIdIndexMap =
      ElementIdHashMap<std::shared_ptr<ElementIdIndex>>;
  // GroupedElementsList will be returned in z-order (the first GroupElements
  // should be drawn first), with the poly_ids contained in the group elements
  // also in the correct relative z-order.
  GroupedElementsList ElementsInRegionByGroup(RegionQuery query) const;
  // Returns the GroupedElementList represented by a subset of POLY ElementIds.
  // The container should be something that contains ElementIds and provides
  // an iterator (begin() and end()) interface.
  // GroupedElementsList will be returned in z-order (the first GroupElements
  // should be drawn first), with the poly_ids contained in the group elements
  // also in the correct relative z-order.
  template <typename Container>
  GroupedElementsList GroupifyElements(const Container& elements);

  bool TopElementInRegion(const RegionQuery& query, ElementId* out) const;
  bool IsElementInRegion(const ElementId& id, const RegionQuery& query) const;

  // Returns the number of elements in the graph
  size_t NumElements() const { return num_elements_; }

  // How big at element will be rendered
  float Coverage(const Camera& cam, ElementId line_id) const;

  void AddListener(SceneGraphListener* listener);
  void RemoveListener(SceneGraphListener* listener);

  void RegisterForUpdates(UpdateListener* updatable);
  void UnregisterForUpdates(UpdateListener* updatable);

  ElementNotifier* GetElementNotifier() { return &element_notifier_; }

  // Returns true if we have data for this element.
  bool ElementExists(const ElementId& id, bool log_on_no_element = false) const;

  bool RenderedByMain(ElementId id) const;
  bool Visible(ElementId id) const;
  int Opacity(ElementId id) const;

  // Returns a raw view of the current per group element index.
  const GroupElementIdIndexMap& GetElementIndex() const;

  // CopyZIndex is used to snap the zindex state per group.
  using IdToZIndex = ElementIdHashMap<uint32_t>;
  using IdToZIndexPerGroup = ElementIdHashMap<IdToZIndex>;
  IdToZIndexPerGroup CopyZIndex() const;

  // Populates id_above with the ID of the element above id, within the same
  // group. If id is at the top of its group, id_above will be
  // kInvalidElementId.
  ABSL_MUST_USE_RESULT Status GetElementAbove(ElementId id,
                                              ElementId* id_above) const;

  // Returns true if the group's bounds should be considered for scissoring.
  bool IsClippableGroup(const GroupId& group_id) const;

  bool IsBulkLoading() const { return is_bulk_loading_; }
  void SetBulkLoading(bool bulk_loading) { is_bulk_loading_ = bulk_loading; }

 private:
  bool TryAddElementBelow(const ElementId& id, const GroupId& group,
                          const ElementId& below);

  // Internal helper function for inserting an element. This will take ownership
  // of element_to_add.processed_element. Note that this will dispatch
  // SceneGraphListener::PreElementAdded and SceneGraphListener::OnElementAdded,
  // but will not dispatch any events to the IElementListeners.
  ABSL_MUST_USE_RESULT Status AddSingleStrokeBelow(ElementAdd* element_to_add);

  bool AssociateElementId(const UUID& uuid, ElementId* result,
                          const ElementId& id);

  Rect MbrForRange(const std::vector<ElementId>::const_iterator start,
                   const std::vector<ElementId>::const_iterator end) const;

  // Returns true if we know about this element.
  //
  // Differs from elementExists() in that will return true even if the element
  // is only partially complete (ex id->uuid mapping, transform, etc)
  bool IsKnownId(const ElementId& id, bool log_on_unknown_id) const;
  void RemoveElementInternal(ElementId id, const SourceDetails source);
  ABSL_MUST_USE_RESULT Status AreIdsOkForAdd(ElementId id,
                                             const UUID& uuid) const;
  Rect ElementMbr(ElementId id, const glm::mat4& obj_to_world) const;

  // Walks over the specified elements and mutates them, tracking deltas for
  // modified getElementMetadata() and notifying SceneGraphListeners if
  // necessary
  //
  // "processElement" is executed for every element. The callee should perform
  // all scene mutations in the context of this callback
  //
  // If log_unknown_id is true, a warning will be logged if an ID is not found
  // in id_bimap_.
  template <typename ElementInputIt>
  void MutateElements(
      ElementInputIt begin_elements, ElementInputIt end_elements,
      std::function<ElementMutationType(ElementId, size_t)> process_element,
      const SourceDetails& source, bool log_unknown_id = true);

  // Walk over all the elements in the scene with respect to z-order.
  // For each element (group or poly), call visit_element with the parent of
  // the element and the  element id itself. For any group, call expand filter
  // with the group id to decide if the group should be expanded into.
  void WalkElementsInScene(GroupFilter expand_filter,
                           ElementVisitor visit_element) const;

  // Walk over all the elements in the scene with respect to z-order.
  // For each poly (in z-order), calls accept_element with the id of the poly
  // element. If that returns true, will add that poly to the grouped elements
  // list (in the correct grouped elements). For any group, call expand filter
  // with the group id to decide of the group should be expanded into.
  GroupedElementsList GroupElementsInSceneByWalk(
      GroupFilter expand_filter, ElementFilter accept_element) const;

  // Update the cached_mbr_
  void RecomputeMbr();

  std::shared_ptr<spatial::SpatialIndexFactoryInterface> spatial_index_factory_;
  std::unique_ptr<ElementIdSource> element_id_source_;
  std::shared_ptr<PolyStore> poly_store_;
  ElementNotifier element_notifier_;
  std::vector<std::shared_ptr<IDrawable>> drawables_;
  TransformMap transforms_;
  ElementIdHashMap<std::shared_ptr<spatial::SpatialIndex>>
      element_id_to_bounds_;
  ElementIdHashMap<bool> rendered_by_main_map_;
  ElementIdHashMap<ElementAttributes> attributes_;
  ElementIdHashMap<ColorModifier> color_modifier_;

  UUIDGenerator uuid_generator_;
  IdMap id_bimap_;  // maps element_id <-> UUID
  int num_elements_ = 0;
  // Id indexes by group. Should always be walked by first looking up the root
  // group (per_group_id_index_[kInvalidElementId])
  GroupElementIdIndexMap per_group_id_index_;
  GroupIdHashSet clippable_groups_;

  // Unlike ElementAttributes (which are intended to be immutable qualities of
  // an Element), the ElementProperties may change at runtime.
  struct ElementProperties {
    bool visible = true;
    int opacity = 255;
  };
  ElementIdHashMap<ElementProperties> element_properties_;

  std::shared_ptr<EventDispatch<SceneGraphListener>> sgl_dispatch_;
  std::shared_ptr<EventDispatch<UpdateListener>> update_dispatch_;

  Rect cached_mbr_;

  bool is_bulk_loading_{false};

  /**
   * Listens for scene graph events and updates the cached MBR as needed.
   */
  class MbrListener : public SceneGraphListener {
   public:
    MbrListener() {}
    void OnElementAdded(SceneGraph* graph, ElementId id) override {
      // MBR can only expand, no need to recompute the whole thing.
      Rect element_mbr = graph->Mbr({id});
      if (graph->cached_mbr_.Empty()) {
        graph->cached_mbr_ = element_mbr;
      } else {
        graph->cached_mbr_ = graph->cached_mbr_.Join(element_mbr);
      }
    }
    void OnElementsRemoved(
        SceneGraph* graph,
        const std::vector<SceneGraphRemoval>& removed_elements) override {
      graph->RecomputeMbr();
    }
    void OnElementsMutated(
        SceneGraph* graph,
        const std::vector<ElementMutationData>& mutation_data) override {
      graph->RecomputeMbr();
    }

   private:
    std::function<void()> callback_;
  };

  // Whenever the scene graph change, this listener will ensure the cached MBR
  // is recomputed.
  std::unique_ptr<MbrListener> mbr_listener_;

  friend class SEngineTestEnvWithHelpers;
  friend class MagicEraserTest;
};

////////////////////////////////////////////////////////////////////////////////
//                     Templated implementations                              //
////////////////////////////////////////////////////////////////////////////////
template <typename ElementInputIt>
void SceneGraph::MutateElements(
    ElementInputIt begin_elements, ElementInputIt end_elements,
    std::function<ElementMutationType(ElementId, size_t)> process_element,
    const SourceDetails& source, bool log_unknown_id) {
  std::vector<ElementMutationData> mutation_data;

  size_t i = 0;
  for (auto ai = begin_elements; ai != end_elements; ai++, i++) {
    ElementId id = *ai;
    if (!IsKnownId(id, log_unknown_id)) {
      continue;
    }
    auto original_data = GetElementMetadata(id);
    ElementMutationType type = process_element(id, i);
    if (ElementExists(id) && type != ElementMutationType::kNone) {
      mutation_data.push_back({type, original_data, GetElementMetadata(id)});
    }
  }

  if (!mutation_data.empty()) {
    sgl_dispatch_->Send(&SceneGraphListener::OnElementsMutated, this,
                        mutation_data);
    element_notifier_.OnElementsMutated(mutation_data, source);
  }
}

template <typename ElementInputIt>
void SceneGraph::RemoveElements(ElementInputIt begin_elements,
                                ElementInputIt end_elements,
                                const SourceDetails& source) {
  std::vector<SceneGraphRemoval> erased;
  std::vector<UUID> erased_uuids;
  for (auto ai = begin_elements; ai != end_elements; ai++) {
    ElementId id = *ai;
    SLOG(SLOG_DATA_FLOW, "removing element $0", id);
    if (!IsKnownId(id, true)) {
      SLOG(SLOG_WARNING, "$0 is NOT a known id", id);
      continue;
    }
    UUID uuid = UUIDFromElementId(id);
    GroupId parent = GetParentGroupId(id);
    UUID parent_uuid =
        parent == kInvalidElementId ? kInvalidUUID : UUIDFromElementId(parent);
    erased.emplace_back(SceneGraphRemoval{id, uuid, parent_uuid});
    erased_uuids.emplace_back(uuid);
    RemoveElementInternal(id, source);
  }
  sgl_dispatch_->Send(&SceneGraphListener::OnElementsRemoved, this, erased);
  if (!erased.empty()) {
    element_notifier_.OnElementsRemoved(erased_uuids, source);
  }
}

template <typename ElementInputIt, typename MatInputIt>
void SceneGraph::TransformElements(ElementInputIt begin_elements,
                                   ElementInputIt end_elements,
                                   MatInputIt begin_transforms,
                                   MatInputIt end_transforms,
                                   const SourceDetails& source) {
  ASSERT(std::distance(begin_transforms, end_transforms) ==
         std::distance(begin_elements, end_elements));
  MutateElements(
      begin_elements, end_elements,
      [this, begin_transforms](ElementId id, size_t i) {
        transforms_.Set(id, begin_transforms[i]);
        return ElementMutationType::kTransformMutation;
      },
      source);
}

template <typename ElementInputIt, typename BoolInputIt>
void SceneGraph::SetVisibilities(ElementInputIt begin_elements,
                                 ElementInputIt end_elements,
                                 BoolInputIt begin_visibilities,
                                 BoolInputIt end_visibilities,
                                 const SourceDetails& source_details) {
  ASSERT(std::distance(begin_elements, end_elements) ==
         std::distance(begin_visibilities, end_visibilities));
  MutateElements(
      begin_elements, end_elements,
      [this, begin_visibilities](ElementId id, size_t i) {
        this->element_properties_[id].visible = begin_visibilities[i];
        return ElementMutationType::kVisibilityMutation;
      },
      source_details);
}

template <typename ElementInputIt, typename IntInputIt>
void SceneGraph::SetOpacities(ElementInputIt begin_elements,
                              ElementInputIt end_elements,
                              IntInputIt begin_opacities,
                              IntInputIt end_opacities,
                              const SourceDetails& source_details) {
  ASSERT(std::distance(begin_elements, end_elements) ==
         std::distance(begin_opacities, end_opacities));
  MutateElements(
      begin_elements, end_elements,
      [this, begin_opacities](ElementId id, size_t i) {
        this->element_properties_[id].opacity = begin_opacities[i];
        return ElementMutationType::kOpacityMutation;
      },
      source_details);
}

template <typename ElementInputIt>
void SceneGraph::ChangeZOrders(ElementInputIt begin_elements,
                               ElementInputIt end_elements,
                               ElementInputIt begin_below_elements,
                               ElementInputIt end_below_elements,
                               const SourceDetails& source_details) {
  ASSERT(std::distance(begin_elements, end_elements) ==
         std::distance(begin_below_elements, end_below_elements));
  MutateElements(
      begin_elements, end_elements,
      [this, begin_below_elements](ElementId id, size_t i) {
        this->MoveGroup(id, begin_below_elements[i]);
        return ElementMutationType::kZOrderMutation;
      },
      source_details);
}

template <typename ElementIter, typename SpatialIndexIter>
void SceneGraph::SetSpatialIndices(ElementIter element_begin,
                                   ElementIter element_end,
                                   SpatialIndexIter index_begin,
                                   SpatialIndexIter index_end) {
  ASSERT(std::distance(element_begin, element_end) ==
         std::distance(index_begin, index_end));
  MutateElements(
      element_begin, element_end,
      [this, &index_begin](ElementId id, size_t i) {
        ASSERT(index_begin[i] != nullptr);
        element_id_to_bounds_[id] = std::move(index_begin[i]);
        return ElementMutationType::kNone;
      },
      SourceDetails::EngineInternal(), /* log_unknown_id */ false);
}

template <typename InputIt>
void SceneGraph::SetElementRenderedByMain(InputIt begin_elements,
                                          InputIt end_elements,
                                          bool rendered_by_main) {
  MutateElements(
      begin_elements, end_elements,
      [this, rendered_by_main](ElementId id, size_t i) {
        rendered_by_main_map_[id] = rendered_by_main;
        return ElementMutationType::kRenderedByMainMutation;
      },
      SourceDetails::EngineInternal());
}

template <typename OutputIt>
void SceneGraph::ElementsInScene(OutputIt output) const {
  ElementsInScene([this](const GroupId& id) { return RenderedByMain(id); },
                  [this](const ElementId& id) { return RenderedByMain(id); },
                  output);
}

template <typename OutputIt>
void SceneGraph::ElementsInScene(GroupFilter group_filter,
                                 ElementFilter element_filter,
                                 OutputIt output) const {
  WalkElementsInScene(
      group_filter,
      [element_filter, &output](const GroupId& parent, const ElementId& id) {
        if (element_filter(id)) {
          *output++ = id;
        }
      });
}

template <typename OutputIt>
void SceneGraph::ElementsInRegion(const RegionQuery& query,
                                  OutputIt output) const {
  auto group_query = query;
  group_query.SetAllowedTypes({GROUP});
  ElementsInScene(
      [this, &group_query](const GroupId& id) {
        return IsElementInRegion(id, group_query);
      },
      [this, &query](const ElementId& id) {
        return IsElementInRegion(id, query);
      },
      output);
}

template <typename Container>
SceneGraph::GroupedElementsList SceneGraph::GroupifyElements(
    const Container& elements) {
  GroupIdHashSet groups_to_accept;
  ElementIdHashSet elements_to_accept;

  for (auto id : elements) {
    if (id.Type() == GROUP) {
      groups_to_accept.insert(id);
    } else {
      elements_to_accept.insert(id);
    }
    auto parent = GetParentGroupId(id);
    if (parent != kInvalidElementId) {
      groups_to_accept.insert(parent);
    }
  }

  // For every group, look for its parent and expand any that are not found.
  GroupIdHashSet added_groups = groups_to_accept;
  while (!added_groups.empty()) {
    GroupIdHashSet next_added_groups;
    for (const auto& id : added_groups) {
      // Add all parents of the added groups that aren't yet in groups to
      // accept.
      auto parent = GetParentGroupId(id);
      if (parent != kInvalidElementId &&
          groups_to_accept.find(parent) == groups_to_accept.end()) {
        next_added_groups.insert(parent);
        groups_to_accept.insert(parent);
      }
    }
    added_groups = std::move(next_added_groups);
  }

  return GroupElementsInSceneByWalk(
      [&groups_to_accept](const GroupId& id) {
        return groups_to_accept.find(id) != groups_to_accept.end();
      },
      [&elements_to_accept](const ElementId& id) {
        return elements_to_accept.find(id) != elements_to_accept.end();
      });
}

template <typename OutputIt>
void SceneGraph::GroupChildrenOfRoot(OutputIt output) const {
  WalkElementsInScene(
      [](const GroupId& group_id) { return group_id == kInvalidElementId; },
      [&output](const GroupId& parent_id, const ElementId& id) {
        if (parent_id == kInvalidElementId && id.Type() == ElementType::GROUP) {
          *output++ = id;
        }
      });
}

}  // namespace ink

#endif  // INK_ENGINE_SCENE_GRAPH_SCENE_GRAPH_H_
