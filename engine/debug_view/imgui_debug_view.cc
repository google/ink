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

#include "third_party/absl/memory/memory.h"
#include "third_party/dear_imgui/imgui.h"
#include "ink/engine/debug_view/debug_view.h"
#include "ink/public/contrib/imgui/imgui_bridge.h"

namespace ink {

// DebugView shows controls with debug information and engine manipulation.
//
// It is optionally compiled with the "--define debug_view=1" flags.
class ImGuiDebugView : public DebugView {
 public:
  using SharedDeps =
      service::Dependencies<FrameState, Camera, GLResourceManager,
                            input::InputDispatch, input::keyboard::Dispatch>;
  ImGuiDebugView(std::shared_ptr<FrameState> frame, std::shared_ptr<Camera> cam,
                 std::shared_ptr<GLResourceManager> gl,
                 std::shared_ptr<input::InputDispatch> pointer_dispatch,
                 std::shared_ptr<input::keyboard::Dispatch> keyboard_dispatch);

  // Per-frame update.
  void Update(ink::FrameTimeS t) override;

  void Draw(ink::RootRenderer::RenderOrder at_order, const Camera& draw_cam,
            FrameTimeS draw_time) const override;

  // DebugView is neither copyable nor movable.
  ImGuiDebugView(const ImGuiDebugView&) = delete;
  ImGuiDebugView& operator=(const ImGuiDebugView&) = delete;

 private:
  std::unique_ptr<imgui::ImGuiBridge> imgui_;
};

// Instantiate a real debug view.
void DefineDebugView(service::DefinitionList* definitions) {
  definitions->DefineService<DebugView, ImGuiDebugView>();
}

ImGuiDebugView::ImGuiDebugView(
    std::shared_ptr<FrameState> frame, std::shared_ptr<Camera> cam,
    std::shared_ptr<GLResourceManager> gl,
    std::shared_ptr<input::InputDispatch> pointer_dispatch,
    std::shared_ptr<input::keyboard::Dispatch> keyboard_dispatch)
    : imgui_(absl::make_unique<imgui::ImGuiBridge>(
          frame, cam, gl, pointer_dispatch, keyboard_dispatch)) {}

void ImGuiDebugView::Update(ink::FrameTimeS t) {
  // imgui bridge needs to update before the commands begin so that it starts
  // a new frame.
  imgui_->Update(t);
  ImGui::Begin(
      "Ink Debug", nullptr,
      ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("Hello world!");
  ImGui::End();
}

void ImGuiDebugView::Draw(ink::RootRenderer::RenderOrder at_order,
                          const ink::Camera& draw_cam,
                          ink::FrameTimeS draw_time) const {
  imgui_->Draw(at_order, draw_cam, draw_time);
}

}  // namespace ink
