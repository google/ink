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

#ifndef INK_ENGINE_REALTIME_SELECTORS_RECT_SELECTOR_H_
#define INK_ENGINE_REALTIME_SELECTORS_RECT_SELECTOR_H_

#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/tap_reco.h"
#include "ink/engine/realtime/selectors/selector.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// A selector that selects elements by querying the scene graph in the rectangle
// from the TDown input to the TUp. If the layer manager is active, only
// selects elements in the active layer.
//
// On creation, can be configured to select only a single element on a
// tap input.
class RectSelector : public Selector {
 public:
  RectSelector(const service::UncheckedRegistry& registry,
               const glm::vec4 color,
               // If true, a tap motion should only select a single element
               bool tap_for_single_selection);

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  bool HasSelectedElements() const override { return !selected_.empty(); }
  std::vector<ElementId> SelectedElements() const override { return selected_; }
  void Reset() override;

 private:
  void Select(input::InputType input_type, Rect region, const Camera& cam,
              bool only_one_element);

  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<LayerManager> layer_manager_;
  std::vector<ElementId> selected_;
  Shape shape_;
  ShapeRenderer shape_renderer_;
  glm::vec2 down_pos_world_{0, 0};
  bool is_selecting_;
  input::TapReco tap_reco_;
  bool tap_for_single_selection_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_SELECTORS_RECT_SELECTOR_H_
