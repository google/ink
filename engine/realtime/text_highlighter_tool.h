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

#ifndef INK_ENGINE_REALTIME_TEXT_HIGHLIGHTER_TOOL_H_
#define INK_ENGINE_REALTIME_TEXT_HIGHLIGHTER_TOOL_H_

#include "ink/engine/realtime/text_selecting_tool.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {

// TextHighlighterTool enables a user to "highlight" text based on a drag
// gesture. Once the user's gesture is finished, TextHighlighterTool will
// handle creating rectangular meshes.
class TextHighlighterTool : public TextSelectingTool {
 public:
  explicit TextHighlighterTool(
      const service::UncheckedRegistry& registry,
      InputRegistrationPolicy input_registration_policy =
          InputRegistrationPolicy::ACTIVE);

  // Called when a gesture is completed.
  void HandleGestureEnd(const std::vector<Rect>& rects) override;

  inline std::string ToString() const override {
    return "<TextHighlighterTool>";
  }

 private:
  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<SceneGraph> scene_graph_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_TEXT_HIGHLIGHTER_TOOL_H_
