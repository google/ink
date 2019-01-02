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

#ifndef INK_ENGINE_REALTIME_MAGIC_ERASER_H_
#define INK_ENGINE_REALTIME_MAGIC_ERASER_H_

#include <memory>
#include <unordered_set>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/tap_reco.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg_helper.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// A tool that "erases" (removes from the scene) entire elements.
// The elements removed are determined by:
//   1) if the input is a Tap, then the top-most element tapped.
//   2) Otherwise, constructs a segment query for each segment of the input
//      polyline and removes all intersected elements.
//
// If the layer manager is active, only elements in the active layer
// will be removed.
//
// At construction time, it can be configured to only accept inputs from
// the stylus "eraser".
//
class MagicEraser : public Tool {
 public:
  explicit MagicEraser(const service::UncheckedRegistry& registry);
  MagicEraser(std::shared_ptr<input::InputDispatch> dispatch,
              std::shared_ptr<SceneGraph> scene_graph,
              std::shared_ptr<IDbgHelper> dbg_helper,
              std::shared_ptr<LayerManager> layer_manager);
  MagicEraser(std::shared_ptr<input::InputDispatch> dispatch,
              std::shared_ptr<SceneGraph> scene_graph,
              std::shared_ptr<IDbgHelper> dbg_helper,
              std::shared_ptr<LayerManager> layer_manager,
              bool only_handle_eraser);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& live_camera) override;

  // ITool
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void Enable(bool enabled) override;

  inline std::string ToString() const override { return "<MagicEraser>"; }

 private:
  void Cancel();
  void Commit();

  std::unordered_set<ElementId, ElementIdHasher> intersected_elements_;
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<IDbgHelper> dbg_helper_;
  std::shared_ptr<LayerManager> layer_manager_;
  input::TapReco tap_reco_;
  const bool only_handle_eraser_;
};

}  // namespace ink
#endif  // INK_ENGINE_REALTIME_MAGIC_ERASER_H_
