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

#include "ink/engine/realtime/element_manipulation_tool.h"

#include <algorithm>  // copy_if
#include <cstddef>
#include <iterator>
#include <limits>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "third_party/glm/glm/gtx/projection.hpp"
#include "third_party/glm/glm/gtx/rotate_vector.hpp"
#include "third_party/glm/glm/gtx/vector_angle.hpp"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/compositing/partition_data.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"

namespace ink {
namespace tools {
namespace {

using RendererInterface = ElementManipulationToolRendererInterface;

// You must click/touch within RegionQuery::MinSelectionSizeCm(type) times this
// to be grabbing a resize handle.  (No that there's no principle to this exact
// value; I tried a few different values and this one felt about right.)
const float kMaxHandleDistFactor = 1.3;

}  // namespace

ElementManipulationTool::ElementManipulationTool(
    const service::UncheckedRegistry& registry, bool register_for_input,
    std::function<void(void)> cancel_callback,
    std::unique_ptr<RendererInterface> render_helper)
    : Tool(input::Priority::ManipulateSelection),
      cancel_callback_(cancel_callback),
      scene_graph_(registry.GetShared<SceneGraph>()),
      flags_(registry.GetShared<settings::Flags>()),
      renderer_(std::move(render_helper)) {
  drag_reco_.SetAllowOneFingerPan(true);
  scene_graph_->AddListener(this);
  if (register_for_input)
    RegisterForInput(registry.GetShared<input::InputDispatch>());
}

ElementManipulationTool::~ElementManipulationTool() {
  SLOG(SLOG_OBJ_LIFETIME, "ElementManipulationTool dtor");
  scene_graph_->RemoveListener(this);
}

input::CaptureResult ElementManipulationTool::OnInput(
    const input::InputData& data, const Camera& camera) {
  const auto tap_data = tap_reco_.OnInput(data, camera);

  if (data.Get(input::Flag::Right)) {
    Cancel();
    return input::CapResRefuse;
  }

  if (tap_data.IsTap() && tap_data.down_data.Get(input::Flag::Primary)) {
    Cancel();
    return input::CapResRefuse;
  }

  if (!is_manipulating_ && data.Get(input::Flag::InContact)) {
    // Find the nearest handle to the input position; if none of them are within
    // max_handle_dist, we'll default to NONE (meaning we're translating the
    // selection rather than resizing).  For this calculation, we pretend that
    // NONE is a real handle at the center of the rect; this ensures that it's
    // still possible to translate a very small selection, as long as the input
    // is closer to the center than to the edges.
    auto nearest_handle = ElementManipulationToolHandle::NONE;
    if (allow_handles_ &&
        flags_->GetFlag(settings::Flag::EnableSelectionBoxHandles)) {
      const float max_handle_dist = camera.ConvertDistance(
          kMaxHandleDistFactor * RegionQuery::MinSelectionSizeCm(data.type),
          DistanceType::kCm, DistanceType::kWorld);
      float nearest_world_dist = max_handle_dist;
      for (auto handle : kAllElementManipulationToolHandles) {
        if (handle == ElementManipulationToolHandle::ROTATION &&
            !flags_->GetFlag(settings::Flag::EnableRotation)) {
          continue;
        }
        glm::vec2 handle_world_pos = ElementManipulationToolHandlePosition(
            handle, camera, selected_region_);
        float world_dist = glm::distance(handle_world_pos, data.world_pos);
        if (world_dist < nearest_world_dist) {
          nearest_world_dist = world_dist;
          nearest_handle = handle;
        }
      }
    }
    // If deselection is allowed, and we're not grabbing any handle, and we're
    // well outside the selection region, we should deselect.
    if (allow_deselect_ &&
        nearest_handle == ElementManipulationToolHandle::NONE) {
      // If the contact is not near the selected region, deselect
      float mbr_buffer_cm = 1.5 * RegionQuery::MinSelectionSizeCm(data.type);
      float within_limit_world = camera.ConvertDistance(
          mbr_buffer_cm, DistanceType::kCm, DistanceType::kWorld);
      auto test_region = geometry::Transform(element_mbr_, GetTransform())
                             .Inset(glm::vec2{-within_limit_world});
      if (!test_region.Contains(data.world_pos)) {
        Cancel();
        return input::CapResRefuse;
      }
    }
    // Otherwise, we are now manipulating.
    is_manipulating_ = true;
    manipulating_handle_ = nearest_handle;
    drag_reco_.Reset();
  }

  if (data.n_down == 0) {
    if (is_manipulating_) {
      Commit();
    }
    is_manipulating_ = false;
    manipulating_handle_ = ElementManipulationToolHandle::NONE;
  }

  if (is_manipulating_) {
    drag_reco_.OnInput(data, camera);
    input::DragData drag;
    if (drag_reco_.GetDrag(&drag)) {
      AttemptToTransformRegion(drag, data.world_pos,
                               /*maintain_aspect_ratio=*/true,
                               /*multitouch=*/data.n_down > 1);
    }
  }

  return input::CapResCapture;
}

absl::optional<input::Cursor> ElementManipulationTool::CurrentCursor(
    const Camera& camera) const {
  if (!is_manipulating_) {
    return input::Cursor(input::CursorType::GRAB);
  }
  float direction_radians = 0.0;
  switch (manipulating_handle_) {
    case ElementManipulationToolHandle::NONE:
      return input::Cursor(input::CursorType::GRABBING);
    case ElementManipulationToolHandle::RIGHT:
    case ElementManipulationToolHandle::LEFT:
      direction_radians = 0.0;
      break;
    case ElementManipulationToolHandle::RIGHTTOP:
    case ElementManipulationToolHandle::LEFTBOTTOM:
      direction_radians = M_TAU / 8.0;
      break;
    case ElementManipulationToolHandle::TOP:
    case ElementManipulationToolHandle::BOTTOM:
      direction_radians = M_TAU / 4.0;
      break;
    case ElementManipulationToolHandle::LEFTTOP:
    case ElementManipulationToolHandle::RIGHTBOTTOM:
      direction_radians = M_TAU * 3.0 / 8.0;
      break;
    case ElementManipulationToolHandle::ROTATION:
      return input::Cursor(input::CursorType::GRABBING);
  }
  // For resize handles, we want to take into account the rotation of the
  // selection rect; for example, the TOP handle would normally use a RESIZE_NS
  // cursor, but if the selection box is rotated by 90 degrees, then a RESIZE_EW
  // cursor is better.  So, we add the direction_radians of the handle to the
  // rotation of the selected_region_, and then round to the closest compass
  // direction.
  direction_radians += selected_region_.Rotation();
  int direction_integer = std::lroundf(direction_radians / (M_TAU / 8.0));
  switch (ink::util::Mod(direction_integer, 4)) {
    case 0:
      return input::Cursor(input::CursorType::RESIZE_EW);
    case 1:
      return input::Cursor(input::CursorType::RESIZE_NESW);
    case 2:
      return input::Cursor(input::CursorType::RESIZE_NS);
    case 3:
    default:
      return input::Cursor(input::CursorType::RESIZE_NWSE);
  }
}

namespace {
bool IsTransformSafeForRegion(glm::mat4 transform, Rect region) {
  auto proposed_region = geometry::Transform(region, transform);
  bool res = proposed_region.Width() > 0 && proposed_region.Height() > 0;
  if (res) {
    Camera proposed_cam;
    proposed_cam.SetWorldWindow(proposed_region);
    res = proposed_cam.WithinPrecisionBounds();
  }
  return res;
}
}  // namespace

// static
glm::mat4 ElementManipulationTool::BestTransformForRegions(
    input::DragData drag, ElementManipulationToolHandle handle, glm::vec2 to,
    Rect smallest, Rect largest, RotRect region, bool allow_rotation,
    bool maintain_aspect_ratio) {
  ASSERT(smallest.Area() <= largest.Area());
  ASSERT(smallest.IsValid() && largest.IsValid());

  glm::mat4 translation = glm::mat4(1);
  glm::mat4 scale_and_rotate = glm::mat4(1);
  if (handle == ElementManipulationToolHandle::NONE) {
    translation = glm::translate(
        glm::mat4(1), glm::vec3(drag.world_drag.x, drag.world_drag.y, 0.0f));
    const auto scale_center_translation = glm::vec3(drag.world_scale_center, 1);
    scale_and_rotate = glm::translate(glm::mat4(1), scale_center_translation);
    if (allow_rotation) {
      scale_and_rotate = glm::rotate(scale_and_rotate, drag.rotation_radians,
                                     glm::vec3(0, 0, 1));
    }
    scale_and_rotate =
        glm::scale(scale_and_rotate, glm::vec3(drag.scale, drag.scale, 1.0f));
    scale_and_rotate =
        glm::translate(scale_and_rotate, -scale_center_translation);
  } else if (handle == ElementManipulationToolHandle::ROTATION) {
    if (allow_rotation) {
      RotRect new_region = region;
      new_region.SetRotation(glm::orientedAngle(
          glm::vec2{0, 1}, glm::normalize(to - region.Center())));
      scale_and_rotate = region.CalcTransformTo(new_region);
    }
  } else {
    glm::vec2 opposite_point =
        ElementManipulationToolHandleAnchor(handle, region);
    RotRect new_region = region;
    switch (handle) {
      case ElementManipulationToolHandle::RIGHTTOP:
      case ElementManipulationToolHandle::LEFTTOP:
      case ElementManipulationToolHandle::LEFTBOTTOM:
      case ElementManipulationToolHandle::RIGHTBOTTOM: {
        new_region =
            RotRect::WithCorners(opposite_point, to, region.Rotation());
        if (maintain_aspect_ratio) {
          new_region =
              new_region.InteriorRotRectWithAspectRatio(region.AspectRatio());
        }
      } break;
      case ElementManipulationToolHandle::TOP:
      case ElementManipulationToolHandle::BOTTOM: {
        new_region.SetHeight(std::abs(
            glm::dot(to - opposite_point,
                     glm::rotate(glm::vec2{0, 1}, region.Rotation()))));
        if (maintain_aspect_ratio) {
          new_region.SetWidth(new_region.Height() * region.AspectRatio());
        }
      } break;
      case ElementManipulationToolHandle::RIGHT:
      case ElementManipulationToolHandle::LEFT: {
        new_region.SetWidth(std::abs(
            glm::dot(to - opposite_point,
                     glm::rotate(glm::vec2{1, 0}, region.Rotation()))));
        if (maintain_aspect_ratio) {
          new_region.SetHeight(new_region.Width() / region.AspectRatio());
        }
      } break;
      default:
        break;
    }
    glm::vec2 new_opposite_point =
        ElementManipulationToolHandleAnchor(handle, new_region);
    new_region.Translate(opposite_point - new_opposite_point);
    scale_and_rotate = region.CalcTransformTo(new_region);
  }

  // always apply the translation, otherwise we feel unresponsive
  glm::mat4 result = translation;
  smallest = geometry::Transform(smallest, translation);
  largest = geometry::Transform(largest, translation);

  // check to see if scale causes precision issues on the largest element
  bool fp_prec_ok_for_largest =
      IsTransformSafeForRegion(scale_and_rotate, largest);
  if (!fp_prec_ok_for_largest) {
    SLOG(SLOG_TOOLS,
         "scale violates largest precision. largest: $0, "
         "scale: \n$1",
         largest, scale_and_rotate);
  }

  // check to see if scale causes precision issues on the smallest element
  bool fp_prec_ok_for_smallest =
      IsTransformSafeForRegion(scale_and_rotate, smallest);
  if (!fp_prec_ok_for_smallest) {
    SLOG(SLOG_TOOLS,
         "scale violates smallest precision. smallest: $0, "
         "scale: \n$1",
         smallest, scale_and_rotate);
  }

  // Is the transform completely safe?
  bool allow_scale = fp_prec_ok_for_smallest && fp_prec_ok_for_largest;

  // If we are in a bad state, try and allow the scale that will fix it
  if (!allow_scale) {
    bool allow_larger = largest.Area() < 1 || fp_prec_ok_for_largest;
    bool allow_smaller = smallest.Area() > 1 || fp_prec_ok_for_smallest;
    ASSERT(!allow_smaller || !allow_larger);

    allow_scale =
        (drag.scale < 1 && allow_smaller) || (drag.scale > 1 && allow_larger);

    SLOG(SLOG_TOOLS,
         "manipulation tooling fp precision exceeded. final allowScale: $0, "
         "allowSmaller: $1, allowLarger: $2, scale factor: $3",
         allow_scale, allow_smaller, allow_larger, drag.scale);
  }

  if (allow_scale) {
    result = scale_and_rotate * result;
  }
  return result;
}

void ElementManipulationTool::AttemptToTransformRegion(
    input::DragData drag, glm::vec2 to, bool maintain_aspect_ratio,
    bool multitouch) {
  auto handle =
      multitouch ? ElementManipulationToolHandle::NONE : manipulating_handle_;
  auto transform = BestTransformForRegions(
      drag, handle, to,
      geometry::Transform(smallest_element_mbr_, GetTransform()),
      geometry::Transform(element_mbr_, GetTransform()),
      geometry::Transform(selected_region_, GetTransform()),
      flags_->GetFlag(settings::Flag::EnableRotation), maintain_aspect_ratio);
  current_transform_ = transform * current_transform_;
}

void ElementManipulationTool::Draw(const Camera& cam,
                                   FrameTimeS draw_time) const {
  if (Enabled()) renderer_->Draw(cam, draw_time, GetTransform());
}

void ElementManipulationTool::Update(const Camera& cam, FrameTimeS draw_time) {
  if (Enabled()) {
    renderer_->Update(cam, draw_time, element_mbr_, selected_region_,
                      GetTransform());
  }
}

void ElementManipulationTool::Commit() {
  SLOG(SLOG_TOOLS, "Committing move.");
  auto transform_to_apply = GetTransform();
  for (auto& t : element_transforms_) {
    t = transform_to_apply * t;
  }
  scene_graph_->TransformElements(
      elements_.begin(), elements_.end(), element_transforms_.begin(),
      element_transforms_.end(), SourceDetails::FromEngine());
  if (Enabled()) {
    RotRect new_selected_region =
        geometry::Transform(selected_region_, transform_to_apply);
    SetElements(start_camera_, elements_);
    selected_region_ = new_selected_region;
  }
}

void ElementManipulationTool::Cancel() {
  SLOG(SLOG_TOOLS, "Cancelling element manipulation.");
  Reset();
  if (cancel_callback_) cancel_callback_();
}

void ElementManipulationTool::Reset() {
  tap_reco_.Reset();
  drag_reco_.Reset();
  is_manipulating_ = false;
  manipulating_handle_ = ElementManipulationToolHandle::NONE;
  scene_graph_->SetElementRenderedByMain(elements_.begin(), elements_.end(),
                                         true);
  if (Enabled() && !elements_.empty()) renderer_->Synchronize();
  elements_.clear();
  current_transform_ = glm::mat4(1);
  start_camera_ = Camera();
}

void ElementManipulationTool::Enable(bool enabled) {
  Tool::Enable(enabled);
  renderer_->Enable(enabled);
  // Apply the transform, if dirty.
  if (!enabled && !elements_.empty()) Commit();
  Reset();
}

void ElementManipulationTool::SetElements(
    const Camera& cam, const std::vector<ElementId>& elements) {
  ASSERT(Enabled());
  elements_ = elements;

  // fetch current transforms
  element_transforms_.resize(elements.size());
  for (size_t i = 0; i < elements.size(); i++) {
    // Store the group transform, as that's what we want to send
    // to the SetTransforms() call on the scene graph later.
    element_transforms_[i] =
        scene_graph_->GetElementMetadata(elements[i]).group_transform;
  }

  element_mbr_ = scene_graph_->Mbr(elements);
  selected_region_ = RotRect(element_mbr_);

  // calculate the smallest rectangle we are trying to manipulate. Knowing it
  // along with the largest Rect (elementMbr_) will let us prevent transforms
  // that would cause fp precesion issues
  smallest_element_mbr_ = Rect();
  float smallest_area = std::numeric_limits<float>::max();
  std::vector<ElementId> element_test_vec(1);
  for (auto& element : elements) {
    element_test_vec[0] = element;
    Rect embr = scene_graph_->Mbr(element_test_vec);
    if (embr.Area() < smallest_area) {
      smallest_area = embr.Area();
      smallest_element_mbr_ = embr;
    }
  }

  Rect visible_mbr;
  if (!geometry::Intersection(element_mbr_, cam.WorldWindow(), &visible_mbr) ||
      visible_mbr.Area() == 0) {
    SLOG(SLOG_TOOLS,
         "All selected elements have been moved offscreen. Deselecting.");
    Cancel();
    return;
  }

  current_transform_ = glm::mat4(1);
  start_camera_ = cam;

  if (!elements_.empty()) {
    scene_graph_->SetElementRenderedByMain(elements_.begin(), elements_.end(),
                                           false);
  }

  renderer_->SetElements(cam, elements_, visible_mbr, selected_region_);
}

const std::vector<ElementId>& ElementManipulationTool::GetElements() const {
  return elements_;
}

void ElementManipulationTool::OnElementAdded(SceneGraph* graph, ElementId id) {
  // noop
}

// If an element we were moving is removed by some other user, we should stop
// tracking it.
void ElementManipulationTool::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  if (elements_.empty()) return;

  // A lambda that returns true if the element is not removed.
  auto is_not_removed = [&removed_elements](ElementId id) {
    return std::none_of(
        removed_elements.begin(), removed_elements.end(),
        [&id](SceneGraphRemoval removal) { return removal.id == id; });
  };
  std::vector<ElementId> filtered_elements;
  std::copy_if(elements_.begin(), elements_.end(),
               std::back_inserter(filtered_elements), is_not_removed);

  if (filtered_elements.size() != elements_.size()) {
    SLOG(SLOG_TOOLS, "Element removed from manipulation tool.");
    if (filtered_elements.empty()) {
      Cancel();
    } else {
      SetElements(start_camera_, filtered_elements);
    }
  }
}

// If an element we were moving is moved by some other user, we should update
// our knowledge of it.
void ElementManipulationTool::OnElementsMutated(
    SceneGraph* graph, const std::vector<ElementMutationData>& mutation_data) {
  if (elements_.empty()) return;
  EXPECT(elements_.size() == element_transforms_.size());
  bool selection_changed = false;
  for (const ElementMutationData& data : mutation_data) {
    for (size_t i = 0; i < elements_.size(); i++) {
      if (elements_[i] == data.modified_element_data.id &&
          element_transforms_[i] !=
              data.modified_element_data.group_transform) {
        selection_changed = true;
        break;
      }
    }
  }
  if (selection_changed) {
    SetElements(start_camera_, elements_);
  }
}

}  // namespace tools
}  // namespace ink
