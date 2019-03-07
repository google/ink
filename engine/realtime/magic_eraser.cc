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

#include "ink/engine/realtime/magic_eraser.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <unordered_set>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

namespace {

// Whether we should also draw all of the queries to the DbgHelper.
constexpr bool kDebugShowQueryPositions = false;
constexpr int kDbgHelperId = 122;

}  // namespace

using glm::mat4;
using glm::vec2;
using glm::vec3;

MagicEraser::MagicEraser(const service::UncheckedRegistry& registry)
    : MagicEraser(registry.GetShared<input::InputDispatch>(),
                  registry.GetShared<SceneGraph>(),
                  registry.GetShared<IDbgHelper>(),
                  registry.GetShared<LayerManager>()) {}

MagicEraser::MagicEraser(std::shared_ptr<input::InputDispatch> dispatch,
                         std::shared_ptr<SceneGraph> scene_graph,
                         std::shared_ptr<IDbgHelper> dbg_helper,
                         std::shared_ptr<LayerManager> layer_manager)
    : MagicEraser(dispatch, scene_graph, dbg_helper, layer_manager, false) {}

MagicEraser::MagicEraser(std::shared_ptr<input::InputDispatch> dispatch,
                         std::shared_ptr<SceneGraph> scene_graph,
                         std::shared_ptr<IDbgHelper> dbg_helper,
                         std::shared_ptr<LayerManager> layer_manager,
                         bool only_handle_eraser)
    : Tool(only_handle_eraser ? input::Priority::StylusEraser
                              : input::Priority::Default),
      scene_graph_(std::move(scene_graph)),
      dbg_helper_(std::move(dbg_helper)),
      layer_manager_(std::move(layer_manager)),
      only_handle_eraser_(only_handle_eraser) {
  RegisterForInput(dispatch);
}

input::CaptureResult MagicEraser::OnInput(const input::InputData& data,
                                          const Camera& camera) {
  if (data.Get(input::Flag::Cancel) || data.Get(input::Flag::Right)) {
    Cancel();
    return only_handle_eraser_ ? input::CapResObserve : input::CapResRefuse;
  } else if (only_handle_eraser_ && !data.Get(input::Flag::Eraser)) {
    return input::CapResObserve;
  } else if (!data.Get(input::Flag::Primary)) {
    return only_handle_eraser_ ? input::CapResObserve : input::CapResRefuse;
  }

  if (data.Get(input::Flag::TDown)) {
    first_world_pos_ = data.world_pos;
  }

  const auto tap_data = tap_reco_.OnInput(data, camera);

  // Don't erase anything while the tap status is ambiguous (e.g. we might still
  // realize this was a tap). Note that the tap data will always stop being
  // ambiguous once the pointer is released, at that point we definitively know
  // it was or wasn't a tap.
  if (tap_data.IsAmbiguous()) {
    return input::CapResCapture;
  }
  bool delete_only_one = tap_data.IsTap();

  std::unordered_set<ElementId, ElementIdHasher> new_ids;

  // If there is an active layer, then only finds elements in that layer.
  auto group_id_or = layer_manager_->GroupIdOfActiveLayer();
  GroupId group_id =
      group_id_or.ok() ? group_id_or.ValueOrDie() : kInvalidElementId;

  if (delete_only_one) {
    float world_query_size =
        camera.ConvertDistance(RegionQuery::MinSelectionSizeCm(data.type),
                               DistanceType::kCm, DistanceType::kWorld);
    ElementId id;
    RegionQuery query =
        RegionQuery::MakePointQuery(data.world_pos, world_query_size);
    query.SetGroupFilter(group_id);
    if (scene_graph_->TopElementInRegion(query, &id)) {
      new_ids.insert(id);
    }
    if (kDebugShowQueryPositions) {
      dbg_helper_->AddMesh(query.MakeDebugMesh(), kDbgHelperId);
    }
  } else {
    float world_query_size = camera.ConvertDistance(
        RegionQuery::MinSegmentSelectionSizeCm(data.type), DistanceType::kCm,
        DistanceType::kWorld);

    vec2 from_pt = data.last_world_pos;
    vec2 to_pt = data.world_pos;
    // For the first step during a drag-erase, we erase from the original down
    // to the current world_pos.  Otherwise, we might miss a small region close
    // to the start before the tap_reco decides it isn't a tap.
    if (first_world_pos_) {
      from_pt = *first_world_pos_;
      first_world_pos_ = absl::nullopt;
    }

    if (from_pt != to_pt) {
      RegionQuery query = RegionQuery::MakeSegmentQuery(Segment(from_pt, to_pt),
                                                        world_query_size);
      query.SetGroupFilter(group_id);
      scene_graph_->ElementsInRegion(query,
                                     std::inserter(new_ids, new_ids.begin()));

      if (kDebugShowQueryPositions) {
        dbg_helper_->AddMesh(query.MakeDebugMesh(), kDbgHelperId);
      }
    }
  }

  for (auto it = std::begin(new_ids); it != std::end(new_ids);) {
    if (!scene_graph_->GetElementMetadata(*it).attributes.magic_erasable) {
      it = new_ids.erase(it);
    } else {
      ++it;
    }
  }

  scene_graph_->SetElementRenderedByMain(new_ids.begin(), new_ids.end(), false);
  for (auto e : new_ids) {
    intersected_elements_.insert(e);
  }

  if (data.Get(input::Flag::TUp)) {
    Commit();
  }

  return input::CapResCapture;
}

absl::optional<input::Cursor> MagicEraser::CurrentCursor(
    const Camera& camera) const {
  if (only_handle_eraser_) {
    return absl::nullopt;
  }
  return input::Cursor(input::CursorType::BRUSH, ink::Color::kWhite, 6.0);
}

void MagicEraser::Cancel() {
  scene_graph_->SetElementRenderedByMain(intersected_elements_.begin(),
                                         intersected_elements_.end(), true);
  first_world_pos_ = absl::nullopt;
  intersected_elements_.clear();
}

void MagicEraser::Commit() {
  scene_graph_->RemoveElements(intersected_elements_.begin(),
                               intersected_elements_.end(),
                               SourceDetails::FromEngine());
  first_world_pos_ = absl::nullopt;
  intersected_elements_.clear();
}

void MagicEraser::Draw(const Camera& cam, FrameTimeS draw_time) const {}

void MagicEraser::Enable(bool enabled) {
  Tool::Enable(enabled);

  Cancel();
}
}  // namespace ink
