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

#ifndef INK_ENGINE_RENDERING_COMPOSITING_TRIPLE_BUFFERED_RENDERER_H_
#define INK_ENGINE_RENDERING_COMPOSITING_TRIPLE_BUFFERED_RENDERER_H_

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/camera/camera_predictor.h"
#include "ink/engine/rendering/compositing/dbrender_target.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/element_renderer.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/funcs/cached_set_difference.h"
#include "ink/engine/util/signal_filters/exp_moving_avg.h"
#include "ink/engine/util/time/stopwatch.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"

// No blur effect on web platform. The web platform is especially susceptible
// to jank, and, when a frame does get stuck, it looks especially bad if it's
// blurry.
#if !(defined(__asmjs__) || defined(__wasm__))
#define INK_HAS_BLUR_EFFECT 1
#endif

namespace ink {

class TripleBufferedRenderer : public SceneGraphRenderer,
                               public SceneGraphListener,
                               public TextureListener,
                               public settings::FlagListener {
 public:
  using SharedDeps =
      service::Dependencies<FrameState, GLResourceManager, input::InputDispatch,
                            SceneGraph, WallClockInterface, PageManager,
                            LayerManager, settings::Flags>;

  TripleBufferedRenderer(std::shared_ptr<FrameState> frame_state,
                         std::shared_ptr<GLResourceManager> gl_resources,
                         std::shared_ptr<input::InputDispatch> input_dispatch,
                         std::shared_ptr<SceneGraph> scene_graph,
                         std::shared_ptr<WallClockInterface> wall_clock,
                         std::shared_ptr<PageManager> page_manager,
                         std::shared_ptr<LayerManager> layer_manager,
                         std::shared_ptr<settings::Flags> flags);
  ~TripleBufferedRenderer() override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;
  void DrawAfterTool(const Camera& cam, FrameTimeS draw_time) const override;

  void Update(const Timer& timer, const Camera& cam,
              FrameTimeS draw_time) override;

  void OnElementAdded(SceneGraph* graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override;

  void Resize(glm::ivec2 size) override;
  glm::ivec2 RenderingSize() const override;

  void Invalidate() override;

  void Synchronize(FrameTimeS draw_time) override;

  bool IsBackBufferComplete() const;

  // FlagListener
  void OnFlagChanged(settings::Flag which, bool new_value) override;

 private:
  // Texture updates will immediately Invalidate the scene.
  void OnTextureLoaded(const TextureInfo& info) override;
  void OnTextureEvicted(const TextureInfo& info) override;

  void OnElementRemoved(ElementId removed_id);

  void UpdateBuffers(const Timer& timer, const Camera& cam,
                     FrameTimeS draw_time);

  // Returns true if this call changed the state of buffers.
  bool DrawToBack(const Timer& timer, const Camera& cam, FrameTimeS draw_time);
  void InitBackBuffer(const Camera& cam, FrameTimeS draw_time);
  bool RenderOutstandingBackBufferElements(const Timer& timer,
                                           const Camera& cam,
                                           FrameTimeS draw_time);
  bool RenderNewElementsToBackBuffer(const Camera& cam);

  bool AllElementsInBackBufferDrawn() const;

  // If we're partially through updating the backbuffer we may not need to
  // restart in order to add or change an element.
  bool NeedToInvalidateToAddElement(const ElementId& id) const;
  bool NeedToInvalidateToMutateElement(const ElementId& id) const;

  ElementId GetTopBackBufferElementForGroup(const GroupId& group_id) const;

  // Bind the above or below tile for subsequent rendering.
  //
  // Chooses based on whether the group is above or below the active layer.  If
  // no layers are active, always binds the below tile.
  void BindTileForGroup(const GroupId& group_id) const;

  // newElements_ are elements that have not yet made it to the backbuffer.
  std::unordered_set<ElementId, ElementIdHasher> new_elements_;

  SceneGraph::GroupedElementsList backbuffer_elements_;
  int current_group_index_ = 0;
  int current_element_index_ = 0;
  ElementId next_id_to_render_;
  std::unordered_map<ElementId, uint32_t, ElementIdHasher> group_ordering_;

  // This maintains the set of poly ids the backbuffer knows about.
  std::unordered_set<ElementId, ElementIdHasher> backbuffer_set_;

  // We do a set difference of newElements_ and backbufferElements_ every
  // frame, this helps avoid some work
  CachedSetDifference<ElementId> new_elements_filter_;

  // backbuffer_id_to_zindex_ is a snapshot of the scenegraph's per-group
  // z-indexes.
  SceneGraph::IdToZIndexPerGroup backbuffer_id_to_zindex_;
  std::unordered_map<ElementId, ElementId, ElementIdHasher> top_id_per_group_;

  absl::optional<RotRect> front_buffer_bounds_;
  absl::optional<Camera> back_camera_;
  RegionQuery back_region_query_;
  FrameTimeS back_time_;

  // The camera at the end of the previous Draw() call.
  mutable Camera last_frame_camera_;
  mutable bool has_drawn_;

  bool valid_;
  bool front_is_valid_;

  std::shared_ptr<GLResourceManager> gl_resources_;
  ElementRenderer element_renderer_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<input::InputDispatch> input_dispatch_;
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<WallClockInterface> wall_clock_;
  // Do not apply prediction to multi-page documents.
  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<LayerManager> layer_manager_;
  std::shared_ptr<settings::Flags> flags_;
  std::unique_ptr<FramerateLock> frame_lock_;
  std::unique_ptr<DBRenderTarget> tile_;
  std::unique_ptr<DBRenderTarget> above_tile_;

#ifdef INK_HAS_BLUR_EFFECT
  // Maintain a separate framelock for blur effects.
  mutable std::unique_ptr<FramerateLock> blur_lock_;
#endif
  bool cached_enable_motion_blur_flag_;

  CameraPredictor cam_predictor_;

  signal_filters::ExpMovingAvg<DurationS, double> avg_back_draw_time_;
  Stopwatch current_back_draw_timer_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_COMPOSITING_TRIPLE_BUFFERED_RENDERER_H_
