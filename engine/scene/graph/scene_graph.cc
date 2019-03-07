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

#include "ink/engine/scene/graph/scene_graph.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"
#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/sticker_spatial_index_factory_interface.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/graph/element_notifier.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/rand_funcs.h"

namespace ink {

using glm::vec4;

SceneGraph::SceneGraph(
    std::shared_ptr<PolyStore> poly_store,
    std::shared_ptr<IElementListener> element_listener,
    std::shared_ptr<spatial::StickerSpatialIndexFactoryInterface>
        sticker_spatial_index_factory)
    : sticker_spatial_index_factory_(std::move(sticker_spatial_index_factory)),
      element_id_source_(new ElementIdSource(1)),
      poly_store_(std::move(poly_store)),
      element_notifier_(element_listener),
      uuid_generator_(U64rand()),
      sgl_dispatch_(new EventDispatch<SceneGraphListener>()),
      update_dispatch_(new EventDispatch<UpdateListener>()) {
  // Instantiate the root.
  per_group_id_index_[kInvalidElementId] = std::make_shared<ElementIdIndex>();
  sticker_spatial_index_factory_->SetSceneGraph(this);
  mbr_listener_ = absl::make_unique<MbrListener>();
  mbr_listener_->RegisterOnDispatch(sgl_dispatch_);
}

SceneGraph::~SceneGraph() {
  sticker_spatial_index_factory_->SetSceneGraph(nullptr);
  SLOG(SLOG_OBJ_LIFETIME, "sceneGraph dtor");
}

bool SceneGraph::AssociateElementId(const UUID& uuid, ElementId* result,
                                    const ElementId& id) {
  *result = kInvalidElementId;
  if (id_bimap_.Contains(uuid)) {
    SLOG(SLOG_ERROR, "attempting to remap uuid $0 to a new element $1", uuid,
         id.ToStringExtended());
    return false;
  }

  ASSERT(!id_bimap_.Contains(id));
  id_bimap_.Insert(uuid, id);
  *result = id;
  return true;
}

bool SceneGraph::GetNextPolyId(const UUID& uuid, ElementId* result) {
  return AssociateElementId(uuid, result, element_id_source_->CreatePolyId());
}

bool SceneGraph::GetNextGroupId(const UUID& uuid, GroupId* result) {
  return AssociateElementId(uuid, result, element_id_source_->CreateGroupId());
}

Status SceneGraph::AreIdsOkForAdd(ElementId id, const UUID& uuid) const {
  if (ElementExists(id)) {
    return ErrorStatus(StatusCode::ALREADY_EXISTS,
                       "Got a repeat add for the same UUID = $0", uuid);
  }
  if (id == kInvalidElementId || uuid == kInvalidUUID) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Attempting to add an invalid id!");
  }
  if (id_bimap_.Contains(id) && id_bimap_.GetUUID(id) != uuid) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Attempting to remap id $0 to uuid $1! (id already mapped)", id, uuid);
  }
  if (id_bimap_.Contains(uuid) && id_bimap_.GetElementId(uuid) != id) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Attempting to remap id $0 to uuid $1! (uuid already mapped)", id,
        uuid);
  }
  return OkStatus();
}

bool SceneGraph::TryAddElementBelow(const ElementId& id, const GroupId& group,
                                    const ElementId& below) {
  auto below_parent = GetParentGroupId(below);
  if (below_parent != group) {
    // Ignore the below element with id hint and add to the group
    // specified.
    SLOG(SLOG_ERROR,
         "below_element $0 specified with an inconsistent parent $1. "
         "Expecting group: $2 ",
         below, below_parent, group);
    return false;
  }
  if (!per_group_id_index_[group]->Contains(below)) {
    SLOG(SLOG_ERROR, "request to add below id $0, but it was not found", below);
    return false;
  }
  per_group_id_index_[group]->AddBelow(id, below);
  ++num_elements_;
  return true;
}

Status SceneGraph::AddSingleStrokeBelow(ElementAdd* element_to_add) {
  if (element_to_add->processed_element == nullptr ||
      element_to_add->serialized_element == nullptr) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Invalid arguments to AddSingleStrokeBelow");
  }

  auto processed_element = std::move(element_to_add->processed_element);
  const UUID& uuid = element_to_add->serialized_element->uuid;
  ElementId id = processed_element->id;
  INK_RETURN_UNLESS(AreIdsOkForAdd(id, uuid));

  GroupId group = processed_element->group;
  auto id_from_uuid =
      ElementIdFromUUID(element_to_add->serialized_element->uuid);
  if (id != id_from_uuid) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Processed element id $0 doesn't match serialized element id $1", id,
        id_from_uuid);
  }

  if (per_group_id_index_.count(group) == 0) {
    // Create the group index if it doesn't exist yet.
    per_group_id_index_[group] = std::make_shared<ElementIdIndex>();
  }
  SLOG(SLOG_DATA_FLOW,
       "Adding line to scene graph, id: $0, uuid $1, belowId: $2, group: $3",
       id, uuid, element_to_add->id_to_add_below, group);

  // Existing transforms take precedence over adds. Note that the group
  // doesn't get set if the transform already exists.
  if (!transforms_.Contains(id)) {
    transforms_.Set(id, group, processed_element->obj_to_group);
  }

  sgl_dispatch_->Send(&SceneGraphListener::PreElementAdded,
                      processed_element.get(), transforms_.ObjToWorld(id));
  if (processed_element->attributes.is_sticker) {
    element_id_to_bounds_[id] =
        sticker_spatial_index_factory_->CreateSpatialIndex(*processed_element);
  } else {
    element_id_to_bounds_[id] = std::move(processed_element->spatial_index);
  }

  ASSERT(element_id_to_bounds_[id]->Mbr(glm::mat4(1)).Area() > 0);
  id_bimap_.Insert(uuid, id);
  attributes_[id] = processed_element->attributes;
  element_properties_[id] = ElementProperties{};
  poly_store_->Add(id, std::move(processed_element->mesh));
  if (element_to_add->id_to_add_below == kInvalidElementId ||
      !TryAddElementBelow(id, group, element_to_add->id_to_add_below)) {
    per_group_id_index_[group]->AddToTop(id);
    ++num_elements_;
  }

  sgl_dispatch_->Send(&SceneGraphListener::OnElementAdded, this, id);
  return OkStatus();
}

void SceneGraph::AddStroke(ElementAdd element_to_add) {
  Status s = AddSingleStrokeBelow(&element_to_add);
  if (!s.ok()) {
    SLOG(SLOG_ERROR, s.error_message());
    return;
  }

  // The notifier is what sends the element to the document storage +
  // redo/undo stack.
  auto source_details = element_to_add.serialized_element->source_details;
  std::vector<SerializedElement> serialized_elements;
  serialized_elements.push_back(std::move(*element_to_add.serialized_element));
  element_notifier_.OnElementsAdded(serialized_elements, kInvalidUUID,
                                    source_details);
}

void SceneGraph::AddStrokes(std::vector<ElementAdd> elements_to_add) {
  std::vector<SerializedElement> serialized_elements;
  for (auto& element_to_add : elements_to_add) {
    Status s = AddSingleStrokeBelow(&element_to_add);
    if (!s.ok()) {
      SLOG(SLOG_ERROR, s.error_message());
      return;
    }

    serialized_elements.push_back(
        std::move(*element_to_add.serialized_element));
  }

  // The notifier is what sends the element to the document storage + redo/undo
  // stack.
  element_notifier_.OnElementsAdded(serialized_elements, kInvalidUUID,
                                    serialized_elements[0].source_details);
}

namespace {
SerializedElement SerializeGroupElement(
    SceneGraph* graph, GroupId group_id,
    const glm::mat4 group_to_world_transform, SourceDetails source_details) {
  SerializedElement serialized_group(
      graph->UUIDFromElementId(group_id), kInvalidUUID, source_details,
      graph->GetElementNotifier()->GetCallbackFlags(source_details));
  serialized_group.bundle = absl::make_unique<proto::ElementBundle>();
  ink::util::WriteToProto(serialized_group.bundle->mutable_transform(),
                          group_to_world_transform);
  serialized_group.bundle->set_uuid(graph->UUIDFromElementId(group_id));
  serialized_group.bundle->mutable_element()->set_minimum_serializer_version(1);
  serialized_group.bundle->mutable_element()
      ->mutable_attributes()
      ->set_is_group(true);
  return serialized_group;
}

}  // namespace

void SceneGraph::AddOrUpdateGroup(GroupId group_id,
                                  glm::mat4 group_to_world_transform,
                                  Rect bounds, bool clippable,
                                  GroupType group_type,
                                  SourceDetails source_details) {
  ASSERT(group_id.Type() == GROUP);
  bool added_new = false;
  if (per_group_id_index_.count(group_id) == 0) {
    added_new = true;
    per_group_id_index_[kInvalidElementId]->AddToTop(group_id);
    per_group_id_index_[group_id] = std::make_shared<ElementIdIndex>();
    ++num_elements_;
    Mesh group_mesh;
    MakeRectangleMesh(&group_mesh, bounds);
    element_id_to_bounds_[group_id] =
        absl::make_unique<spatial::MeshRTree>(group_mesh);
    transforms_.Set(group_id, kInvalidElementId, group_to_world_transform);
  } else {
    if (!transforms_.Contains(group_id) ||
        transforms_.ObjToWorld(group_id) != group_to_world_transform) {
      TransformElement(group_id, group_to_world_transform,
                       SourceDetails::FromEngine());
    }
    auto bounds_it = element_id_to_bounds_.find(group_id);
    if (bounds_it == element_id_to_bounds_.end() ||
        bounds_it->second->Mbr(group_to_world_transform) != bounds) {
      Mesh group_mesh;
      MakeRectangleMesh(&group_mesh, bounds);
      element_id_to_bounds_[group_id] =
          absl::make_unique<spatial::MeshRTree>(group_mesh);
    }
  }

  attributes_[group_id].group_type = group_type;
  attributes_[group_id].selectable = false;

  if (clippable) {
    clippable_groups_.insert(group_id);
  } else {
    clippable_groups_.erase(group_id);
  }
  if (added_new) {
    sgl_dispatch_->Send(&SceneGraphListener::OnElementAdded, this, group_id);

    auto serialized_group = SerializeGroupElement(
        this, group_id, group_to_world_transform, source_details);
    serialized_group.bundle->mutable_element()
        ->mutable_attributes()
        ->set_group_type(GroupTypeToProto(group_type));
    std::vector<SerializedElement> groups;
    groups.push_back(std::move(serialized_group));
    element_notifier_.OnElementsAdded(groups, kInvalidUUID,
                                      groups[0].source_details);
  }
}

void SceneGraph::MoveGroup(GroupId group_id, GroupId before_group) {
  // Moving a group before itself is a no-op.
  if (group_id == kInvalidElementId || group_id == before_group) return;

  // There is an assumption here that all groups are children of the root.
  auto root_index = per_group_id_index_[kInvalidElementId];
  ASSERT(root_index->Contains(group_id));
  ASSERT(before_group == kInvalidElementId ||
         root_index->Contains(before_group));
  root_index->Remove(group_id);

  if (before_group == kInvalidElementId) {
    root_index->AddToTop(group_id);
  } else {
    root_index->AddBelow(group_id, before_group);
  }
}

void SceneGraph::SetParent(ElementId element_id, GroupId group_id) {
  ASSERT(IsKnownId(element_id, /* log_on_unknown_id= */ true));
  ASSERT(group_id.Type() == GROUP || group_id == kInvalidElementId);

  // Currently, only one level of groups is supported.
  ASSERT(element_id.Type() != GROUP);

  auto last_group_id = GetParentGroupId(element_id);
  auto obj_to_world = transforms_.ObjToWorld(element_id);
  per_group_id_index_[last_group_id]->Remove(element_id);

  auto group_to_world = transforms_.ObjToWorld(group_id);
  auto obj_to_group = glm::inverse(group_to_world) * obj_to_world;
  // Preserve the last obj-to-world transform.
  transforms_.Set(element_id, group_id, obj_to_group);
  per_group_id_index_[group_id]->AddToTop(element_id);
}

void SceneGraph::RemoveElement(ElementId id, SourceDetails source) {
  RemoveElements(&id, &id + 1, source);
}

void SceneGraph::RemoveAllElements(SourceDetails source) {
  SLOG(SLOG_DATA_FLOW, "removing all elements");
  // Remove poly elements first and then groups.
  std::vector<ElementId> to_remove_polys;
  std::vector<GroupId> to_remove_groups;
  for (auto kv : id_bimap_) {
    if (kv.first.Type() == POLY) {
      to_remove_polys.push_back(kv.first);
    } else {
      to_remove_groups.push_back(kv.first);
    }
  }
  RemoveElements(to_remove_polys.begin(), to_remove_polys.end(), source);
  RemoveElements(to_remove_groups.begin(), to_remove_groups.end(), source);
}

void SceneGraph::RemoveAllSelectableElements() {
  SLOG(SLOG_DATA_FLOW, "removing all user elements");
  std::vector<ElementId> to_remove;
  ElementsInScene([](const GroupId& any) { return true; },
                  [this](const ElementId& id) {
                    auto a = attributes_.find(id);
                    return a != attributes_.end() && a->second.selectable;
                  },
                  std::back_inserter(to_remove));
  RemoveElements(to_remove.begin(), to_remove.end(),
                 SourceDetails::FromEngine());
}

void SceneGraph::RemoveElementInternal(ElementId id,
                                       const SourceDetails source) {
  SLOG(SLOG_DATA_FLOW, "scenegraph removing element $0", id);
  switch (id.Type()) {
    case POLY:
      poly_store_->Remove(id);
      break;
    case GROUP: {
      // Remove all of the group's children before removing the group.
      //
      // We have to copy the vector out of the index, since we will be
      // modifying it as we delete children.
      std::vector<GroupId> children =
          per_group_id_index_[id]->SortedElements().AsValueVector<GroupId>();
      RemoveElements(children.begin(), children.end(), source);
    } break;
    default:
      UNHANDLED_ELEMENT_TYPE(id);
  }
  auto parent = GetParentGroupId(id);
  transforms_.Remove(id);
  id_bimap_.Remove(id);
  element_id_to_bounds_.erase(id);
  color_modifier_.erase(id);
  per_group_id_index_[parent]->Remove(id);
  --num_elements_;
  if (id.Type() == GROUP) {
    clippable_groups_.erase(id);
    per_group_id_index_.erase(id);
  }
}

void SceneGraph::ReplaceElements(
    std::vector<ElementAdd> elements_to_add,
    const std::vector<ElementId>& elements_to_remove,
    const SourceDetails source_details) {
  // Insert the new elements. This must be done before removing existing
  // elements, in case any of the new elements are to be added below an element
  // that will be removed.
  std::vector<SerializedElement> element_bundles_to_add;
  std::vector<UUID> uuids_to_add_below;
  element_bundles_to_add.reserve(elements_to_add.size());
  uuids_to_add_below.reserve(elements_to_add.size());
  for (auto& element_to_add : elements_to_add) {
    // Insert the new element. This will dispatch a call to
    // SceneGraphListener::OnElementAdded().
    Status status = AddSingleStrokeBelow(&element_to_add);
    if (!status.ok()) {
      SLOG(SLOG_ERROR, status.error_message());
      return;
    }

    element_bundles_to_add.emplace_back(
        std::move(*element_to_add.serialized_element));
    if (element_to_add.id_to_add_below == kInvalidElementId) {
      uuids_to_add_below.emplace_back(kInvalidUUID);
    } else {
      uuids_to_add_below.emplace_back(
          UUIDFromElementId(element_to_add.id_to_add_below));
    }
  }

  // Remove existing elements.
  std::vector<UUID> removed_uuids;
  std::vector<SceneGraphRemoval> removed;
  removed_uuids.reserve(elements_to_remove.size());
  for (const auto& element_to_remove : elements_to_remove) {
    if (element_to_remove.Type() != ElementType::POLY) {
      SLOG(SLOG_WARNING, "Skipping $0: Groups cannot be replaced.",
           element_to_remove);
      continue;
    } else if (!ElementExists(element_to_remove)) {
      SLOG(SLOG_WARNING, "Skipping $0: element not found.", element_to_remove);
      continue;
    }

    // Remove the old element. This does not dispatch a call to the
    // SceneGraphListeners -- instead, we aggregate the removed IDs into one
    // call.
    UUID uuid = UUIDFromElementId(element_to_remove);
    GroupId parent_id = GetParentGroupId(element_to_remove);
    UUID parent_uuid = parent_id == kInvalidElementId
                           ? kInvalidUUID
                           : UUIDFromElementId(parent_id);
    removed.emplace_back(
        SceneGraphRemoval{element_to_remove, uuid, parent_uuid});
    removed_uuids.emplace_back(uuid);

    RemoveElementInternal(element_to_remove, source_details);
  }

  sgl_dispatch_->Send(&SceneGraphListener::OnElementsRemoved, this, removed);
  element_notifier_.OnElementsReplaced(removed_uuids, element_bundles_to_add,
                                       uuids_to_add_below, source_details);
}

void SceneGraph::TransformElement(ElementId id, glm::mat4 new_transform,
                                  const SourceDetails& source_details) {
  TransformElements(&id, &id + 1, &new_transform, &new_transform + 1,
                    source_details);
}

void SceneGraph::SetElementRenderedByMain(ElementId id, bool rendered_by_main) {
  SetElementRenderedByMain(&id, &id + 1, rendered_by_main);
}

ElementMetadata SceneGraph::GetElementMetadata(ElementId id) const {
  if (!IsKnownId(id, true)) {
    SLOG(SLOG_ERROR, "$0 id not known id", id);
    return ElementMetadata();
  }

  glm::mat4 obj_to_group = glm::mat4(1);
  glm::mat4 obj_to_world = glm::mat4(1);
  glm::mat4 group_to_world = glm::mat4(1);
  if (transforms_.Contains(id)) {
    obj_to_group = transforms_.ObjToGroup(id);
    obj_to_world = transforms_.ObjToWorld(id);
    auto group_id = transforms_.GetGroup(id);
    group_to_world = group_id == kInvalidElementId
                         ? glm::mat4(1)
                         : transforms_.ObjToWorld(group_id);
  }
  auto a = attributes_.find(id);
  auto attributes = (a != attributes_.end()) ? a->second : ElementAttributes();
  auto c = color_modifier_.find(id);
  auto color_modifier =
      c != color_modifier_.end() ? c->second : ColorModifier();
  auto p = element_properties_.find(id);
  auto properties =
      (p != element_properties_.end()) ? p->second : ElementProperties{};
  return ElementMetadata(id, id_bimap_.GetUUID(id), obj_to_world, obj_to_group,
                         group_to_world, RenderedByMain(id), attributes,
                         color_modifier, transforms_.GetGroup(id),
                         properties.visible, properties.opacity);
}

GroupId SceneGraph::GetParentGroupId(ElementId id) const {
  if (!IsKnownId(id, false)) {
    return kInvalidElementId;
  }
  return transforms_.GetGroup(id);
}

glm::vec4 SceneGraph::GetColor(ElementId id) {
  ASSERT(id.Type() == POLY);
  auto c = color_modifier_.find(id);
  auto color_modifier =
      c != color_modifier_.end() ? c->second : ColorModifier();

  // If the ColorModifier is a replacement modifier (multiplying the base
  // color by 0) then we can return the add part as the final color without
  // looking up the optmesh.
  if (color_modifier.mul == glm::vec4(0)) {
    return color_modifier.add;
  }

  // If the ColorModifier wasn't a replacement modifier, we have to pull the
  // optmesh to compute the color.
  if (!ElementExists(id, true)) return glm::vec4{0, 0, 0, 0};

  OptimizedMesh* mesh;
  if (!poly_store_->Get(id, &mesh)) {
    return glm::vec4{0, 0, 0, 0};
  }

  return color_modifier.Apply(mesh->color);
}

bool SceneGraph::GetTextureUri(ElementId id, std::string* uri) {
  if (id.Type() != POLY) {
    // Non-POLYs don't have a texture.
    return false;
  }
  OptimizedMesh* mesh;
  if (ElementExists(id, true) && poly_store_->Get(id, &mesh) &&
      mesh->type == ShaderType::TexturedVertShader) {
    ASSERT(mesh->texture != nullptr);
    *uri = mesh->texture->uri;
    return true;
  } else {
    return false;
  }
}

void SceneGraph::SetColor(ElementId id, glm::vec4 rgba, SourceDetails source) {
  MutateElements(
      &id, &id + 1,
      [this, rgba](ElementId id, size_t i) {
        color_modifier_[id] = ColorModifier(glm::vec4(0, 0, 0, 0), rgba);
        return ElementMutationType::kColorMutation;
      },
      source);
}

void SceneGraph::Update(const Camera& cam) {
  update_dispatch_->Send(&UpdateListener::Update, cam);
}

void SceneGraph::AddDrawable(std::shared_ptr<IDrawable> drawable) {
  ASSERT(drawable != nullptr);

  SLOG(SLOG_DATA_FLOW, "adding drawable $0", AddressStr(drawable.get()));
  auto existing = std::find(drawables_.begin(), drawables_.end(), drawable);
  if (existing != drawables_.end()) {
    SLOG(SLOG_ERROR,
         "attempting to add drawable $0 to the scene, but it's "
         "already been added!",
         AddressStr(drawable.get()));
  } else {
    drawables_.push_back(drawable);
  }
}

void SceneGraph::RemoveDrawable(IDrawable* drawable) {
  ASSERT(drawable);
  SLOG(SLOG_DATA_FLOW, "removing drawable $0", AddressStr(drawable));

  auto existing =
      std::find_if(drawables_.begin(), drawables_.end(),
                   [&drawable](const std::shared_ptr<IDrawable>& d) {
                     return d.get() == drawable;
                   });

  if (existing != drawables_.end()) {
    drawables_.erase(existing);
  }
}

const std::vector<std::shared_ptr<IDrawable>>& SceneGraph::GetDrawables()
    const {
  return drawables_;
}

bool SceneGraph::TopElementInRegion(const RegionQuery& query,
                                    ElementId* out) const {
  std::queue<GroupId> to_process;
  to_process.emplace(kInvalidElementId);  // root.
  while (!to_process.empty()) {
    auto group_id = to_process.front();
    to_process.pop();
    auto group_iter = per_group_id_index_.find(group_id);
    if (group_iter == per_group_id_index_.end()) {
      SLOG(SLOG_ERROR, "Found group $0 but no per group index", group_id);
      continue;
    }
    const auto& group = group_iter->second;
    for (auto id : group->ReverseSortedElements()) {
      if (IsElementInRegion(id, query)) {
        *out = id;
        return true;
      }
      if (id.Type() == GROUP) {
        to_process.emplace(id);
      }
    }
  }
  return false;
}

bool SceneGraph::IsElementInRegion(const ElementId& id,
                                   const RegionQuery& query) const {
  ASSERT(IsKnownId(id, true));
  if (!RenderedByMain(id) || !Visible(id)) {
    return false;
  }
  const auto& filter = query.CustomFilter();
  if (filter && !filter(id)) {
    return false;
  }
  const auto& type_filter = query.TypeFilter();
  if (!type_filter.empty() &&
      type_filter.find(id.Type()) == type_filter.end()) {
    return false;
  }

  // Check the parent group to see if it matches the group filter.
  // When hierarchical groups are supported, this will need to check
  // for any ancestor.
  if (query.GroupFilter() != kInvalidElementId && id != query.GroupFilter() &&
      GetParentGroupId(id) != query.GroupFilter()) {
    return false;
  }

  const spatial::SpatialIndex& spi = *(element_id_to_bounds_.find(id)->second);
  if (id.Type() == GROUP && spi.Mbr(transforms_.WorldToObj(id)).Area() == 0) {
    // Zero-area groups are Layers and always pass the intersection test.
    return true;
  }
  return spi.Intersects(query.Region(),
                        transforms_.WorldToObj(id) * query.Transform());
}

bool SceneGraph::RenderedByMain(ElementId id) const {
  ASSERT(IsKnownId(id, true));
  auto ai = rendered_by_main_map_.find(id);
  if (ai == rendered_by_main_map_.end()) return true;
  return ai->second;
}

bool SceneGraph::Visible(ElementId id) const {
  ASSERT(IsKnownId(id, true));
  auto p = element_properties_.find(id);
  return (p == element_properties_.end()) ? true : p->second.visible;
}

int SceneGraph::Opacity(ElementId id) const {
  ASSERT(IsKnownId(id, true));
  auto p = element_properties_.find(id);
  return (p == element_properties_.end()) ? 255 : p->second.opacity;
}

bool SceneGraph::GetMesh(ElementId id, OptimizedMesh** mesh) const {
  ASSERT(id.Type() == POLY);
  if (!ElementExists(id, true)) return false;

  if (poly_store_->Get(id, mesh)) {
    (*mesh)->object_matrix = transforms_.ObjToWorld(id);
    auto c = color_modifier_.find(id);
    if (c != color_modifier_.end()) {
      (*mesh)->mul_color_modifier = c->second.mul;
      (*mesh)->add_color_modifier = c->second.add;
    }
    return true;
  }
  return false;
}

std::shared_ptr<const spatial::SpatialIndex> SceneGraph::GetSpatialIndex(
    ElementId id) const {
  auto iter = element_id_to_bounds_.find(id);
  if (iter == element_id_to_bounds_.end()) {
    return nullptr;
  }

  return iter->second;
}

void SceneGraph::SetSpatialIndex(ElementId id,
                                 std::shared_ptr<spatial::SpatialIndex> index) {
  ASSERT(index != nullptr);
  SetSpatialIndices(&id, &id + 1, &index, &index + 1);
}

Rect SceneGraph::MbrForRange(
    const std::vector<ElementId>::const_iterator start,
    const std::vector<ElementId>::const_iterator end) const {
  Rect res;
  bool found_any = false;
  for (auto el = start; el != end; ++el) {
    auto mbr = ElementMbr(*el, transforms_.ObjToWorld(*el));
    if (found_any) {
      res = res.Join(mbr);
    } else {
      res = mbr;
      found_any = true;
    }
  }
  return res;
}

Rect SceneGraph::Mbr(const std::vector<ElementId>& elements) const {
  return MbrForRange(elements.begin(), elements.end());
}

Rect SceneGraph::MbrForGroup(GroupId group_id) const {
  auto group_iter = per_group_id_index_.find(group_id);
  if (group_iter == per_group_id_index_.end()) {
    SLOG(SLOG_WARNING, "computing Mbr for group $0, but it was not found",
         group_id);
    return Rect();
  }

  auto element_ids = group_iter->second->SortedElements();
  return MbrForRange(element_ids.begin(), element_ids.end());
}

Rect SceneGraph::MbrObjCoords(ElementId element) const {
  return ElementMbr(element, glm::mat4(1));
}

Rect SceneGraph::ElementMbr(ElementId id, const glm::mat4& obj_to_world) const {
  const auto& br = element_id_to_bounds_.find(id);
  ASSERT(br != element_id_to_bounds_.end());
  return br->second->Mbr(obj_to_world);
}

Rect SceneGraph::Mbr() const {
  if (!cached_mbr_) {
    std::vector<ElementId> all_ids;
    ElementsInScene(std::back_inserter(all_ids));
    cached_mbr_ = Mbr(all_ids);
  }
  return *cached_mbr_;
}

float SceneGraph::Coverage(const Camera& cam, ElementId line_id) const {
  return cam.Coverage(element_id_to_bounds_.at(line_id)
                          ->Mbr(transforms_.ObjToWorld(line_id))
                          .Width());
}

void SceneGraph::AddListener(SceneGraphListener* listener) {
  listener->RegisterOnDispatch(sgl_dispatch_);
}

void SceneGraph::RemoveListener(SceneGraphListener* listener) {
  listener->Unregister(sgl_dispatch_);
}

void SceneGraph::RegisterForUpdates(UpdateListener* updatable) {
  updatable->RegisterOnDispatch(update_dispatch_);
}

void SceneGraph::UnregisterForUpdates(UpdateListener* updatable) {
  updatable->Unregister(update_dispatch_);
}

bool SceneGraph::IsKnownId(const ElementId& id, bool log_on_unknown_id) const {
  bool res = id_bimap_.Contains(id);
  if (!res && log_on_unknown_id) {
    SLOG(SLOG_WARNING, "looking for id $0, but it was not found", id);
  }
  return res;
}

bool SceneGraph::ElementExists(const ElementId& id,
                               bool log_on_no_element) const {
  const auto& ai = element_id_to_bounds_.find(id);
  bool has_bounds = ai != element_id_to_bounds_.end();
  auto group = GetParentGroupId(id);
  auto group_iter = per_group_id_index_.find(group);
  bool in_group = group_iter != per_group_id_index_.end() &&
                  group_iter->second->Contains(id);
  ASSERT(!has_bounds || ai->second != nullptr);
  ASSERT(!has_bounds || transforms_.Contains(id));
  ASSERT(!has_bounds || id_bimap_.Contains(id));
  ASSERT(!has_bounds || in_group);
  if (log_on_no_element && !has_bounds) {
    SLOG(SLOG_WARNING,
         "attempted operation on id $0 (parent group $1, in group $2), but it "
         "was not found",
         id, group, in_group);
  }
  return has_bounds;
}

ElementId SceneGraph::ElementIdFromUUID(const UUID& id) const {
  if (!id_bimap_.Contains(id)) {
    SLOG(SLOG_WARNING,
         "Attempting to find the ElementId corresponding to uuid $0, but no "
         "mapping was found. (Did you call getNext*Id(uuid)?",
         id);
    return kInvalidElementId;
  }
  return id_bimap_.GetElementId(id);
}

ElementId SceneGraph::PolyIdFromUUID(const UUID& id) const {
  auto elem = ElementIdFromUUID(id);
  if (elem == kInvalidElementId) {
    return elem;
  }
  ASSERT(elem.Type() == POLY);
  if (elem.Type() != POLY) {
    return kInvalidElementId;
  }
  return elem;
}

ElementId SceneGraph::GroupIdFromUUID(const UUID& id) const {
  auto elem = ElementIdFromUUID(id);
  if (elem == kInvalidElementId) {
    return elem;
  }
  ASSERT(elem.Type() == GROUP);
  if (elem.Type() != GROUP) {
    return kInvalidElementId;
  }
  return elem;
}

UUID SceneGraph::UUIDFromElementId(const ElementId& id) const {
  if (id == kInvalidElementId) {
    return kInvalidUUID;
  }
  if (!id_bimap_.Contains(id)) {
    SLOG(SLOG_WARNING,
         "Attempting to find the uuid corresponding to ElementId $0, but no "
         "mapping was found.",
         id);
    return kInvalidUUID;
  }
  return id_bimap_.GetUUID(id);
}

void SceneGraph::WalkElementsInScene(GroupFilter expand_filter,
                                     ElementVisitor visit_element) const {
  // Expand the root.
  auto root_index_iter = per_group_id_index_.find(kInvalidElementId);
  const auto& root = root_index_iter->second;
  ASSERT(root_index_iter != per_group_id_index_.end());
  std::stack<std::pair<GroupId, ElementId>> to_process;
  // The elements are in order A, B, C, D (D should be drawn on top).
  // We want to add them in such that the order is preserved (A drawn
  // first).
  for (auto id : root->ReverseSortedElements()) {
    to_process.emplace(std::make_pair(kInvalidElementId, id));
  }
  while (!to_process.empty()) {
    const auto& id_pair = to_process.top();
    auto parent = id_pair.first;
    auto id = id_pair.second;
    to_process.pop();
    visit_element(parent, id);
    if (id.Type() == GROUP && expand_filter(id)) {
      auto group_iter = per_group_id_index_.find(id);
      if (group_iter == per_group_id_index_.end()) {
        SLOG(SLOG_ERROR, "Found group $0 but no per group index", id);
        continue;
      }
      const auto& group = group_iter->second;
      // The elements are in order A, B, C, D (D should be drawn on top).
      // Note that to_process may have other elements, say E, F.
      // We want the final order to be A, B, C, D, E, F, since this group
      // should be processed immediately in-order.
      // We want to add them in such that the order is preserved, thus walking
      // the group elements in _reverse_ and pushing the elements in front.
      for (auto sub_id : group->ReverseSortedElements()) {
        to_process.emplace(std::make_pair(id, sub_id));
      }
    }
  }
}

SceneGraph::GroupedElementsList SceneGraph::GroupElementsInSceneByWalk(
    GroupFilter expand_filter, ElementFilter accept_element) const {
  GroupedElementsList result;
  GroupedElements grouped_elements;
  grouped_elements.group_id = kInvalidElementId;
  WalkElementsInScene(
      expand_filter, [this, &accept_element, &result, &grouped_elements](
                         const GroupId& parent, const ElementId& id) {
        if (id.Type() != POLY) {
          return;
        }
        if (!accept_element(id)) {
          return;
        }
        if (parent != grouped_elements.group_id) {
          if (!grouped_elements.poly_ids.empty()) {
            result.emplace_back(grouped_elements);
          }
          grouped_elements.group_id = parent;
          if (parent != kInvalidElementId && IsClippableGroup(parent)) {
            grouped_elements.bounds = Mbr({parent});
          } else {
            grouped_elements.bounds = Rect(0, 0, 0, 0);
          }
          grouped_elements.poly_ids.clear();
        }
        grouped_elements.poly_ids.emplace_back(id);
      });
  // Insert the last grouped elements found.
  if (!grouped_elements.poly_ids.empty()) {
    result.emplace_back(std::move(grouped_elements));
  }
  return result;
}

SceneGraph::GroupedElementsList SceneGraph::ElementsInRegionByGroup(
    RegionQuery query) const {
  auto group_query = query;
  group_query.SetAllowedTypes({GROUP});
  return GroupElementsInSceneByWalk(
      [this, &group_query](const GroupId& id) {
        // only expand groups based on Rect.
        return IsElementInRegion(id, group_query);
      },
      [this, &query](const ElementId& id) {
        return IsElementInRegion(id, query);
      });
}

const SceneGraph::GroupElementIdIndexMap& SceneGraph::GetElementIndex() const {
  return per_group_id_index_;
}

SceneGraph::IdToZIndexPerGroup SceneGraph::CopyZIndex() const {
  IdToZIndexPerGroup ret;
  for (auto group : per_group_id_index_) {
    ret[group.first] = group.second->IdToZIndexMap();
  }
  return ret;
}

Status SceneGraph::GetElementAbove(ElementId id, ElementId* id_above) const {
  if (!IsKnownId(id, false))
    return ErrorStatus("Element $0 does not exist.", id);

  GroupId parent_group = GetParentGroupId(id);
  auto index_it = per_group_id_index_.find(parent_group);
  ASSERT(index_it != per_group_id_index_.end() &&
         index_it->second->Contains(id));
  *id_above = index_it->second->GetIdAbove(id).value_or(kInvalidElementId);
  return OkStatus();
}

bool SceneGraph::IsClippableGroup(const GroupId& group_id) const {
  return clippable_groups_.find(group_id) != clippable_groups_.end();
}

}  // namespace ink
