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

#ifndef INK_ENGINE_REALTIME_TOOL_H_
#define INK_ENGINE_REALTIME_TOOL_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

enum class InputRegistrationPolicy {
  ACTIVE,  // The tool should register itself with input dispatch
  PASSIVE  // The tool will be given input events.
};

// Tool is the parent class for all tools which are used by ToolController.
class Tool : public IDrawable, public input::InputHandler {
 public:
  Tool() : Tool(input::Priority::Default) {}
  explicit Tool(input::Priority priority)
      : input::InputHandler(priority), enabled_(true) {}
  ~Tool() override {}

  // Called after the background but before the rest of the scene is drawn.
  // The GL surface is clipped to the scene bounds.
  virtual void BeforeSceneDrawn(const Camera& camera,
                                FrameTimeS draw_time) const {}

  // Called after the scene and any border are drawn.
  // The GL surface is NOT clipped to the scene bounds.
  // If your tool has elements that extend beyond the scene bounds, this is the
  // place to draw them.
  virtual void AfterSceneDrawn(const Camera& camera,
                               FrameTimeS draw_time) const {}

  // Set the color for this tool, no-op by default.
  virtual void SetColor(glm::vec4 rgba) {}
  virtual glm::vec4 GetColor() { return glm::vec4(0); }

  virtual void Update(const Camera& cam, FrameTimeS draw_time) {}

  virtual bool Enabled() const { return enabled_; }
  virtual void Enable(bool enabled) {
    enabled_ = enabled;
    SetRefuseAllNewInput(!enabled);
  }

  inline std::string ToString() const override { return "<Tool>"; }

 private:
  bool enabled_;
};

// Tool that doesn't do anything
class NullTool : public Tool {
 public:
  void Draw(const Camera& cam, FrameTimeS draw_time) const override {}
  void Update(const Camera& cam, FrameTimeS draw_time) override {}
  bool Enabled() const override { return false; }
  void Enable(bool enabled) override {}
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override {
    return input::CapResRefuse;
  }
};

}  // namespace ink
#endif  // INK_ENGINE_REALTIME_TOOL_H_
