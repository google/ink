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

#ifndef INK_ENGINE_REALTIME_EDIT_TOOL_H_
#define INK_ENGINE_REALTIME_EDIT_TOOL_H_

#include <memory>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/realtime/element_manipulation_tool.h"
#include "ink/engine/realtime/rect_selection_tool.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

// The EditTool allows the user the select and manipulate elements in the scene.
// It is actually made up of two component tools, the RectSelectionTool and the
// ElementManipulationTool, which are responsible for the sub-tasks of selection
// and manipulation, respectively.
//
// The EditTool's primary responsibility is to handle the interaction between
// the RectSelectionTool and the ElementManipulationTool, passing information
// between them and disabling one when the other is active.
//
// Note that OnInput() does nothing -- this is because RectSelectionTool and
// ElementManipulationTool are each registered separately with InputDispatch.
// This is done in order to give them different input priorities; selection must
// be lower priority than the pan handler, but manipulation must be higher.
class EditTool : public Tool {
 public:
  explicit EditTool(const service::UncheckedRegistry& registry);
  ~EditTool() override;

  void ManipulateElements(const Camera& cam,
                          const std::vector<ElementId>& elements);
  void CancelManipulation();
  bool IsManipulating() const;
  const ElementManipulationTool& Manipulation() const;

  // ITool
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void Update(const Camera& cam, FrameTimeS draw_time) override;
  void Enable(bool enabled) override;
  bool Enabled() const override { return enabled_; }
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override {
    return input::CapResRefuse;
  }

  inline std::string ToString() const override { return "<EditTool>"; }

 private:
  bool enabled_;
  bool selection_empty_;
  RectSelectionTool rect_selection_tool_;
  ElementManipulationTool manipulation_tool_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<PublicEvents> public_events_;

  void SelectionStateChanged(bool selected);
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_EDIT_TOOL_H_
