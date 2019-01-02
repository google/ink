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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_SCENE_GRAPH_RENDERER_H_
#define INK_ENGINE_RENDERING_COMPOSITING_SCENE_GRAPH_RENDERER_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"

namespace ink {

// Responsible for drawing:
//   1. The background
//   2. The elements of the scene graph
//   3. The real time drawables in the scene graph
class SceneGraphRenderer : public IDrawable {
 public:
  ~SceneGraphRenderer() override {}

  // Use the time alloted by "timer" to update internal caches
  virtual void Update(const Timer& timer, const Camera& cam,
                      FrameTimeS draw_time) = 0;

  virtual void Resize(glm::ivec2 size) = 0;
  virtual glm::ivec2 RenderingSize() const = 0;

  // Called _after_ the current tool has been rendered.
  virtual void DrawAfterTool(const Camera& cam, FrameTimeS draw_time) const {}

  // Discard any cached rendering data and begin rendering from scratch.
  virtual void Invalidate() = 0;

  // Update the renderer in a blocking manner until it matches the state of the
  // scene graph.
  //
  // AVOID USE!!!
  //
  // SceneGraphRenderer is designed to be asynchronous. Calling Synchronize()
  // will cause poor performance on large docs under some SceneGraphRenderer
  // implementations (e.g. TripleBufferedRenderer)
  virtual void Synchronize(FrameTimeS draw_time) = 0;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_COMPOSITING_SCENE_GRAPH_RENDERER_H_
