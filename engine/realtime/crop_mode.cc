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

#include "ink/engine/realtime/crop_mode.h"

namespace ink {

CropMode::RenderableCropController::RenderableCropController(
    std::shared_ptr<tools::CropController> crop_controller)
    : crop_controller_(std::move(crop_controller)) {}

void CropMode::RenderableCropController::Draw(
    RootRenderer::RenderOrder at_order, const Camera& cam,
    FrameTimeS draw_time) const {
  if (at_order == RootRenderer::RenderOrder::End) {
    crop_controller_->Draw(cam, draw_time);
  }
}

CropMode::CropMode(std::shared_ptr<tools::CropController> crop_controller,
                   std::shared_ptr<input::InputDispatch> input,
                   std::shared_ptr<RootRenderer> root_renderer,
                   std::shared_ptr<settings::Flags> flags)
    : renderable_crop_controller_(crop_controller),
      input_(input),
      root_renderer_(root_renderer) {
  flags->AddListener(this);
}

void CropMode::OnFlagChanged(settings::Flag which, bool new_value) {
  if (which == settings::Flag::CropModeEnabled) {
    renderable_crop_controller_.crop_controller_->Enable(new_value);
    if (new_value) {
      renderable_crop_controller_.crop_controller_->SetInteriorInputPolicy(
          tools::CropController::InteriorInputPolicy::kPassthrough);
      root_renderer_->AddDrawable(&renderable_crop_controller_);
      input_token_ = input_->RegisterHandler(
          renderable_crop_controller_.crop_controller_.get());
    } else {
      root_renderer_->RemoveDrawable(&renderable_crop_controller_);
      input_->UnregisterHandler(input_token_);
    }
  }
}

void CropMode::Update(const Camera& cam) {
  renderable_crop_controller_.crop_controller_->Update(cam);
}

}  // namespace ink
