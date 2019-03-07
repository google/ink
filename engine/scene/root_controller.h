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

#ifndef INK_ENGINE_SCENE_ROOT_CONTROLLER_H_
#define INK_ENGINE_SCENE_ROOT_CONTROLLER_H_

#include <sys/types.h>
#include <cstdint>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/input/cursor_manager.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/public/types/exported_image.h"
#include "ink/engine/public/types/iselection_provider.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/realtime/crop_mode.h"
#include "ink/engine/realtime/tool_controller.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/export/image_exporter.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/element_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/grid_manager.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/scene/types/text.h"
#include "ink/engine/scene/unsafe_scene_helper.h"
#include "ink/engine/scene/update_loop.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/signal_filters/exp_moving_avg.h"
#include "ink/engine/util/time/logging_perf_timer.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/export_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

class RootController : public settings::FlagListener {
 public:
  explicit RootController(
      std::shared_ptr<IPlatform> platform,
      std::unique_ptr<service::DefinitionList> service_definitions);
  ~RootController() override;

  void Reshape(glm::ivec2 new_size, float ppi, int rotation_deg);

  void Draw(FrameTimeS draw_time);

  ABSL_MUST_USE_RESULT Status
  SetToolParams(const proto::ToolParams& unsafe_proto);

  ABSL_MUST_USE_RESULT Status
  AddElement(const proto::ElementBundle& unsafe_bundle,
             const SourceDetails& source_details);
  ABSL_MUST_USE_RESULT Status AddElementBelow(
      const proto::ElementBundle& unsafe_bundle,
      const SourceDetails& source_details, const UUID& below_element_with_uuid);
  void AddStrokeOutline(const proto::StrokeOutline& unsafe_stroke_outline,
                        const GroupId& group,
                        const SourceDetails& source_details);
  void RemoveElement(const UUID& uuid, const SourceDetails& source_details);

  void ReplaceElements(const proto::ElementBundleReplace& replace,
                       const SourceDetails& source_details);

  UUID AddPath(const proto::Path& unsafe_path, const GroupId& group,
               const SourceDetails& source_details);
  UUID AddImageRect(const Rect& rectangle, float rotation,
                    const std::string& uri, const ElementAttributes& attributes,
                    GroupId group_id);
  // World coordinates of the added text rect are the given rect with the given
  // transform applied.
  UUID AddTextRect(const text::TextSpec& text, const Rect& rect, GroupId group,
                   UUID uuid, const glm::mat4& transform = glm::mat4(1),
                   const ElementId& below_element_with_id = kInvalidElementId);
  // Make the scene reflect the updated text data for the given element.
  void UpdateText(const UUID& uuid, const text::TextSpec& text,
                  const Rect& world_bounds);
  // Update text contents, set element size relative to current size.
  void UpdateText(const UUID& uuid, const text::TextSpec& text,
                  float width_multiplier, float height_multiplier);
  void SetTransforms(const std::vector<UUID>& ids,
                     const std::vector<glm::mat4>& new_transforms,
                     const SourceDetails& source_details);
  void SetVisibilities(const std::vector<UUID>& uuids,
                       const std::vector<bool>& visibilities,
                       const SourceDetails& source_details);
  void SetOpacities(const std::vector<UUID>& uuids,
                    const std::vector<int>& opacities,
                    const SourceDetails& source_details);
  void ChangeZOrders(const std::vector<UUID>& uuids,
                     const std::vector<UUID>& below_uuids,
                     const SourceDetails& source_details);
  void SetPageBorder(const std::string& border_uri, float scale);
  void ClearPageBorder();
  void SetOutOfBoundsColor(glm::vec4 out_of_bounds_color);
  void SetGrid(const proto::GridInfo& grid_info);
  void ClearGrid();

  // Set the color of the element with the given UUID to the given premultipled
  // color.
  // If it isn't meaningful to set "the color" for the element with the given
  // UUID, the result is undefined.
  void SetColor(const UUID& uuid, glm::vec4 rgba, SourceDetails source);

  // Creates a bitmap of the Scene contents matching the given world_rect. If
  // world_rect is empty, use the document bounds or the current screen view if
  // document bounds are not set. The bitmap has the larger of width or height
  // equal to "maxDimensionPx" and the other dimension matching the aspect ratio
  // of the page (or viewport if no page bounds exist). The bitmap may be scaled
  // down to be less than the maximum texture size of GPU, while preserving
  // aspect ratio.
  // If render_only_group != kInvalidElementId, then only render elements which
  // are descendents of that group.
  void Render(uint32_t max_dimension_px, bool should_draw_background,
              const Rect& world_rect, GroupId render_only_group,
              ExportedImage* out);

  void AddSequencePoint(int32_t id);

  // If an element with the given UUID exists, switches to the
  // ElementManipulationTool and selects that element.
  void SelectElement(const UUID& uuid);

  void DeselectAll();

  void OnFlagChanged(settings::Flag which, bool new_value) override;

  service::UncheckedRegistry* registry() { return registry_.get(); }

  template <typename T>
  std::shared_ptr<T> service() {
    return registry_->GetShared<T>();
  }

  // Set the selection provider (there can only be one).
  void SetSelectionProvider(std::shared_ptr<ISelectionProvider> provider);
  ISelectionProvider* SelectionProvider() const;

 private:
  void SetupTools();

 public:
  std::unique_ptr<UnsafeSceneHelper> unsafe_helper_;

 private:
  std::unique_ptr<service::UncheckedRegistry> registry_;

  glm::ivec2 size_{0, 0};

  std::unique_ptr<LoggingPerfTimer> draw_timer_;
  std::unique_ptr<LoggingPerfTimer> blit_timer_;
  signal_filters::ExpMovingAvg<double, double> blit_time_filter_;
  std::unique_ptr<FramerateLimiter> frame_limiter_;

  // The most recent tool params provided to the engine (may need to be
  // reinterpreted if flags change).
  ink::proto::ToolParams tool_params_;

  std::shared_ptr<ISelectionProvider> selection_provider_;

  // Pointers to registry.
  std::shared_ptr<input::InputDispatch> input_;
  std::shared_ptr<ToolController> tools_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<SceneGraphRenderer> graph_renderer_;
  std::shared_ptr<CameraController> camera_controller_;
  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<ITaskRunner> task_runner_;
  std::shared_ptr<RootRenderer> root_renderer_;
  std::shared_ptr<WallClockInterface> wall_clock_;
  std::shared_ptr<UpdateLoop> update_loop_;
  std::shared_ptr<GridManager> grid_manager_;
  std::shared_ptr<PageBorder> page_border_;
  std::shared_ptr<settings::Flags> flags_;
  std::shared_ptr<CropMode> crop_mode_;
  std::shared_ptr<ImageExporter> image_exporter_;
  std::shared_ptr<input::CursorManager> cursor_manager_;

  friend class SEngineTestHelper;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_ROOT_CONTROLLER_H_
