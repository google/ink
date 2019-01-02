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

#ifndef INK_ENGINE_REALTIME_RECT_SELECTION_TOOL_H_
#define INK_ENGINE_REALTIME_RECT_SELECTION_TOOL_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/tap_reco.h"
#include "ink/engine/realtime/selectors/rect_selector.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

class EditTool;

class RectSelectionTool : public Tool {
 public:
  RectSelectionTool(const service::UncheckedRegistry& registry,
                    EditTool* edit_tool);
  ~RectSelectionTool() override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  void Enable(bool enabled) override;

  inline std::string ToString() const override { return "<RectSelectionTool>"; }

 private:
  void OnHitComplete(std::vector<ElementId> elements, const Camera& cam);

  RectSelector selector_;
  EditTool* edit_tool_;
  std::shared_ptr<SceneGraph> scene_graph_;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_RECT_SELECTION_TOOL_H_
