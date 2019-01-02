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

#include "ink/engine/realtime/crop_tool.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

using glm::vec2;
using glm::vec4;

namespace ink {
namespace tools {

CropTool::CropTool(const service::UncheckedRegistry& registry)
    : Tool(input::Priority::Crop),
      crop_controller_(registry.GetShared<CropController>()) {
  RegisterForInput(registry.GetShared<input::InputDispatch>());
}

CropTool::~CropTool() { SLOG(SLOG_OBJ_LIFETIME, "CropTool dtor"); }

input::CaptureResult CropTool::OnInput(const input::InputData& data,
                                       const Camera& camera) {
  return crop_controller_->OnInput(data, camera);
}

void CropTool::AfterSceneDrawn(const Camera& cam, FrameTimeS draw_time) const {
  crop_controller_->Draw(cam, draw_time);
}

void CropTool::Enable(bool enabled) {
  Tool::Enable(enabled);
  crop_controller_->SetInteriorInputPolicy(
      CropController::InteriorInputPolicy::kMove);
  crop_controller_->Enable(enabled);
}

}  // namespace tools
}  // namespace ink
