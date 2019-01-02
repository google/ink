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

#include "ink/engine/realtime/smart_highlighter_tool.h"

namespace ink {

SmartHighlighterTool::SmartHighlighterTool(
    const service::UncheckedRegistry& registry)
    : Tool(),
      line_tool_(registry, InputRegistrationPolicy::PASSIVE),
      text_tool_(registry, InputRegistrationPolicy::PASSIVE),
      page_manager_(registry.GetShared<PageManager>()),
      dbg_helper_(registry.GetShared<IDbgHelper>()) {
  RegisterForInput(registry.GetShared<input::InputDispatch>());
  line_tool_.Clear();
}

void SmartHighlighterTool::Enable(bool enabled) {
  if (!enabled) line_tool_.Clear();
  Tool::Enable(enabled);
}

void SmartHighlighterTool::SetColor(glm::vec4 rgba) {
  text_tool_.SetColor(rgba);
  line_tool_.SetColor(rgba);
}

void SmartHighlighterTool::Update(const Camera& cam, FrameTimeS draw_time) {
  if (state_ == State::kDrawingLine) {
    line_tool_.Update(cam, draw_time);
  } else if (state_ == State::kSelectingText) {
    text_tool_.Update(cam, draw_time);
  }
}

void SmartHighlighterTool::Draw(const Camera& cam, FrameTimeS draw_time) const {
  if (state_ == State::kDrawingLine) {
    line_tool_.Draw(cam, draw_time);
  } else if (state_ == State::kSelectingText) {
    text_tool_.Draw(cam, draw_time);
  }
}

input::CaptureResult SmartHighlighterTool::OnInput(const input::InputData& data,
                                                   const Camera& camera) {
#ifndef NDEBUG
  // When debugging, indicate selection candidate rectangles with green boxes.
  if (selection_provider_) {
    static constexpr uint32_t s_debug_rect_id = 2929;
    dbg_helper_->Remove(s_debug_rect_id);
    for (const Rect& r : selection_provider_->GetCandidateRects(
             data.world_pos, *page_manager_)) {
      dbg_helper_->AddRect(r, glm::vec4(0, 1, 0, 1), false, s_debug_rect_id);
    }
  }
#endif

  input::CaptureResult res = input::CapResObserve;
  if (data.Get(input::Flag::Primary) && data.Get(input::Flag::TDown)) {
    if (selection_provider_ &&
        selection_provider_->IsInText(data.world_pos, *page_manager_)) {
      state_ = State::kSelectingText;
    } else {
      state_ = State::kDrawingLine;
    }
  }

  if (state_ == State::kDrawingLine) {
    res = line_tool_.OnInput(data, camera);
  } else if (state_ == State::kSelectingText) {
    res = text_tool_.OnInput(data, camera);
  }

  if (res == input::CapResRefuse) {
    state_ = State::kIdle;
  }
  return res;
}

void SmartHighlighterTool::SetBrushParams(BrushParams params) {
  // force highlighter
  params.line_modifier = BrushParams::LineModifier::HIGHLIGHTER;
  line_tool_.SetBrushParams(params);
}

void SmartHighlighterTool::SetSelectionProvider(
    std::shared_ptr<ISelectionProvider> selection_provider) {
  text_tool_.SetSelectionProvider(selection_provider);
  selection_provider_ = selection_provider;
}
}  // namespace ink
