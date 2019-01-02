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

#ifndef RESEARCH_INK_LF_GUI_IMGUI_imgui_bridge_H_
#define RESEARCH_INK_LF_GUI_IMGUI_imgui_bridge_H_

#include "third_party/dear_imgui/imgui.h"
#include "ink/engine/geometry/mesh/gl/vbo.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/public/contrib/imgui/framewise_input.h"
#include "ink/public/contrib/imgui/imgui_shader.h"
#include "ink/public/contrib/keyboard_input/keyboard_dispatch.h"
#include "ink/public/contrib/keyboard_input/keyboard_handler.h"

namespace ink {
namespace imgui {

// Provides a link between Dear ImGui and the sketchology rendering engine.
// Instantiate this class, call Update + Draw each frame, and enjoy usage
// of the ImGui framework. See https://github.com/ocornut/imgui for details.
class ImGuiBridge : public RootRenderer::DrawListener,
                    public input::keyboard::EventHandler {
 public:
  struct FontConfig {
    std::vector<char> raw_font_bytes;
    std::vector<int> desired_px;
  };
  using SharedDeps =
      service::Dependencies<FrameState, Camera, GLResourceManager,
                            input::InputDispatch, input::keyboard::Dispatch>;

  // RAII guard for calling SetEnabled.
  class ImGuiEnableScope {
   public:
    // Calls bridge->SetEnabled(enabled_).
    ImGuiEnableScope(bool enabled, ImGuiBridge* bridge);
    // Calls bridge->SetEnabled(!enabled_).
    ~ImGuiEnableScope();

   private:
    bool enabled_;
    ImGuiBridge* bridge_;
  };

  // Construct an ImGui bridge. If fonts_to_load is empty a default font will be
  // provided.
  ImGuiBridge(std::shared_ptr<FrameState> frame, std::shared_ptr<Camera> cam,
              std::shared_ptr<GLResourceManager> gl,
              std::shared_ptr<input::InputDispatch> pointer_dispatch,
              std::shared_ptr<input::keyboard::Dispatch> keyboard_dispatch,
              std::vector<FontConfig> fonts_to_load = {},
              bool show_demo_window = false);
  ~ImGuiBridge() override;

  void Update(FrameTimeS t);
  void Draw(RootRenderer::RenderOrder at_order, const Camera& cam,
            FrameTimeS draw_time) const override;

  // keyboard::EventHandler
  void HandleEventAsFirstResponder(
      const input::keyboard::Event& event) override;

  // Returns a RAII-style marker that causes ImGui inputs to be
  // disabled for the duration of its lifetime. Use like:
  //
  // std::unique_ptr<ImGuiBridge> bridge = ...;
  // {
  //   auto _ = bridge->DisableForScope();
  //   /* do stuff without ImGui intercepting your input events */
  // }
  // /* now ImGui is active again */
  ImGuiEnableScope DisableForScope();

 private:
  void SetEnabled(bool b);
  void NewFrame(FrameTimeS t);
  void MaybeInit();

  std::shared_ptr<FrameState> frame_;
  std::shared_ptr<Camera> cam_;
  std::shared_ptr<GLResourceManager> gl_;
  std::shared_ptr<input::keyboard::Dispatch> keyboard_dispatch_;
  Texture font_tex_;
  ImGuiShader shader_;
  bool did_init_ = false;
  bool show_demo_window_ = false;
  // Allows external control of whether we intercept events.
  bool enabled_ = true;
  FramewiseInput frame_input_;
  std::vector<FontConfig> fonts_to_load_;
};

}  // namespace imgui
}  // namespace ink

#endif  // RESEARCH_INK_LF_GUI_IMGUI_imgui_bridge_H_
