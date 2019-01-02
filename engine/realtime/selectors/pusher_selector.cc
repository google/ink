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

#include "ink/engine/realtime/selectors/pusher_selector.h"

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/realtime/selectors/selector.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/graph/scene_graph.h"

namespace ink {

std::vector<ElementId> PusherSelector::SelectedElements() const {
  if (HasSelectedElements()) return {selected_id_};

  return {};
}

input::CaptureResult PusherSelector::OnInput(const input::InputData& data,
                                             const Camera& camera) {
  if (data.Get(input::Flag::Cancel)) {
    Reset();
    return input::CapResRefuse;
  } else if (!data.Get(input::Flag::TDown) || data.n_down > 2) {
    return input::CapResObserve;
  }

  RegionQuery query;
  if (data.n_down == 1) {
    float world_selection_size =
        camera.ConvertDistance(RegionQuery::MinSelectionSizeCm(data.type),
                               DistanceType::kCm, DistanceType::kWorld);
    query = RegionQuery::MakePointQuery(data.world_pos, world_selection_size);
  } else {
    float world_selection_size = camera.ConvertDistance(
        RegionQuery::MinSegmentSelectionSizeCm(data.type), DistanceType::kCm,
        DistanceType::kWorld);
    query = RegionQuery::MakeSegmentQuery(
        Segment(first_down_pos_, data.world_pos), world_selection_size);
  }
  query.SetCustomFilter(Filter());
  if (scene_graph_->TopElementInRegion(query, &selected_id_)) {
    has_selected_ = true;
  } else {
    Reset();
    if (data.n_down == 1) first_down_pos_ = data.world_pos;
  }

  return has_selected_ ? input::CapResCapture : input::CapResObserve;
}

void PusherSelector::Reset() {
  has_selected_ = false;
  selected_id_ = kInvalidElementId;
  first_down_pos_ = glm::vec2(0, 0);
}

}  // namespace ink
