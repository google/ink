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

#include "ink/engine/realtime/query_tool.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

static const glm::vec4 kGray(0.259f, 0.259f, 0.259f, 0.2f);

QueryTool::QueryTool(const service::UncheckedRegistry& registry)
    : Tool(),
      selector_(registry, kGray, false),
      engine_listener_(registry.GetShared<IEngineListener>()),
      scene_graph_(registry.GetShared<SceneGraph>()),
      text_texture_provider_(registry.GetShared<TextTextureProvider>()) {
  RegisterForInput(registry.GetShared<input::InputDispatch>());
}

QueryTool::~QueryTool() { SLOG(SLOG_OBJ_LIFETIME, "QueryTool dtor"); }

input::CaptureResult QueryTool::OnInput(const input::InputData& data,
                                        const Camera& camera) {
  if (data.Get(input::Flag::TDown) &&
      (data.Get(input::Flag::Primary) || data.n_down == 1)) {
    down_pos_world_ = data.world_pos;
  }

  input::CaptureResult result = selector_.OnInput(data, camera);
  if (data.Get(input::Flag::TUp)) {
    OnHitComplete(selector_.SelectedElements(), down_pos_world_, data.world_pos,
                  camera);
  }

  return result;
}

void QueryTool::Draw(const Camera& cam, FrameTimeS draw_time) const {
  selector_.Draw(cam, draw_time);
}

void QueryTool::Enable(bool enabled) {
  Tool::Enable(enabled);
  selector_.Reset();
}

void QueryTool::OnHitComplete(std::vector<ElementId> elements,
                              glm::vec2 down_world_coords,
                              glm::vec2 up_world_coords, const Camera& cam) {
  ink::proto::ToolEvent event;
  ink::proto::ElementQueryData* data = event.mutable_element_query_data();
  for (auto id : elements) {
    auto item = data->add_item();

    auto element_metadata = scene_graph_->GetElementMetadata(id);
    item->set_uuid(element_metadata.uuid);

    auto element_bounds = scene_graph_->Mbr(std::vector<ElementId>(1, id));
    ink::util::WriteToProto(item->mutable_world_bounds(), element_bounds);

    OptimizedMesh* mesh;
    if (scene_graph_->GetMesh(id, &mesh)) {
      if (mesh->texture) {
        item->set_uri(mesh->texture->uri);
        text::TextSpec text;
        if (text_texture_provider_->GetTextSpec(mesh->texture->uri, &text)) {
          util::WriteToProto(item->mutable_text(), text);
        }
      }
    }
  }

  auto up_point = data->mutable_up_world_location();
  up_point->set_x(up_world_coords.x);
  up_point->set_y(up_world_coords.y);

  auto down_point = data->mutable_down_world_location();
  down_point->set_x(down_world_coords.x);
  down_point->set_y(down_world_coords.y);

  engine_listener_->ToolEvent(event);
}
}  // namespace ink
