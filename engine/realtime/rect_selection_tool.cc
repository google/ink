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

#include "ink/engine/realtime/rect_selection_tool.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/realtime/edit_tool.h"
#include "ink/engine/realtime/selectors/rect_selector.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

RectSelectionTool::RectSelectionTool(const service::UncheckedRegistry& registry,
                                     EditTool* edit_tool)
    : Tool(),
      selector_(registry, kGoogleBlue500, true),
      edit_tool_(edit_tool),
      scene_graph_(registry.GetShared<SceneGraph>()) {
  RegisterForInput(registry.GetShared<input::InputDispatch>());
}

RectSelectionTool::~RectSelectionTool() {
  SLOG(SLOG_OBJ_LIFETIME, "RectSelectionTool dtor");
}

input::CaptureResult RectSelectionTool::OnInput(const input::InputData& data,
                                                const Camera& camera) {
  input::CaptureResult result = selector_.OnInput(data, camera);
  if (selector_.HasSelectedElements()) {
    OnHitComplete(selector_.SelectedElements(), camera);
  }

  return result;
}

void RectSelectionTool::Draw(const Camera& cam, FrameTimeS draw_time) const {
  selector_.Draw(cam, draw_time);
}

void RectSelectionTool::Enable(bool enabled) {
  Tool::Enable(enabled);
  selector_.Reset();
}
void RectSelectionTool::OnHitComplete(std::vector<ElementId> elements,
                                      const Camera& cam) {
  if (elements.empty()) {
    return;
  }
  auto pred = [this](const ElementId& id) {
    return !scene_graph_->GetElementMetadata(id).attributes.selectable;
  };
  elements.erase(std::remove_if(elements.begin(), elements.end(), pred),
                 elements.end());
  SLOG(SLOG_TOOLS, "selected elements: $0", elements);
  edit_tool_->ManipulateElements(cam, elements);
}

}  // namespace tools
}  // namespace ink
