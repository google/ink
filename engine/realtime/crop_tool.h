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

#ifndef INK_ENGINE_REALTIME_CROP_TOOL_H_
#define INK_ENGINE_REALTIME_CROP_TOOL_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/realtime/crop_controller.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {
namespace tools {

// CropTool provides a thin wrapper around CropController that allows it to act
// as a tool. It is expected that no app will use both CropTool and CropMode,
// as interacting with both is undefined.
class CropTool : public Tool {
 public:
  explicit CropTool(const service::UncheckedRegistry& registry);
  ~CropTool() override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override {
    // We do everything in AfterSceneDrawn.
  }
  void AfterSceneDrawn(const Camera& cam, FrameTimeS draw_time) const override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  void Enable(bool enabled) override;

  void Update(const Camera& cam, FrameTimeS draw_time) override {
    crop_controller_->Update(cam);
  }

  inline std::string ToString() const override { return "<CropTool>"; }

 private:
  std::shared_ptr<CropController> crop_controller_;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_CROP_TOOL_H_
