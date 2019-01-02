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

#ifndef INK_ENGINE_REALTIME_PUSHER_TOOL_H_
#define INK_ENGINE_REALTIME_PUSHER_TOOL_H_

#include <memory>
#include <unordered_map>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/realtime/element_manipulation_tool.h"
#include "ink/engine/realtime/selectors/pusher_selector.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace tools {

// Tool used to manipulate an element with a single touch sequence (possibly
// using multiple pointers).
class PusherTool : public Tool, public FrameStateListener {
 public:
  explicit PusherTool(const service::UncheckedRegistry& registry);

  void Draw(const Camera& cam, FrameTimeS draw_time) const override {
    // All drawing done after scene.
  }
  void AfterSceneDrawn(const Camera& cam, FrameTimeS draw_time) const override;
  void Update(const Camera& cam, FrameTimeS draw_time) override;
  void Enable(bool enabled) override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  void SetPusherToolParams(const proto::PusherToolParams& unsafe_params_proto);

  void OnFrameEnd() override;

  inline std::string ToString() const override { return "<PusherTool>"; }

 private:
  // Reset() will send the final position update if it's currently manipulating.
  void Reset();
  void SendPositionUpdate() const;
  void TapCallback(const Camera& cam, glm::vec2 down_world, glm::vec2 up_world);

  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<IEngineListener> engine_listener_;
  PusherSelector selector_;
  ElementManipulationTool manipulation_tool_;
  std::unordered_map<uint32_t, glm::vec2> last_position_;

  input::TapReco tap_reco_;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_PUSHER_TOOL_H_
