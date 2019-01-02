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

#include "ink/engine/realtime/tool_controller.h"

#include <cstdint>
#include <memory>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/realtime/crop_tool.h"
#include "ink/engine/realtime/edit_tool.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

using glm::vec4;
using Tools::ToolType;

ToolController::ToolController(std::shared_ptr<PanHandler> pan_handler,
                               std::shared_ptr<settings::Flags> flags)
    : chosen_tool_type_(Tools::NoTool),
      pan_handler_(std::move(pan_handler)),
      flags_(flags) {
  AddTool(ToolType::NoTool, std::unique_ptr<Tool>(new NullTool()));

  SetToolType(Tools::NoTool);

  flags_->AddListener(this);
}

void ToolController::AddTool(Tools::ToolType type, std::unique_ptr<Tool> tool) {
  ASSERT(!HasTool(type));
  tool->Enable(false);
  tools_.emplace(type, std::move(tool));
}

void ToolController::SetToolType(ToolType type) {
  ASSERT(chosen_tool_type_ != Tools::MinTool);
  ASSERT(HasTool(type));

  EnabledTool()->Enable(false);
  chosen_tool_type_ = type;
  EnabledTool()->Enable(true);
  pan_handler_->SetAllowOneFingerPan(EnabledToolType() == Tools::NoTool);
}

Tool* ToolController::EnabledTool() const {
  Tools::ToolType type = EnabledToolType();
  ASSERT(HasTool(type));
  return tools_.find(type)->second.get();
}

Tools::ToolType ToolController::EnabledToolType() const {
  if (flags_->GetFlag(settings::Flag::ReadOnlyMode)) {
    return ToolType::NoTool;
  }
  return chosen_tool_type_;
}

Tool* ToolController::ChosenTool() const {
  Tools::ToolType type = ChosenToolType();
  ASSERT(HasTool(type));
  return tools_.find(type)->second.get();
}

Tools::ToolType ToolController::ChosenToolType() const {
  return chosen_tool_type_;
}

void ToolController::Update(const Camera& cam, FrameTimeS draw_time) {
  for (auto iter = tools_.begin(); iter != tools_.end(); ++iter) {
    iter->second->Update(cam, draw_time);
  }
}

void ToolController::OnFlagChanged(settings::Flag which, bool enabled) {
  if (which == settings::Flag::ReadOnlyMode) {
    if (!enabled) {
      // When exiting ReadOnly, Enable the Chosen tool.
      SetToolType(chosen_tool_type_);
    } else {
      // If we are entering ReadOnly we need to disable the chosen tool and
      // enable the new Active tool (which will already be NoTool since the flag
      // was flipped).
      ASSERT(EnabledToolType() == ToolType::NoTool);
      ChosenTool()->Enable(false);
      EnabledTool()->Enable(true);
      pan_handler_->SetAllowOneFingerPan(true);
    }
  }
}

bool ToolController::IsEditToolManipulating() const {
  if (!HasTool(Tools::Edit)) return false;
  tools::EditTool* edit =
      static_cast<tools::EditTool*>(tools_.find(Tools::Edit)->second.get());
  return edit->IsManipulating();
}

bool ToolController::HasTool(Tools::ToolType type) const {
  auto a = tools_.find(type);
  return a != tools_.end();
}

}  // namespace ink
