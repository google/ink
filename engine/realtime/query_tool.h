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

#ifndef INK_ENGINE_REALTIME_QUERY_TOOL_H_
#define INK_ENGINE_REALTIME_QUERY_TOOL_H_

#include <memory>
#include <vector>

#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/realtime/selectors/rect_selector.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// Implement a single-tap tool that invokes a platform-provided callback with  a
// list of elements at the tapped location with some information about them in
// the scene.  See ElementQueryData in elements.proto for the returned
// information.  If no elements are present, returns an empty list.
class QueryTool : public Tool {
 public:
  explicit QueryTool(const service::UncheckedRegistry& registry);
  ~QueryTool() override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  void Enable(bool enabled) override;

  inline std::string ToString() const override { return "<QueryTool>"; }

 private:
  void OnHitComplete(std::vector<ElementId> elements,
                     glm::vec2 down_world_coords, glm::vec2 up_world_coords,
                     const Camera& cam);

  RectSelector selector_;
  glm::vec2 down_pos_world_{0, 0};
  std::shared_ptr<IEngineListener> engine_listener_;
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<TextTextureProvider> text_texture_provider_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_QUERY_TOOL_H_
