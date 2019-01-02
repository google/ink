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

#include "ink/engine/rendering/compositing/live_renderer.h"

namespace ink {

LiveRenderer::~LiveRenderer() {}
LiveRenderer::LiveRenderer(std::shared_ptr<SceneGraph> scene_graph,
                           std::shared_ptr<FrameState> frame_state,
                           std::shared_ptr<GLResourceManager> gl_resources,
                           std::shared_ptr<LayerManager> layer_manager,
                           std::shared_ptr<input::InputDispatch> input_dispatch,
                           std::shared_ptr<WallClockInterface> wall_clock,
                           std::shared_ptr<PageManager> page_manager)
    : scene_graph_(std::move(scene_graph)),
      frame_state_(std::move(frame_state)),
      gl_resources_(std::move(gl_resources)),
      layer_manager_(std::move(layer_manager)),
      input_dispatch_(std::move(input_dispatch)),
      wall_clock_(std::move(wall_clock)),
      page_manager_(std::move(page_manager)) {}

SceneGraphRenderer* LiveRenderer::delegate() const {
  if (!delegate_) {
    switch (strategy_) {
      case RenderingStrategy::kBufferedRenderer:
        SLOG(SLOG_INFO, "Creating buffered renderer.");
        delegate_ = absl::make_unique<TripleBufferedRenderer>(
            frame_state_, gl_resources_, input_dispatch_, scene_graph_,
            wall_clock_, page_manager_, layer_manager_);
        break;
      case RenderingStrategy::kDirectRenderer:
        SLOG(SLOG_INFO, "Creating direct renderer.");
        delegate_ = absl::make_unique<DirectRenderer>(
            scene_graph_, frame_state_, gl_resources_, layer_manager_);
        break;
    }
    delegate_->Resize(cached_size_);
  }
  return delegate_.get();
}

void LiveRenderer::Use(RenderingStrategy rendering_strategy) {
  if (rendering_strategy != strategy_) {
    delegate_.reset();
    strategy_ = rendering_strategy;
  }
}

void LiveRenderer::Update(const Timer& timer, const Camera& cam,
                          FrameTimeS draw_time) {
  delegate()->Update(timer, cam, draw_time);
}

glm::ivec2 LiveRenderer::RenderingSize() const {
  return delegate()->RenderingSize();
}

void LiveRenderer::DrawAfterTool(const Camera& cam,
                                 FrameTimeS draw_time) const {
  delegate()->DrawAfterTool(cam, draw_time);
}

void LiveRenderer::Synchronize(FrameTimeS draw_time) {
  delegate()->Synchronize(draw_time);
}

void LiveRenderer::Draw(const Camera& cam, FrameTimeS draw_time) const {
  delegate()->Draw(cam, draw_time);
}

// These two functions do not auto-vivify a delegate renderer when called; they
// no-op if no delegate has been constructed yet.

// We handle resize specially, since when a new renderer is created, it needs to
// know the last size that was explicitly set (in order to, e.g., generate back
// buffers).
void LiveRenderer::Resize(glm::ivec2 size) {
  cached_size_ = size;
  if (delegate_) delegate_->Resize(size);
}

void LiveRenderer::Invalidate() {
  if (delegate_) delegate_->Invalidate();
}

}  // namespace ink
