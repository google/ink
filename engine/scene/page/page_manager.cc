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

#include "ink/engine/scene/page/page_manager.h"

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_layout_strategy.h"
#include "ink/engine/scene/page/page_properties_notifier.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {
namespace {
class NoOpLayout : public PageLayoutStrategy {
 public:
  // A no-op layout will overlay all the pages on top of each other.
  PageInfoList GenerateLayout(
      const Camera& cam,
      const std::vector<PageSpec>& page_defs) const override {
    PageInfoList pages;
    for (auto& page : page_defs) {
      PageInfo page_info;
      page_info.bounds = Rect(glm::vec2(0, 0), page.dimensions);
      page_info.transform = glm::mat4(1);
      pages.emplace_back(page_info);
    }
    return pages;
  }
};

}  // namespace

PageManager::PageManager(std::shared_ptr<SceneGraph> scene_graph,
                         std::shared_ptr<Camera> camera)
    : scene_graph_(scene_graph), camera_(camera) {
  Clear();
  SetLayoutStrategy(absl::make_unique<NoOpLayout>());
}

bool PageManager::MultiPageEnabled() const { return !page_specs_.empty(); }

bool PageManager::IsDirty() const { return dirty_; }

int PageManager::GetNumPages() const { return page_specs_.size(); }

bool PageManager::GroupExists(const GroupId& group_id) const {
  return group_to_page_index_.find(group_id) != group_to_page_index_.end();
}

void PageManager::SetLayoutStrategy(
    std::unique_ptr<PageLayoutStrategy> strategy) {
  strategy_ = std::move(strategy);
  dirty_ = true;
}

PageLayoutStrategy* PageManager::GetLayoutStrategy() const {
  return strategy_.get();
}

Status PageManager::AddNewPageWithDimensions(glm::vec2 dimensions) {
  PageSpec page_spec;
  page_spec.uuid = scene_graph_->GenerateUUID();
  if (!scene_graph_->GetNextGroupId(page_spec.uuid, &page_spec.group_id)) {
    return ErrorStatus("could not generate group id");
  }
  page_spec.dimensions = dimensions;
  group_to_page_index_[page_spec.group_id] = page_specs_.size();
  page_specs_.emplace_back(page_spec);
  dirty_ = true;
  return OkStatus();
}

Status PageManager::AddPageFromProto(
    const ink::proto::PerPageProperties& unsafe_per_page_properties) {
  PageSpec page_spec;
  page_spec.uuid = unsafe_per_page_properties.uuid();
  page_spec.group_id = scene_graph_->ElementIdFromUUID(page_spec.uuid);
  if (page_spec.group_id == kInvalidElementId) {
    return ErrorStatus("could not find group $0", page_spec.uuid);
  }
  page_spec.dimensions = glm::vec2(unsafe_per_page_properties.width(),
                                   unsafe_per_page_properties.height());
  group_to_page_index_[page_spec.group_id] = page_specs_.size();
  page_specs_.emplace_back(page_spec);
  dirty_ = true;
  return OkStatus();
}

void PageManager::GenerateLayout() {
  if (!dirty_) {
    return;
  }
  dirty_ = false;
  ASSERT(strategy_ != nullptr);
  // Reset the bounds.
  full_bounds_ = Rect();
  page_info_ = strategy_->GenerateLayout(*camera_, page_specs_);
  ASSERT(page_info_.size() == page_specs_.size());
  // Push the transform into the scene graph and update our full
  // scene size.
  for (int i = 0; i < page_info_.size(); ++i) {
    auto& info = page_info_[i];
    page_info_[i].page_spec = page_specs_[i];
    page_info_[i].page_index = i;
    ASSERT(page_info_[i].IsConsistent());
    if (i == 0) {
      full_bounds_ = info.bounds;
    } else {
      full_bounds_ = full_bounds_.Join(info.bounds);
    }
    scene_graph_->AddOrUpdateGroup(info.page_spec.group_id, info.transform,
                                   Rect({0, 0}, info.page_spec.dimensions),
                                   /* clippable= */ true, kUnknownGroupType,
                                   SourceDetails::FromEngine());
  }
}

void PageManager::Clear() {
  page_specs_.clear();
  group_to_page_index_.clear();
  page_info_.clear();
  full_bounds_ = Rect();
  dirty_ = false;
}

GroupId PageManager::GetPageGroupId(int page) const {
  ASSERT(page < page_specs_.size());
  return page_specs_[page].group_id;
}

const PageSpec& PageManager::GetPageSpec(int page) const {
  ASSERT(page < page_specs_.size());
  return page_specs_[page];
}

const PageSpec& PageManager::GetPageSpec(GroupId id) const {
  ASSERT(group_to_page_index_.count(id) > 0);
  return GetPageSpec(group_to_page_index_.find(id)->second);
}

const PageInfo& PageManager::GetPageInfo(int page) const {
  ASSERT(!dirty_);
  ASSERT(page < page_info_.size());
  return page_info_[page];
}

const PageInfo& PageManager::GetPageInfo(GroupId id) const {
  ASSERT(group_to_page_index_.count(id) > 0);
  return GetPageInfo(group_to_page_index_.find(id)->second);
}

GroupId PageManager::GetPageGroupForRect(Rect region) const {
  ASSERT(!dirty_);
  for (auto& page : page_info_) {
    Rect intersection;
    if ((geometry::Intersection(page.bounds, region, &intersection) &&
         intersection.Area() > 0) ||
        page.bounds.Contains(region)) {
      return page.page_spec.group_id;
    }
  }
  return kInvalidElementId;
}

Rect PageManager::GetFullBounds() const {
  ASSERT(!dirty_);
  return full_bounds_;
}

}  // namespace ink
