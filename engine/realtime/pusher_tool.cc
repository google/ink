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

#include "ink/engine/realtime/pusher_tool.h"

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/gl.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/realtime/element_manipulation_tool.h"
#include "ink/engine/realtime/element_manipulation_tool_renderer.h"
#include "ink/engine/realtime/selectors/pusher_selector.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
namespace tools {

PusherTool::PusherTool(const service::UncheckedRegistry& registry)
    : Tool(input::Priority::ManipulateSelection),
      scene_graph_(registry.GetShared<SceneGraph>()),
      engine_listener_(registry.GetShared<IEngineListener>()),
      selector_(scene_graph_),
      manipulation_tool_(
          registry, false, [this]() { Reset(); },
          std::unique_ptr<ElementManipulationToolRendererInterface>(
              new SingleElementManipulationToolRenderer(registry))) {
  // Default filter is stickers only.
  selector_.SetFilter([this](const ElementId& id) -> bool {
    return scene_graph_->GetElementMetadata(id).attributes.is_sticker;
  });
  manipulation_tool_.SetDeselectWhenOutside(false);
  RegisterForInput(registry.GetShared<input::InputDispatch>());
  registry.Get<FrameState>()->AddListener(this);
}

void PusherTool::AfterSceneDrawn(const Camera& cam,
                                 FrameTimeS draw_time) const {
  manipulation_tool_.Draw(cam, draw_time);
}

void PusherTool::Update(const Camera& cam, FrameTimeS draw_time) {
  manipulation_tool_.Update(cam, draw_time);
}

void PusherTool::Enable(bool enabled) {
  Reset();
  Tool::Enable(enabled);
}

input::CaptureResult PusherTool::OnInput(const input::InputData& data,
                                         const Camera& camera) {
  const auto tap_data = tap_reco_.OnInput(data, camera);

  if (tap_data.IsTap() && tap_data.down_data.Get(input::Flag::Primary)) {
    TapCallback(camera, tap_data.down_data.world_pos,
                tap_data.up_data.world_pos);
    Reset();
    return input::CapResRefuse;
  }

  input::CaptureResult result = input::CapResObserve;
  if (!manipulation_tool_.Enabled()) {
    result = selector_.OnInput(data, camera);
    if (selector_.HasSelectedElements()) {
      manipulation_tool_.Enable(true);
      manipulation_tool_.SetElements(camera, selector_.SelectedElements());
    }
  }

  if (manipulation_tool_.Enabled()) {
    if (data.Get(input::Flag::InContact)) {
      last_position_[data.id] = data.screen_pos;
    } else {
      last_position_.erase(data.id);
    }
    result = manipulation_tool_.OnInput(data, camera);
    if (!manipulation_tool_.IsManipulating()) Reset();
  }

  return result;
}

void PusherTool::TapCallback(const Camera& cam, glm::vec2 down_world,
                             glm::vec2 up_world) {
  ASSERT(selector_.SelectedElements().size() <= 1);

  proto::ToolEvent tool_event;
  proto::ElementQueryData* data = tool_event.mutable_element_query_data();
  data->mutable_down_world_location()->set_x(down_world.x);
  data->mutable_down_world_location()->set_y(down_world.y);
  data->mutable_up_world_location()->set_x(up_world.x);
  data->mutable_up_world_location()->set_y(up_world.y);

  if (!selector_.SelectedElements().empty()) {
    ElementId elId = selector_.SelectedElements().front();
    UUID uuid = scene_graph_->UUIDFromElementId(elId);

    proto::ElementQueryItem* item = data->add_item();
    item->set_uuid(uuid);
    OptimizedMesh* mesh;
    if (scene_graph_->GetMesh(selector_.SelectedElements().front(), &mesh)) {
      if (mesh->texture) {
        item->set_uri(mesh->texture->uri);
      }
    }
    Rect r = scene_graph_->Mbr(
        std::vector<ElementId>(1, selector_.SelectedElements().front()));
    ink::util::WriteToProto(item->mutable_world_bounds(), r);
  }

  engine_listener_->ToolEvent(tool_event);
}

void PusherTool::SetPusherToolParams(
    const proto::PusherToolParams& unsafe_params_proto) {
  selector_.SetFilter([this, unsafe_params_proto](const ElementId& id) -> bool {
    return (unsafe_params_proto.manipulate_stickers() &&
            scene_graph_->GetElementMetadata(id).attributes.is_sticker) ||
           (unsafe_params_proto.manipulate_text() &&
            scene_graph_->GetElementMetadata(id).attributes.is_text);
  });
}

void PusherTool::OnFrameEnd() { SendPositionUpdate(); }

void PusherTool::Reset() {
  manipulation_tool_.Enable(false);
  last_position_.clear();
  SendPositionUpdate();
  selector_.Reset();
  tap_reco_.Reset();
}

void PusherTool::SendPositionUpdate() const {
  if (!Enabled() || !selector_.HasSelectedElements()) {
    return;
  }

  ASSERT(selector_.SelectedElements().size() == 1);
  ElementId elId = selector_.SelectedElements().front();
  UUID uuid = scene_graph_->UUIDFromElementId(elId);

  proto::ToolEvent event;
  proto::PusherPositionUpdate* update = event.mutable_pusher_position_update();
  update->set_uuid(uuid);
  for (const auto& position : last_position_) {
    proto::Point* point = update->add_pointer_location();
    point->set_x(position.second.x);
    point->set_y(position.second.y);
  }

  engine_listener_->ToolEvent(event);
}

}  // namespace tools
}  // namespace ink
