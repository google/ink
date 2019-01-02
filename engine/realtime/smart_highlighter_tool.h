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

#ifndef INK_ENGINE_REALTIME_SMART_HIGHLIGHTER_TOOL_H_
#define INK_ENGINE_REALTIME_SMART_HIGHLIGHTER_TOOL_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/realtime/line_tool.h"
#include "ink/engine/realtime/text_highlighter_tool.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// The "smart" highlighter is a tool that, if you begin your drawing gesture
// near or in text, highlights lines of text, but, if you begin your drawing
// gesture away from text, acts like a normal highlighter pen.
class SmartHighlighterTool : public Tool {
 public:
  explicit SmartHighlighterTool(const service::UncheckedRegistry& registry);

  // SmartHighlighterTool is neither copyable nor movable.
  SmartHighlighterTool(const SmartHighlighterTool&) = delete;
  SmartHighlighterTool& operator=(const SmartHighlighterTool&) = delete;

  void Enable(bool enabled) override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void Update(const Camera& cam, FrameTimeS draw_time) override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  void SetColor(glm::vec4 rgba) override;

  void SetBrushParams(BrushParams params);

  void SetSelectionProvider(
      std::shared_ptr<ISelectionProvider> selection_provider);

  inline std::string ToString() const override {
    return "<SmartHighlighterTool>";
  }

 private:
  enum class State { kIdle, kDrawingLine, kSelectingText };
  State state_{State::kIdle};

  LineTool line_tool_;
  TextHighlighterTool text_tool_;
  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<ISelectionProvider> selection_provider_;

  std::shared_ptr<IDbgHelper> dbg_helper_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_SMART_HIGHLIGHTER_TOOL_H_
