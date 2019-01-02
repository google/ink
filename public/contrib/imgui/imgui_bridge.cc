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

#include "ink/public/contrib/imgui/imgui_bridge.h"

#include <numeric>
#include "ink/engine/gl.h"
#include "ink/engine/util/dbg/glerrors.h"

namespace ink {
namespace input {
namespace keyboard {
void InitKeyMap(ImGuiIO* io) {
  io->KeyMap[ImGuiKey_Tab] = kTab;
  io->KeyMap[ImGuiKey_LeftArrow] = kLeftArrow;
  io->KeyMap[ImGuiKey_RightArrow] = kRightArrow;
  io->KeyMap[ImGuiKey_UpArrow] = kUpArrow;
  io->KeyMap[ImGuiKey_DownArrow] = kDownArrow;
  io->KeyMap[ImGuiKey_PageUp] = kPageUp;
  io->KeyMap[ImGuiKey_PageDown] = kPageDown;
  io->KeyMap[ImGuiKey_Home] = kHome;
  io->KeyMap[ImGuiKey_End] = kEnd;
  io->KeyMap[ImGuiKey_Delete] = kDelete;
  io->KeyMap[ImGuiKey_Backspace] = kBackspace;
  io->KeyMap[ImGuiKey_Enter] = kEnter;
  io->KeyMap[ImGuiKey_Escape] = kEscape;
  io->KeyMap[ImGuiKey_A] = kA;
  io->KeyMap[ImGuiKey_C] = kC;
  io->KeyMap[ImGuiKey_V] = kV;
  io->KeyMap[ImGuiKey_X] = kX;
  io->KeyMap[ImGuiKey_Y] = kY;
  io->KeyMap[ImGuiKey_Z] = kZ;
}
}  // namespace keyboard
}  // namespace input

namespace imgui {
using input::keyboard::Keycode;

ImGuiBridge::ImGuiEnableScope::ImGuiEnableScope(const bool enabled,
                                                ImGuiBridge* bridge)
    : enabled_(enabled), bridge_(bridge) {
  bridge_->SetEnabled(enabled_);
}

ImGuiBridge::ImGuiEnableScope::~ImGuiEnableScope() {
  bridge_->SetEnabled(!enabled_);
}

ImGuiBridge::ImGuiBridge(
    std::shared_ptr<FrameState> frame, std::shared_ptr<Camera> cam,
    std::shared_ptr<GLResourceManager> gl,
    std::shared_ptr<input::InputDispatch> pointer_dispatch,
    std::shared_ptr<input::keyboard::Dispatch> keyboard_dispatch,
    std::vector<FontConfig> fonts_to_load, bool show_demo_window)
    : frame_(frame),
      cam_(cam),
      gl_(gl),
      keyboard_dispatch_(keyboard_dispatch),
      font_tex_(gl->gl),
      shader_(gl),
      show_demo_window_(show_demo_window),
      frame_input_(pointer_dispatch, frame),
      fonts_to_load_(std::move(fonts_to_load)) {}

ImGuiBridge::~ImGuiBridge() { ImGui::Shutdown(); }

void ImGuiBridge::Update(FrameTimeS t) {
  if (enabled_) {
    NewFrame(t);
    if (show_demo_window_) {
      ImGui::ShowTestWindow();
    }
  }
}

void ImGuiBridge::Draw(ink::RootRenderer::RenderOrder at_order,
                       const Camera& cam, FrameTimeS draw_time) const {
  if (at_order != ink::RootRenderer::RenderOrder::End) {
    return;
  }
  ImGui::Render();
  ImGuiIO& io = ImGui::GetIO();
  ImDrawData* draw_data = ImGui::GetDrawData();
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);
  shader_.Use(cam);
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    shader_.Draw(cam, draw_data->CmdLists[n]);
  }
  shader_.Unuse();
}

ImGuiBridge::ImGuiEnableScope ImGuiBridge::DisableForScope() {
  return ImGuiBridge::ImGuiEnableScope(false, this);
}

void ImGuiBridge::SetEnabled(const bool b) {
  enabled_ = b;
  frame_input_.SetEnabled(b);
}

namespace {
// Glyph ranges need to outlive the font.
const ImWchar* GetGlyphRanges() {
  static const ImWchar ranges[] = {
      0x0020, 0x00FF,  // Basic Latin + Latin Supplement
      0x2400, 0x243F,  // Control Pictures
      0x2190, 0x21FF,  // Arrows
      0,
  };
  return &ranges[0];
}
}  // namespace

void ImGuiBridge::MaybeInit() {
  if (did_init_) return;
  did_init_ = true;

  float scale =
      cam_->ConvertDistance(1, DistanceType::kScreen, DistanceType::kDp);
  ImGuiIO& io = ImGui::GetIO();
  input::keyboard::InitKeyMap(&io);
  for (auto font_to_load : fonts_to_load_) {
    ImFontConfig imfont_config;
    imfont_config.FontDataOwnedByAtlas = false;
    imfont_config.GlyphRanges = GetGlyphRanges();
    for (auto desired_px : font_to_load.desired_px) {
      io.Fonts->AddFontFromMemoryTTF(
          static_cast<void*>(font_to_load.raw_font_bytes.data()),
          font_to_load.raw_font_bytes.size(), std::lround(desired_px),
          &imfont_config);
    }
  }
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
  ClientBitmapWrapper pixel_bitmap(pixels, ImageSize(width, height),
                                   ImageFormat::BITMAP_FORMAT_A_8);
  font_tex_.Load(pixel_bitmap, TextureParams());
  io.Fonts->TexID = &font_tex_;
  shader_.Load();

  ImGuiStyle& style = ImGui::GetStyle();
  glm::vec2 pad_amt{scale * cam_->ConvertDistance(4, DistanceType::kDp,
                                                  DistanceType::kScreen)};
  style.TouchExtraPadding = ImVec2(pad_amt.x, pad_amt.y);
  style.WindowPadding =
      ImVec2(scale * style.WindowPadding.x, scale * style.WindowPadding.y);
  style.FramePadding =
      ImVec2(scale * style.FramePadding.x, scale * style.FramePadding.y);
}

void ImGuiBridge::HandleEventAsFirstResponder(
    const input::keyboard::Event& event) {
  if (auto utf8 = absl::get_if<input::keyboard::UTF8InputEvent>(&event)) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(utf8->text.data());
  }
}

void ImGuiBridge::NewFrame(FrameTimeS t) {
  ImGuiIO& io = ImGui::GetIO();
  MaybeInit();
  io.DisplaySize = ImVec2(cam_->ScreenDim().x, cam_->ScreenDim().y);
  io.DisplayFramebufferScale = ImVec2(1, 1);

  if (frame_->GetLastFrameTime() == FrameTimeS(0)) {
    io.DeltaTime = .00001;
  } else {
    io.DeltaTime = static_cast<float>(t - frame_->GetLastFrameTime());
  }

  frame_input_.NewFrame();
  bool was_mouse_down = io.MouseDown[0] || io.MouseDown[2];
  if (frame_input_.LastTrackingState() !=
          FramewiseInput::TrackingState::kNone &&
      frame_input_.SawAnyNonWheelInputLastFrame()) {
    auto input = frame_input_.LastInput();
    io.MousePos.x = input.screen_pos.x;
    io.MousePos.y = cam_->ScreenDim().y - input.screen_pos.y;
    io.MouseDown[0] = false;
    io.MouseDown[1] = false;
    io.MouseDown[2] = false;
    if (input.Get(input::Flag::InContact)) {
      if (input.Get(input::Flag::Right)) {
        io.MouseDown[2] = true;
      } else {
        io.MouseDown[0] = true;
      }
    }
  }
  if (frame_input_.LastWheelDelta() != 0) {
    io.MouseWheel = frame_input_.LastWheelDelta() > 0 ? 1 : -1;
  }

  auto keystate = keyboard_dispatch_->GetState();
  for (typename std::underlying_type<Keycode>::type keycode =
           input::keyboard::kLowerLimit;
       keycode < input::keyboard::kUpperLimit; keycode++) {
    io.KeysDown[static_cast<unsigned char>(keycode)] =
        keystate.IsDown(static_cast<Keycode>(keycode));
  }
  io.KeyShift = keystate.IsModifierDown(input::keyboard::kShiftModifier);
  io.KeyCtrl = keystate.IsModifierDown(input::keyboard::kControlModifier);
  io.KeyShift = keystate.IsModifierDown(input::keyboard::kShiftModifier);
  io.KeyAlt = keystate.IsModifierDown(input::keyboard::kAltModifier);
  io.KeySuper = keystate.IsModifierDown(input::keyboard::kSuperModifier);

  // Start the frame
  ImGui::NewFrame();

  frame_input_.SetCapture(io.WantCaptureMouse);
  if (io.WantCaptureKeyboard) {
    keyboard_dispatch_->BecomeFirstResponder(this);
  } else {
    keyboard_dispatch_->DiscardFirstResponder(this);
  }

  // Get at least one extra frame after every transition we send to imgui.
  bool is_mouse_down = io.MouseDown[0] || io.MouseDown[2];
  if (io.WantCaptureMouse && (was_mouse_down != is_mouse_down)) {
    frame_->RequestFrame();
  }
}

}  // namespace imgui
}  // namespace ink
