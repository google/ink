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

#ifndef INK_ENGINE_REALTIME_SELECTORS_PUSHER_SELECTOR_H_
#define INK_ENGINE_REALTIME_SELECTORS_PUSHER_SELECTOR_H_

#include <memory>
#include <utility>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/realtime/selectors/selector.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// Selects elements in two ways:
// - the top element at the TDown of the first finger
// - the top element intersected by the line segment drawn from the TDown of the
//   first finger to the TDown of the second finger
class PusherSelector : public Selector {
 public:
  explicit PusherSelector(std::shared_ptr<SceneGraph> scene_graph)
      : scene_graph_(std::move(scene_graph)),
        has_selected_(false),
        selected_id_(kInvalidElementId),
        first_down_pos_(0, 0) {}

  void Draw(const Camera &camera, FrameTimeS draw_time) const override {}
  input::CaptureResult OnInput(const input::InputData &data,
                               const Camera &camera) override;
  bool HasSelectedElements() const override { return has_selected_; }
  std::vector<ElementId> SelectedElements() const override;
  void Reset() override;

 private:
  std::shared_ptr<SceneGraph> scene_graph_;
  bool has_selected_;
  ElementId selected_id_;
  glm::vec2 first_down_pos_{0, 0};
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_SELECTORS_PUSHER_SELECTOR_H_
