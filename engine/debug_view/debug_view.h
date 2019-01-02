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

#ifndef INK_ENGINE_DEBUG_VIEW_DEBUG_VIEW_H_
#define INK_ENGINE_DEBUG_VIEW_DEBUG_VIEW_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// No-op debug view. See imgui_debug_view.cc for actual debug view.
class DebugView : public RootRenderer::DrawListener {
 public:
  // Per-frame update.
  virtual void Update(FrameTimeS t) {}

  // RootRenderer::DrawListener
  void Draw(RootRenderer::RenderOrder at_order, const Camera& draw_cam,
            FrameTimeS draw_time) const override {}
};

// Instantiates a no-op DebugViewInterface by default or an actual DebugView
// if enabled at compile time (--define debug_view=1)
void DefineDebugView(service::DefinitionList* definitions);

}  // namespace ink

#endif  // INK_ENGINE_DEBUG_VIEW_DEBUG_VIEW_H_
