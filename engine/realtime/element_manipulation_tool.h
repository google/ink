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

#ifndef INK_ENGINE_REALTIME_ELEMENT_MANIPULATION_TOOL_H_
#define INK_ENGINE_REALTIME_ELEMENT_MANIPULATION_TOOL_H_

#include <functional>
#include <memory>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/drag_reco.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/tap_reco.h"
#include "ink/engine/realtime/element_manipulation_tool_renderer.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/baseGL/render_target.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/compositing/single_partition_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

class ElementManipulationTool : public Tool, public SceneGraphListener {
 public:
  ElementManipulationTool(
      const service::UncheckedRegistry& registry, bool register_for_input,
      std::function<void(void)> cancel_callback,
      std::unique_ptr<ElementManipulationToolRendererInterface> renderer);
  ~ElementManipulationTool() override;

  // Note: SetElements() will cause an immediate Cancel/Reset if all of the
  // provided elements are off screen for the provided Camera.
  void SetElements(const Camera& cam, const std::vector<ElementId>& elements);

  const std::vector<ElementId>& GetElements() const;

  // ITool
  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void Update(const Camera& cam, FrameTimeS draw_time) override;
  void Enable(bool enabled) override;

  // IInputHandler
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  // SceneGraphListener
  void OnElementAdded(SceneGraph* graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override;

  glm::mat4 GetTransform() const { return current_transform_; }
  bool IsManipulating() const { return is_manipulating_; }

  void SetDeselectWhenOutside(bool deselect) { allow_deselect_ = deselect; }

  //
  // Calculates a transform matrix for input::DragData that preserves floating
  // point precision when applied to the ranges specified by smallest and
  // largest regions.
  //
  // In some cases:
  //   - Only a partial transform will be returned
  //   - A transform that loses precision will be returned. (As minimal as
  //     possible)
  static glm::mat4 BestTransformForRegions(input::DragData drag, Rect smallest,
                                           Rect largest, bool allow_rotation);
  static bool IsTransformSafeForRegion(glm::mat4 transform, Rect region);

  inline std::string ToString() const override {
    return "<ElementManipulationTool>";
  }

 private:
  // attempts to modify currentRegion_ to move along with drag, checking for
  // precision loss
  void AttemptToTransformRegion(input::DragData drag);
  void Commit();
  void Cancel();
  void Reset();

  bool is_manipulating_;
  bool allow_deselect_;

  // the bounding rectangle of the selected elements.
  // In world coords, fixed at selection time
  Rect element_mbr_;

  // The mbr of the smallest selected element. Used to prevent accumulation
  // of fp precision errors while manipulating groups of elements.
  Rect smallest_element_mbr_;

  glm::mat4 current_transform_{1};
  std::vector<ElementId> elements_;
  std::vector<glm::mat4> element_transforms_;
  std::function<void(void)> cancel_callback_;
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<settings::Flags> flags_;

  // For selection movement
  input::DragReco drag_reco_;
  input::TapReco tap_reco_;

  // Rendering
  std::unique_ptr<ElementManipulationToolRendererInterface> renderer_;
  Camera start_camera_;

  friend class ElementManipulationToolTestFriend;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_ELEMENT_MANIPULATION_TOOL_H_
