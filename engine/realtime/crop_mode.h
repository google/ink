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

#ifndef INK_ENGINE_REALTIME_CROP_MODE_H_
#define INK_ENGINE_REALTIME_CROP_MODE_H_

#include <memory>
#include "ink/engine/realtime/crop_controller.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/settings/flags.h"

namespace ink {

// CropMode allows the cropping rectangle to be manipulated regardless of which
// tool is in use. The crop mode is enabled with the CropModeEnabled flag. It
// should not be used in conjunction with the CropTool.
class CropMode : public settings::FlagListener {
 public:
  using SharedDeps =
      service::Dependencies<tools::CropController, input::InputDispatch,
                            RootRenderer, settings::Flags>;

  explicit CropMode(std::shared_ptr<tools::CropController> crop_controller,
                    std::shared_ptr<input::InputDispatch> input,
                    std::shared_ptr<RootRenderer> root_renderer,
                    std::shared_ptr<settings::Flags> flags);

  // CropMode is neither copyable nor movable.
  CropMode(const CropMode&) = delete;
  CropMode& operator=(const CropMode&) = delete;

  void OnFlagChanged(settings::Flag which, bool new_value) override;

  void Update(const Camera& cam);

 private:
  // Wrapper around a CropController that can take a renderorder argument.
  class RenderableCropController : public RootRenderer::DrawListener {
   public:
    explicit RenderableCropController(
        std::shared_ptr<tools::CropController> crop_controller);
    void Draw(RootRenderer::RenderOrder, const Camera& cam,
              FrameTimeS draw_time) const override;

   private:
    friend class CropMode;  // Allow direct access to crop_controller_;
    std::shared_ptr<tools::CropController> crop_controller_;
  };

  RenderableCropController renderable_crop_controller_;
  std::shared_ptr<input::InputDispatch> input_;
  std::shared_ptr<RootRenderer> root_renderer_;

  uint32_t input_token_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_CROP_MODE_H_
