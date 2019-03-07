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

#include "ink/engine/rendering/compositing/triple_buffered_renderer.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/rendering/renderers/element_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/scene/types/element_index.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

TripleBufferedRenderer::TripleBufferedRenderer(
    std::shared_ptr<FrameState> frame_state,
    std::shared_ptr<GLResourceManager> gl_resources,
    std::shared_ptr<input::InputDispatch> input_dispatch,
    std::shared_ptr<SceneGraph> scene_graph,
    std::shared_ptr<WallClockInterface> wall_clock,
    std::shared_ptr<PageManager> page_manager,
    std::shared_ptr<LayerManager> layer_manager,
    std::shared_ptr<settings::Flags> flags)
    : back_region_query_(Rect(0, 0, 0, 0)),
      has_drawn_(false),
      valid_(false),
      front_is_valid_(false),  // does the front buffer have valid data
      gl_resources_(gl_resources),
      element_renderer_(gl_resources),
      frame_state_(std::move(frame_state)),
      input_dispatch_(std::move(input_dispatch)),
      scene_graph_(std::move(scene_graph)),
      wall_clock_(std::move(wall_clock)),
      page_manager_(std::move(page_manager)),
      layer_manager_(std::move(layer_manager)),
      flags_(std::move(flags)),
      tile_(absl::make_unique<DBRenderTarget>(wall_clock_, gl_resources)),
      above_tile_(absl::make_unique<DBRenderTarget>(wall_clock_, gl_resources)),
      cached_enable_motion_blur_flag_(
          flags_->GetFlag(settings::Flag::EnableMotionBlur)),
      current_back_draw_timer_(wall_clock_, false) {
  scene_graph_->AddListener(this);
  gl_resources_->texture_manager->AddListener(this);
  flags_->AddListener(this);
}

TripleBufferedRenderer::~TripleBufferedRenderer() {
  flags_->RemoveListener(this);
  scene_graph_->RemoveListener(this);
  gl_resources_->texture_manager->RemoveListener(this);
}

void TripleBufferedRenderer::Draw(const Camera& cam,
                                  FrameTimeS draw_time) const {
  SLOG(SLOG_DRAWING, "triple buffer draw request blitting to window: $0",
       cam.WorldWindow());

  // If the front buffer doesn't have anything in it, we don't need to blit it.
  if (front_buffer_bounds_) {
#ifdef INK_HAS_BLUR_EFFECT
    if (cached_enable_motion_blur_flag_) {
      if (last_frame_camera_ != cam) {
        // If we are blurring, acquire a framerate lock since to guarantee that
        // we draw at least one more frame after this one before dropping to 0
        // fps.
        blur_lock_ = frame_state_->AcquireFramerateLock(30, "blurring");
        blit_attrs::BlitMotionBlur attrs(cam.WorldWindow().CalcTransformTo(
            last_frame_camera_.WorldWindow()));
        tile_->DrawFront(cam, attrs, RotRect(tile_->Bounds()),
                         *front_buffer_bounds_);
      } else {
        if (blur_lock_) blur_lock_.reset();
        tile_->DrawFront(cam, blit_attrs::Blit(), RotRect(tile_->Bounds()),
                         *front_buffer_bounds_);
      }
    } else {
      tile_->DrawFront(cam, blit_attrs::Blit(), RotRect(tile_->Bounds()),
                       *front_buffer_bounds_);
    }
#else
    // No blur effect on web platform.
    tile_->DrawFront(cam, blit_attrs::Blit(), RotRect(tile_->Bounds()),
                     *front_buffer_bounds_);
#endif  // INK_HAS_BLUR_EFFECT
  }

  auto sorted_new_elements = scene_graph_->GroupifyElements(new_elements_);
  for (const auto& group : sorted_new_elements) {
    std::unique_ptr<Scissor> scissor;
    if (group.bounds.Area() != 0) {
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(cam, group.bounds, CoordType::kWorld);
    }
    for (const ElementId& id : group.poly_ids) {
      element_renderer_.Draw(id, *scene_graph_, cam, back_time_);
    }
  }

  for (const auto& d : scene_graph_->GetDrawables()) {
    d->Draw(cam, draw_time);
  }
  last_frame_camera_ = cam;
  has_drawn_ = true;
}

void TripleBufferedRenderer::DrawAfterTool(const Camera& cam,
                                           FrameTimeS draw_time) const {
  if (front_buffer_bounds_ && !layer_manager_->IsActiveLayerTopmost()) {
    above_tile_->DrawFront(cam, blit_attrs::Blit(),
                           RotRect(above_tile_->Bounds()),
                           *front_buffer_bounds_);
  }
}

void TripleBufferedRenderer::Update(const Timer& timer, const Camera& cam,
                                    FrameTimeS draw_time) {
  SLOG(SLOG_DRAWING, "triple buffer renderer updating");
  if (!has_drawn_) {
    // If we haven't drawn, last_frame_camera_ may not yet be correct. Don't
    // update buffers & predicted camera until after the first draw.
    return;
  }

  bool single_frame_update_expected =
      avg_back_draw_time_.Value() < timer.TimeRemaining();

  Camera predicted_cam;
  if (single_frame_update_expected || page_manager_->MultiPageEnabled()) {
    // If a single frame update is likely, or if the scene is very large and the
    // predictor is liable to zoom out spuriously, don't bother with the camera
    // predictor.
    predicted_cam = cam;
  } else {
    cam_predictor_.Update(cam, last_frame_camera_);
    predicted_cam = cam_predictor_.Predict(cam);
  }

  static constexpr int kFramesBetweenBufferUpdates = 5;
  if (!front_is_valid_ || single_frame_update_expected ||
      input_dispatch_->GetNContacts() == 0 ||
      frame_state_->GetFrameNumber() % kFramesBetweenBufferUpdates == 0) {
    UpdateBuffers(timer, predicted_cam, draw_time);

    // Hold a framelock if the this update draw was with a different camera.
    if (predicted_cam != cam) {
      frame_lock_ = frame_state_->AcquireFramerateLock(
          30, "TBR update draw with different camera");
    }
  }
}

void TripleBufferedRenderer::UpdateBuffers(const Timer& timer,
                                           const Camera& cam,
                                           FrameTimeS draw_time) {
  // watch for view change, but only restart:
  //   if we've finished drawing the old buffer
  //   or we're flagged to always restart on view change
  //   or we haven't progressed very far in drawing the new update
  bool back_eq_current_wnd =
      back_camera_ && cam.WorldWindow() == back_camera_->WorldWindow();
  if (!back_eq_current_wnd) {
    float coverage = 0;
    Rect intersection;
    if (back_camera_ &&
        geometry::Intersection(back_camera_->WorldWindow(), cam.WorldWindow(),
                               &intersection))
      coverage = intersection.Area() / back_camera_->WorldWindow().Area();
    auto tp = util::Lerp(0.2f, 0.6f, util::Normalize(0.8f, 0.1f, coverage));
    if (IsBackBufferComplete() ||
        current_back_draw_timer_.Elapsed() < tp * avg_back_draw_time_.Value()) {
      valid_ = false;
      frame_lock_ =
          frame_state_->AcquireFramerateLock(30, "TBR !backEqCurrentWnd");
    }
  }

  current_back_draw_timer_.Resume();
  bool changed = DrawToBack(timer, cam, draw_time);
  current_back_draw_timer_.Pause();
  ASSERT(back_camera_.has_value());

  if ((changed || !front_is_valid_) && IsBackBufferComplete()) {
    SLOG(SLOG_DRAWING, "tiled renderer completed back, resolving...");
    tile_->BlitBackToFront();
    front_buffer_bounds_ = back_camera_->WorldRotRect();
    if (!layer_manager_->IsActiveLayerTopmost()) {
      above_tile_->BlitBackToFront();
    }
    front_is_valid_ = true;
    if (cam.WorldWindow() == back_camera_->WorldWindow()) {
      frame_lock_.reset();
    }

    avg_back_draw_time_.Sample(current_back_draw_timer_.Elapsed());
    current_back_draw_timer_.Reset();
  }
}

bool TripleBufferedRenderer::DrawToBack(const Timer& timer, const Camera& cam,
                                        FrameTimeS draw_time) {
  SLOG(SLOG_DRAWING, "tiled renderer drawing to back");
  bool changed = false;
  if (!valid_) {
    InitBackBuffer(cam, draw_time);
    // If we are dirty and have no elements, the blank canvas is treated as
    // changed before any other drawing. This is needed to have correct
    // behavior when the last Element is removed from the back buffer.
    changed = backbuffer_elements_.empty();
  }
  changed |= RenderOutstandingBackBufferElements(timer, cam, draw_time);
  // We finished the backbuffer, special case fast path for element adds.
  // (Add them directly to the top of the composited result)
  if (AllElementsInBackBufferDrawn()) {
    changed |= RenderNewElementsToBackBuffer(cam);
  }
  return changed;
}

void TripleBufferedRenderer::InitBackBuffer(const Camera& cam,
                                            FrameTimeS draw_time) {
  SLOG(SLOG_DRAWING, "tiled renderer clearing back buffer");
  back_camera_ = cam;
  back_region_query_ = RegionQuery::MakeCameraQuery(*back_camera_);
  back_time_ = draw_time;
  tile_->ClearBack();
  above_tile_->ClearBack();
  current_back_draw_timer_.Reset();

  backbuffer_elements_ =
      scene_graph_->ElementsInRegionByGroup(back_region_query_);
  backbuffer_set_.clear();
  next_id_to_render_ = kInvalidElementId;
  backbuffer_id_to_zindex_ = scene_graph_->CopyZIndex();

  group_ordering_.clear();
  for (const auto& group : backbuffer_elements_) {
    group_ordering_[group.group_id] = group_ordering_.size();
    top_id_per_group_[group.group_id] = kInvalidElementId;
    if (!group.poly_ids.empty()) {
      top_id_per_group_[group.group_id] = group.poly_ids.back();
    }
    std::copy(group.poly_ids.begin(), group.poly_ids.end(),
              std::inserter(backbuffer_set_, backbuffer_set_.end()));
  }

  current_group_index_ = 0;
  current_element_index_ = 0;
  valid_ = true;
}

bool TripleBufferedRenderer::RenderOutstandingBackBufferElements(
    const Timer& timer, const Camera& cam, FrameTimeS draw_time) {
  bool drew_anything = false;
  float itercount = 0;
  const float kBatchSize = 4;

  if (current_group_index_ < backbuffer_elements_.size()) {
    BindTileForGroup(backbuffer_elements_[current_group_index_].group_id);
  }

  while (current_group_index_ < backbuffer_elements_.size()) {
    std::unique_ptr<Scissor> scissor;
    const auto& elements = backbuffer_elements_[current_group_index_];
    if (elements.bounds.Area() != 0) {
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(cam, elements.bounds, CoordType::kWorld);
    }
    while (current_element_index_ < elements.poly_ids.size()) {
      const ElementId& element = elements.poly_ids[current_element_index_];
      if (element_renderer_.Draw(element, *scene_graph_, *back_camera_,
                                 back_time_)) {
        drew_anything = true;
        float cvg = scene_graph_->Coverage(*back_camera_, element);
        itercount += util::Lerp(0.25f, 1.0f, util::Normalize(0.0f, 0.4f, cvg));
      }
      ++current_element_index_;
      if (itercount > kBatchSize && timer.Expired()) {
        break;
      }
    }
    if (current_element_index_ == elements.poly_ids.size()) {
      current_element_index_ = 0;
      ++current_group_index_;
      if (current_group_index_ < backbuffer_elements_.size()) {
        BindTileForGroup(backbuffer_elements_[current_group_index_].group_id);
      }
    }
    if (itercount > kBatchSize && timer.Expired()) {
      break;
    }
  }
  // Note that current_group_index_ and current_element_index_ is pointing
  // to the next thing to be processed at this point. Time to bail!
  if (AllElementsInBackBufferDrawn()) {
    next_id_to_render_ = kInvalidElementId;
  } else {
    next_id_to_render_ = backbuffer_elements_[current_group_index_]
                             .poly_ids[current_element_index_];
  }
  return drew_anything;
}

ElementId TripleBufferedRenderer::GetTopBackBufferElementForGroup(
    const GroupId& group_id) const {
  auto it = top_id_per_group_.find(group_id);
  if (it == top_id_per_group_.end()) {
    return kInvalidElementId;
  }
  return it->second;
}

bool TripleBufferedRenderer::RenderNewElementsToBackBuffer(const Camera& cam) {
  bool drew_anything = false;
  // If we get an add and then an invalidate an element could end up in
  // backbuffer_elements_ and new_elements_ as we requery backbuffer_elements_
  // on invalidation.
  //
  // We can't simply clear new_elements_ on invalidate (we have to wait
  // until blit backToFront), as doing so could cause elements to be
  // dropped and not drawn until the recomposite is complete
  const auto& new_elements_to_draw = new_elements_filter_.Filter(
      new_elements_.size(), new_elements_.begin(), new_elements_.end(),
      backbuffer_set_.begin(), backbuffer_set_.end(),
      scene_element::LessByHandle);
  auto sorted_new_elements =
      scene_graph_->GroupifyElements(new_elements_to_draw);
  bool was_empty = group_ordering_.empty();
  for (const auto& group : sorted_new_elements) {
    if (was_empty) {
      // If the backbuffer was previously empty, add the new groups.
      group_ordering_[group.group_id] = group_ordering_.size();
    }
    std::unique_ptr<Scissor> scissor;
    if (group.bounds.Area() != 0) {
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(cam, group.bounds, CoordType::kWorld);
    }
    BindTileForGroup(group.group_id);
    uint32_t top_backbuffer_zindex = 0;
    auto& id_to_zindex = backbuffer_id_to_zindex_[group.group_id];
    auto top_element = GetTopBackBufferElementForGroup(group.group_id);
    if (!id_to_zindex.empty() && top_element != kInvalidElementId) {
      top_backbuffer_zindex = id_to_zindex[top_element];
    }
    for (const ElementId& id : group.poly_ids) {
      if (element_renderer_.Draw(id, *scene_graph_, cam, back_time_)) {
        SLOG(SLOG_DRAWING,
             "transferring id $0 from newelements to the backbuffer", id);
        drew_anything = true;
        backbuffer_set_.insert(id);
        ++top_backbuffer_zindex;
        id_to_zindex[id] = top_backbuffer_zindex;
        top_id_per_group_[group.group_id] = id;
      }
    }
  }

  new_elements_.clear();
  return drew_anything;
}

bool TripleBufferedRenderer::IsBackBufferComplete() const {
  return new_elements_.empty() && AllElementsInBackBufferDrawn();
}

bool TripleBufferedRenderer::AllElementsInBackBufferDrawn() const {
  return current_group_index_ == backbuffer_elements_.size();
}

void TripleBufferedRenderer::OnElementAdded(SceneGraph* graph, ElementId id) {
  if (id.Type() == GROUP) {
    Invalidate();
    return;
  }
  if (graph->IsElementInRegion(id, back_region_query_)) {
    SLOG(SLOG_DATA_FLOW, "tbr adding $0, id $1 ", id.Type(), id.Handle());
    new_elements_.insert(id);
    frame_lock_ = frame_state_->AcquireFramerateLock(30, "TBR onElementAdded");
    if (NeedToInvalidateToAddElement(id)) {
      Invalidate();
    }
  } else {
    SLOG(SLOG_DATA_FLOW,
         "tbr saw addition of $0, but the visibility test didn't pass. "
         "Ignoring.",
         id);
  }
}

bool TripleBufferedRenderer::NeedToInvalidateToAddElement(
    const ElementId& id) const {
  // If there was nothing already drawn, we can simply add the element.
  if (group_ordering_.empty()) return false;
  // If the backbuffer is already going to be redrawn, don't add the element
  // yet.
  if (!valid_) return true;
  // If the thing I just added is < z index of the last thing I want to
  // render, then we should invalidate.
  auto parent = scene_graph_->GetParentGroupId(id);
  // If I don't know about this parent in the backbuffer. We don't know the
  // relative ordering of this group relative to the groups already known, so
  // invalidate the backbuffer.
  auto group_iter = group_ordering_.find(parent);
  if (group_iter == group_ordering_.end()) {
    return true;
  }
  // If this isn't in the last group being rendered, then we need
  // to invalidate.
  if (group_iter->second != group_ordering_.size() - 1) {
    return true;
  }

  // OK, this is rendering the last group. We better have a top element for
  // that group.
  auto last_composite_id = GetTopBackBufferElementForGroup(parent);
  ASSERT(last_composite_id != kInvalidElementId);

  const auto& scene_element_index = scene_graph_->GetElementIndex();
  auto group_index_iter = scene_element_index.find(parent);
  // We got the parent from the scene... the index should exist.
  ASSERT(group_index_iter != scene_element_index.end());

  // lastCompositeId isn't necessarily in sceneGraph_->getElementIndex().
  // However, we'll already be invalid if it isn't (due to the remove)
  auto last_composite_z = group_index_iter->second->ZIndexOf(last_composite_id);
  auto z_id_to_add = group_index_iter->second->ZIndexOf(id);
  bool res = z_id_to_add < last_composite_z;
  SLOG(SLOG_DATA_FLOW,
       "tbr saw addition of $0, invalidating is: $1. (zIdToAdd: $2, "
       "lastCompositeZ: $3)",
       id, res, z_id_to_add, last_composite_z);
  return res;
}

bool TripleBufferedRenderer::NeedToInvalidateToMutateElement(
    const ElementId& id) const {
  // If we've completed compositing, we need to invalidate
  if (AllElementsInBackBufferDrawn()) {
    return true;
  }

  // We only need to invalidate if we've already rendered this id.
  auto parent = scene_graph_->GetParentGroupId(id);
  auto next_parent = scene_graph_->GetParentGroupId(next_id_to_render_);

  if (group_ordering_.find(parent) == group_ordering_.end()) {
    // Don't know about this group.
    return false;
  }

  if (parent != next_parent) {
    auto group_index = group_ordering_.find(parent)->second;
    if (group_index > current_group_index_) {
      // If compositing progress hasn't reached this id's group, we don't need
      // to recomposite.
      return false;
    }
    // We definitely need to invalidate. We're past rendering this.
    return true;
  }

  // OK, the parents are the same. If we can't find this group, then we
  // have nothing to do. This shouldn't happen because of the earlier check.
  auto zindex_iter = backbuffer_id_to_zindex_.find(parent);
  if (zindex_iter == backbuffer_id_to_zindex_.end()) {
    return false;
  }
  const auto& id_to_zindex = zindex_iter->second;
  auto itr_zto_modify = id_to_zindex.find(id);
  // If we're not compositing the element, we don't need to invalidate
  if (itr_zto_modify == id_to_zindex.end()) {
    return false;
  }

  // z index of the item currently being composited
  auto itr_ztop = id_to_zindex.find(next_id_to_render_);
  ASSERT(itr_ztop != id_to_zindex.end());

  // If compositing progress hasn't reached this zIndex, we don't need to
  // recomposite, else, we do
  return itr_zto_modify->second < itr_ztop->second;
}

void TripleBufferedRenderer::OnElementRemoved(ElementId removed_id) {
  SLOG(SLOG_DATA_FLOW, "tbr removing element id $0", removed_id);
  if (removed_id.Type() == GROUP) {
    Invalidate();
    return;
  }
  auto fid = new_elements_.find(removed_id);
  if (fid != new_elements_.end()) {
    new_elements_.erase(fid);
  } else {
    Invalidate();
  }
  frame_lock_ = frame_state_->AcquireFramerateLock(30, "TBR onElementRemoved");
}

void TripleBufferedRenderer::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  for (auto removed : removed_elements) {
    OnElementRemoved(removed.id);
  }
}

void TripleBufferedRenderer::OnElementsMutated(
    SceneGraph* graph, const std::vector<ElementMutationData>& mutation_data) {
  bool needs_recomposite = false;
  for (const auto& data : mutation_data) {
    const auto& old_data = data.original_element_data;
    const auto& new_data = data.modified_element_data;

    if (old_data.id.Type() == GROUP) {
      needs_recomposite = true;
      continue;
    }

    bool was_visible =
        old_data.rendered_by_main &&
        (std::count(new_elements_.begin(), new_elements_.end(), old_data.id) ||
         std::count(backbuffer_set_.begin(), backbuffer_set_.end(),
                    old_data.id));
    bool is_visible = graph->IsElementInRegion(new_data.id, back_region_query_);
    if (was_visible && !is_visible) {  // went invisible
      SLOG(SLOG_DATA_FLOW,
           "tbr saw visibility mutation of $0. Treating as a remove",
           new_data.id);
      OnElementRemoved(new_data.id);
    } else if (!was_visible && is_visible) {  // became visible
      SLOG(SLOG_DATA_FLOW,
           "tbr saw visibility mutation of $0. Treating as a add", new_data.id);
      OnElementAdded(graph, new_data.id);
    }

    if (!needs_recomposite && is_visible &&
        (old_data.world_transform != new_data.world_transform ||
         old_data.color_modifier != new_data.color_modifier) &&
        NeedToInvalidateToMutateElement(new_data.id)) {
      needs_recomposite = true;
    }
  }

  if (needs_recomposite) {
    Invalidate();
  }
}

void TripleBufferedRenderer::Resize(glm::ivec2 size) {
  tile_->Resize(size);
  above_tile_->Resize(size);
  backbuffer_elements_.clear();
  backbuffer_set_.clear();
  next_id_to_render_ = kInvalidElementId;
  current_group_index_ = 0;
  current_element_index_ = 0;
  valid_ = false;
  front_is_valid_ = false;

  frame_lock_ = frame_state_->AcquireFramerateLock(30, "TBR resize");
}

void TripleBufferedRenderer::Invalidate() {
  frame_lock_ = frame_state_->AcquireFramerateLock(30, "TBR invalidate");
  valid_ = false;
}

void TripleBufferedRenderer::Synchronize(FrameTimeS draw_time) {
  // last_frame_camera_ only meaningful if we've already drawn.
  if (has_drawn_) {
    Camera cam = last_frame_camera_;
    cam.UnFlipWorldToDevice();  // UpdateBuffers expects an unflipped camera.
    Timer t(wall_clock_, 2);
    UpdateBuffers(t, cam, draw_time);
    if (!IsBackBufferComplete()) {
      ASSERT(t.Expired());
      SLOG(SLOG_WARNING, "giving up on tbr sync point after $0 seconds",
           static_cast<double>(t.TargetInterval()));
    }
    frame_state_->RequestFrame();
  }
}

glm::ivec2 TripleBufferedRenderer::RenderingSize() const {
  return tile_->GetSize();
}

void TripleBufferedRenderer::OnTextureLoaded(const TextureInfo& info) {
  Invalidate();
}

void TripleBufferedRenderer::OnTextureEvicted(const TextureInfo& info) {
  Invalidate();
}

void TripleBufferedRenderer::BindTileForGroup(const GroupId& group_id) const {
  if (!layer_manager_->IsActive() || group_id == kInvalidElementId) {
    tile_->BindBack();  // Rendering elements attached to root.
  } else {              // Check if group is a layer above the active layer.
    auto layer_z_index_or = layer_manager_->IndexForLayerWithGroupId(group_id);
    if (!layer_z_index_or.ok()) {
      SLOG(SLOG_ERROR, "No z-index for group $0, status $1, binding back tile.",
           group_id, layer_z_index_or.status());
      tile_->BindBack();
      return;
    }
    auto active_layer_z_index_or = layer_manager_->IndexOfActiveLayer();
    if (!active_layer_z_index_or.ok()) {
      SLOG(SLOG_ERROR,
           "No active layer index but layer manager was active, status $0, "
           "binding back tile.",
           active_layer_z_index_or.status());
      tile_->BindBack();
      return;
    }
    if (layer_z_index_or.ValueOrDie() > active_layer_z_index_or.ValueOrDie()) {
      above_tile_->BindBack();
    } else {
      tile_->BindBack();
    }
  }
}

void TripleBufferedRenderer::OnFlagChanged(settings::Flag which,
                                           bool new_value) {
  if (which == settings::Flag::EnableMotionBlur) {
    cached_enable_motion_blur_flag_ = new_value;
  }
}

}  // namespace ink
