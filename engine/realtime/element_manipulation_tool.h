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

#include "third_party/absl/types/optional.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/input/cursor.h"
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
  absl::optional<input::Cursor> CurrentCursor(
      const Camera& camera) const override;

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

  void SetHandlesEnabled(bool enabled) { allow_handles_ = enabled; }
  void SetDeselectWhenOutside(bool deselect) { allow_deselect_ = deselect; }

  // Calculates a transform matrix for input::DragData that preserves floating
  // point precision when applied to the ranges specified by smallest and
  // largest regions.  This is public for the sake of unit tests.
  //
  // In some cases:
  //   - Only a partial transform will be returned
  //   - A transform that loses precision will be returned. (As minimal as
  //     possible)
  //
  // Parameters:
  //   drag: the input drag data for which to compute a transform
  //   handle: which resize handle is being dragged, if any
  //   to: the input position, i.e. where we're dragging to (world coordinates)
  //   smallest: the MBR of the smallest selected element (world coordinates)
  //   largest: the MBR for all selected elements (world coordinates)
  //   region: the selection box (world coordinates)
  //   allow_rotation: if false, only allow scale/translate
  //   maintain_aspect_ratio: if true, prevents non-uniform scaling
  static glm::mat4 BestTransformForRegions(input::DragData drag,
                                           ElementManipulationToolHandle handle,
                                           glm::vec2 to, Rect smallest,
                                           Rect largest, RotRect region,
                                           bool allow_rotation,
                                           bool maintain_aspect_ratio);

  inline std::string ToString() const override {
    return "<ElementManipulationTool>";
  }

 private:
  // attempts to modify currentRegion_ to move along with drag, checking for
  // precision loss
  void AttemptToTransformRegion(input::DragData drag, glm::vec2 to,
                                bool maintain_aspect_ratio, bool multitouch);
  void Commit();
  void Cancel();
  void Reset();

  // Which resize handle the user is currently grabbing, or NONE if no handle is
  // being grabbed.
  ElementManipulationToolHandle manipulating_handle_ =
      ElementManipulationToolHandle::NONE;

  // True if the selection box or a resize handle is currently being dragged.
  bool is_manipulating_ = false;

  // If false, resize handles will be disabled for this tool, even if
  // Flag::EnableSelectionBoxHandles is set to true.  This exists so that the
  // pusher tool (which does not render the selection box) can disable the
  // resize handles.
  bool allow_handles_ = true;

  // If true, tapping outside the selection box will deselect the element(s).
  bool allow_deselect_ = true;

  // The bounding rectangle of the selected elements, in world coords, fixed at
  // selection/commit time (i.e. not taking current_transform_ into account).
  Rect element_mbr_;

  // The mbr of the smallest selected element. Used to prevent accumulation
  // of fp precision errors while manipulating groups of elements.
  Rect smallest_element_mbr_;

  // A possibly-rotated rectangle that bounds the selected elements, in world
  // coords, fixed at selection/commit time (i.e. not taking current_transform_
  // into account).  When the elements are first selected, this is just
  // element_mbr_, but can become rotated if a rotation transform is committed.
  RotRect selected_region_;

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
