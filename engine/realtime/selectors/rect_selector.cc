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

#include "ink/engine/realtime/selectors/rect_selector.h"

#include <memory>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/tap_reco.h"
#include "ink/engine/realtime/selectors/selector.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

RectSelector::RectSelector(const service::UncheckedRegistry& registry,
                           const glm::vec4 color, bool tap_for_single_selection)
    : scene_graph_(registry.GetShared<SceneGraph>()),
      layer_manager_(registry.GetShared<LayerManager>()),
      shape_(ShapeGeometry::Type::Rectangle),
      shape_renderer_(registry),
      is_selecting_(false),
      tap_for_single_selection_(tap_for_single_selection) {
  // Just make the fill a translucent version of the color for now.
  shape_.SetFillColor(glm::vec4(color.r, color.g, color.b, 0.3f * color.a));
  shape_.SetBorderColor(color);
}

input::CaptureResult RectSelector::OnInput(const input::InputData& data,
                                           const Camera& camera) {
  if (data.Get(input::Flag::Cancel)) {
    Reset();
    return input::CapResRefuse;
  }

  if (data.Get(input::Flag::InContact) &&
      (data.Get(input::Flag::Primary) || data.n_down == 1)) {
    if (!is_selecting_) {
      down_pos_world_ = data.world_pos;
      selected_.clear();
      shape_.SetVisible(true);
      is_selecting_ = true;
    }

    glm::vec2 border_size{
        camera.ConvertDistance(1, DistanceType::kDp, DistanceType::kWorld)};
    Rect border_rect(down_pos_world_, data.world_pos);
    shape_.SetSizeAndPosition(border_rect, border_size, true);
  }

  const auto tap_data = tap_reco_.OnInput(data, camera);
  bool select_only_one = tap_for_single_selection_ && tap_data.IsTap();

  if ((is_selecting_ && data.n_down == 0) || tap_data.IsTap()) {
    Rect region(down_pos_world_, data.world_pos);
    Select(data.type, region, camera, select_only_one);
  }

  return input::CapResCapture;
}

void RectSelector::Select(input::InputType input_type, Rect region,
                          const Camera& cam, bool only_one_element) {
  Reset();

  auto group_id_or = layer_manager_->GroupIdOfActiveLayer();
  GroupId group_id =
      group_id_or.ok() ? group_id_or.ValueOrDie() : kInvalidElementId;

  // Ensure a minimum search area.
  float world_selection_size =
      cam.ConvertDistance(RegionQuery::MinSelectionSizeCm(input_type),
                          DistanceType::kCm, DistanceType::kWorld);
  RegionQuery query =
      RegionQuery::MakeRectangleQuery(region, world_selection_size)
          .SetCustomFilter(Filter());
  query.SetGroupFilter(group_id);

  if (only_one_element) {
    ElementId topId;
    if (scene_graph_->TopElementInRegion(query, &topId)) {
      selected_.push_back(topId);
    }
  } else {
    scene_graph_->ElementsInRegion(query, std::back_inserter(selected_));
  }
}

void RectSelector::Reset() {
  selected_.clear();
  shape_.SetVisible(false);
  is_selecting_ = false;
}

void RectSelector::Draw(const Camera& cam, FrameTimeS draw_time) const {
  shape_renderer_.Draw(cam, draw_time, shape_);
}

}  // namespace ink
