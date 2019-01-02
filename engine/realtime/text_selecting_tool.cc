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

#include "ink/engine/realtime/text_selecting_tool.h"
#include "ink/engine/public/types/color.h"

namespace ink {

TextSelectingTool::TextSelectingTool(
    const service::UncheckedRegistry& registry,
    InputRegistrationPolicy input_registration_policy)
    : Tool(),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      page_manager_(registry.GetShared<PageManager>()),
      rect_renderer_(gl_resources_) {
  if (input_registration_policy == InputRegistrationPolicy::ACTIVE) {
    RegisterForInput(registry.GetShared<input::InputDispatch>());
  }
}

void TextSelectingTool::Draw(const Camera& cam, FrameTimeS draw_time) const {
  if (input_started_ && !rects_.empty() && page_manager_->MultiPageEnabled()) {
    rect_renderer_.DrawRectangles(rects_, rgba_, cam, draw_time);
    return;
  }
}

void TextSelectingTool::Update(const Camera& cam, FrameTimeS draw_time) {
  if (selection_provider_ && input_started_) {
    UpdateSelection();
  }
}

void TextSelectingTool::UpdateSelection() {
  if (page_manager_->GetPageGroupForRect(Rect(curr_world_, curr_world_)) !=
          kInvalidElementId &&
      page_manager_->GetPageGroupForRect(Rect(start_world_, start_world_)) !=
          kInvalidElementId) {
    rects_.clear();
    end_world_ = curr_world_;
    if (start_world_ != end_world_) {
      selection_provider_
          ->GetSelection(start_world_, end_world_, *page_manager_, &rects_)
          .IgnoreError();
    }
  }
}

input::CaptureResult TextSelectingTool::OnInput(const input::InputData& data,
                                                const Camera& camera) {
  if (data.Get(input::Flag::Cancel)) {
    Clear();
    return input::CapResRefuse;
  }

  if (data.Get(input::Flag::TDown) && data.Get(input::Flag::Primary)) {
    if (page_manager_->MultiPageEnabled() &&
        page_manager_->GetPageGroupForRect(
            Rect(data.world_pos, data.world_pos)) != kInvalidElementId) {
      input_started_ = true;
      start_world_ = data.world_pos;
      curr_world_ = data.world_pos;
    }
  } else if (data.Get(input::Flag::InContact)) {
    curr_world_ = data.last_world_pos;
  } else if (data.Get(input::Flag::TUp)) {
    if (input_started_ && start_world_ != end_world_) {
      UpdateSelection();  // Ensure rects_ is populated with complete gesture.
      HandleGestureEnd(rects_);
    }
    Clear();
  }

  return input::CapResObserve;
}

void TextSelectingTool::SetSelectionProvider(
    std::shared_ptr<ISelectionProvider> selection_provider) {
  selection_provider_ = std::move(selection_provider);
}

void TextSelectingTool::SetColor(glm::vec4 rgba) {
  rgba_ =
      Color::FromNonPremultipliedRGBA(rgba).WithAlpha(0.2).AsPremultipliedVec();
}

void TextSelectingTool::Clear() {
  rects_.clear();
  start_world_ = {0, 0};
  end_world_ = {0, 0};
  curr_world_ = {0, 0};
  input_started_ = false;
}
}  // namespace ink
