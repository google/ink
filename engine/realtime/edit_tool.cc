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

#include "ink/engine/realtime/edit_tool.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/realtime/element_manipulation_tool.h"
#include "ink/engine/realtime/element_manipulation_tool_renderer.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

EditTool::EditTool(const service::UncheckedRegistry& registry)
    : enabled_(false),
      selection_empty_(true),
      rect_selection_tool_(registry, this),
      manipulation_tool_(
          registry, true, [this]() -> void { this->CancelManipulation(); },
          std::unique_ptr<ElementManipulationToolRendererInterface>(
              new ElementManipulationToolRenderer(registry))),
      frame_state_(registry.GetShared<FrameState>()),
      public_events_(registry.GetShared<PublicEvents>()) {}

EditTool::~EditTool() { SLOG(SLOG_OBJ_LIFETIME, "EditTool dtor"); }

void EditTool::Draw(const Camera& cam, FrameTimeS draw_time) const {
  rect_selection_tool_.Draw(cam, draw_time);
  manipulation_tool_.Draw(cam, draw_time);
}

void EditTool::Enable(bool enabled) {
  enabled_ = enabled;
  CancelManipulation();
  if (!enabled_) {
    manipulation_tool_.Enable(false);
    rect_selection_tool_.Enable(false);
  }
}

void EditTool::CancelManipulation() {
  if (!selection_empty_) {
    SelectionStateChanged(false);
  }
  if (enabled_) {
    manipulation_tool_.Enable(false);
    rect_selection_tool_.Enable(true);

    selection_empty_ = true;
  }
}

const ElementManipulationTool& EditTool::Manipulation() const {
  return manipulation_tool_;
}

void EditTool::ManipulateElements(const Camera& cam,
                                  const std::vector<ElementId>& elements) {
  rect_selection_tool_.Enable(false);
  manipulation_tool_.Enable(true);
  manipulation_tool_.SetElements(cam, elements);

  if (selection_empty_ != elements.empty()) {
    selection_empty_ = elements.empty();
    SelectionStateChanged(!selection_empty_);
  }

  frame_state_->RequestFrame();
}

void EditTool::Update(const Camera& cam, FrameTimeS draw_time) {
  rect_selection_tool_.Update(cam, draw_time);
  manipulation_tool_.Update(cam, draw_time);
}

bool EditTool::IsManipulating() const { return manipulation_tool_.Enabled(); }

void EditTool::SelectionStateChanged(bool selected) {
  proto::ToolEvent event;
  event.mutable_selection_state()->set_anything_selected(selected);
  public_events_->ToolEvent(event);
}

}  // namespace tools
}  // namespace ink
