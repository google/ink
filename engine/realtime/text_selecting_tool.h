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

#ifndef INK_ENGINE_REALTIME_TEXT_SELECTING_TOOL_H_
#define INK_ENGINE_REALTIME_TEXT_SELECTING_TOOL_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/rectangles_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// TextSelectingTool takes in input data (drag gesture) and renders the
// inferred user selection in the form of bounding rectangles around lines of
// text as reported by the engine's ISelectionProvider, if any. Subclasses
// must implement HandleGestureEnd.
class TextSelectingTool : public Tool {
 public:
  TextSelectingTool(const service::UncheckedRegistry& registry,
                    InputRegistrationPolicy input_registration_policy);

  // Draws the rectangles in rects_ using the rectangles renderer
  // if input has started and the rects_ vector is not empty.
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

  // Update lets us interactively indicate the current selection.
  void Update(const Camera& cam, FrameTimeS draw_time) override;

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  void SetSelectionProvider(
      std::shared_ptr<ISelectionProvider> selection_provider);

  void SetColor(glm::vec4 rgba) override;

  inline std::string ToString() const override { return "<TextSelectingTool>"; }

 protected:
  virtual void HandleGestureEnd(const std::vector<Rect>& rects) = 0;
  std::vector<Rect> rects_;
  glm::vec4 rgba_{0, 0, 0, 0};

 private:
  // Based on the start_world_ and end_world_, calculate the current text
  // selection and populate rects_ with that selection.
  void UpdateSelection();

  // Reset all gesture-tracking and selection data.
  void Clear();

  mutable bool input_started_ = false;

  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<ISelectionProvider> selection_provider_;
  std::shared_ptr<PageManager> page_manager_;

  mutable RectanglesRenderer rect_renderer_;

  glm::vec2 end_world_{0, 0};
  glm::vec2 start_world_{0, 0};
  glm::vec2 curr_world_{0, 0};
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_TEXT_SELECTING_TOOL_H_
