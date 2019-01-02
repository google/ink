/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_REALTIME_TOOL_CONTROLLER_H_
#define INK_ENGINE_REALTIME_TOOL_CONTROLLER_H_

#include <memory>

#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/realtime/pan_handler.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/security.h"

namespace ink {

// Keeps track of the available and currently-selected tools.
//
// Initially, ToolController will only contain a NullTool. addTool() must be
// used to add them. Attempting to interact with a tool that hasn't been added
// will cause a failed assertion.
class ToolController : public settings::FlagListener {
 public:
  using SharedDeps = service::Dependencies<PanHandler, settings::Flags>;

  ToolController(std::shared_ptr<PanHandler> pan_handler,
                 std::shared_ptr<settings::Flags> flags);
  // Only one tool can be added for each tool type.
  void AddTool(Tools::ToolType, std::unique_ptr<Tool> tool);

  // Setting the ToolType changes the ChosenTool to that type.
  //
  // If we are not in ReadOnly mode the ChosenTool and EnabledTool are
  // synonymous: setting a Tool will enable that tool and disable the existing
  // tool.
  //
  // When in ReadOnly mode, the EnabledTool is always NoTool, and setting the
  // ChosenTool does not cause any change in what is Enabled.
  void SetToolType(Tools::ToolType type);
  Tool* ChosenTool() const;
  Tools::ToolType ChosenToolType() const;
  Tool* EnabledTool() const;
  Tools::ToolType EnabledToolType() const;

  void Update(const Camera& cam, FrameTimeS draw_time);

  void OnFlagChanged(settings::Flag which, bool new_value) override;

  bool IsEditToolManipulating() const;

  template <typename T>
  S_WARN_UNUSED_RESULT bool GetTool(Tools::ToolType type, T** tool) const {
    *tool = nullptr;
    auto result = tools_.find(type);
    if (result != tools_.end()) {
      *tool = dynamic_cast<T*>(result->second.get());
      if (tool == nullptr) {
        ASSERT(false);
        return false;
      }
      return true;
    }
    return false;
  }

 private:
  bool HasTool(Tools::ToolType type) const;

  // The tool that has been set. This may differ from the active tool when
  // ReadOnly mode is enabled.
  Tools::ToolType chosen_tool_type_;
  std::unordered_map<Tools::ToolType, std::unique_ptr<Tool>> tools_;
  std::shared_ptr<PanHandler> pan_handler_;
  std::shared_ptr<settings::Flags> flags_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_TOOL_CONTROLLER_H_
