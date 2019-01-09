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

#include "ink/engine/rendering/export/image_exporter.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/baseGL/render_target.h"
#include "ink/engine/rendering/compositing/partition_data.h"
#include "ink/engine/rendering/compositing/single_partition_renderer.h"
#include "ink/engine/rendering/renderers/background_renderer.h"
#include "ink/engine/scene/graph/region_query.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/public/fingerprint/fingerprint.h"

namespace ink {
namespace {
// Returns true iff the argument is the kDraw variant, in a type-safe-ish way.
template <typename T>
bool WantDraw(const T t) {
  return t == T::kDraw;
}
}  // namespace

glm::ivec2 ImageExporter::BestTextureSize(const Rect& world_rect,
                                          uint32_t max_dim_px) {
  uint32_t width_px;
  uint32_t height_px;

  float aspect_ratio = world_rect.AspectRatio();
  if (aspect_ratio > 1) {
    width_px = max_dim_px;
    height_px = width_px * (1.0f / aspect_ratio);
  } else {
    height_px = max_dim_px;
    width_px = height_px * aspect_ratio;
  }

  return glm::ivec2(width_px, height_px);
}

DefaultImageExporter::DefaultImageExporter(
    const service::Registry<DefaultImageExporter>& registry)
    : scene_graph_(registry.GetShared<SceneGraph>()),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      page_bounds_(registry.GetShared<PageBounds>()),
      wall_clock_(registry.GetShared<WallClockInterface>()),
      root_renderer_(registry.GetShared<RootRenderer>()),
      tools_(registry.GetShared<ToolController>()),
      frame_state_(registry.GetShared<FrameState>()) {}

// The specific order of draw operations in Render should be kept in sync with
// RootRenderer.
//
void DefaultImageExporter::Render(
    uint32_t max_dimension_px, const Rect& image_export_world_bounds,
    const ImageExporter::BackgroundOptions background_options,
    const ImageExporter::CurrentToolOptions current_tool_options,
    const ImageExporter::DrawablesOptions drawables_options,
    GroupId render_only_group, ExportedImage* out) {
  ASSERT(image_export_world_bounds.Width() > 0);
  ASSERT(image_export_world_bounds.Height() > 0);

  const FrameTimeS draw_time = frame_state_->GetFrameTime();

  out->size_px = BestTextureSizeWithinAvailableLimits(
      max_dimension_px, image_export_world_bounds);

  SLOG(SLOG_INFO, "Creating image: widthPx: $0, heightPx: $1, world bounds: $2",
       out->size_px.x, out->size_px.y, image_export_world_bounds);

  Camera export_cam;
  export_cam.SetScreenDim(out->size_px);
  export_cam.SetWorldWindow(image_export_world_bounds);
  SinglePartitionRenderer renderer(wall_clock_, gl_resources_);

  ink::Fingerprinter fingerprinter;

  std::vector<ElementId> all_elements;
  scene_graph_->ElementsInScene(std::back_inserter(all_elements));
  for (ElementId id : all_elements) {
    // The fingerprinter cares about whether the elements relative to their
    // groups are the same, not if (for example) the pages are re-layed out.
    if (id.Type() != GROUP) {
      ElementMetadata md = scene_graph_->GetElementMetadata(id);
      fingerprinter.Note(md.uuid, md.group_transform);
    }
  }
  out->fingerprint = fingerprinter.GetFingerprint();

  RegionQuery query = RegionQuery::MakeCameraQuery(export_cam)
                          .SetGroupFilter(render_only_group);
  auto elements_by_group = scene_graph_->ElementsInRegionByGroup(query);

  renderer.AssignPartitionData(PartitionData(1, elements_by_group));
  renderer.Resize(out->size_px);

  // Draw scene to target
  while (renderer.CacheState() != PartitionCacheState::Complete) {
    Timer t(wall_clock_, 1);
    renderer.Update(t, export_cam, draw_time, *scene_graph_);
  }

  // This is an extra copy that we don't really need.
  // We could expose a captureToBuffer function on SinglePartitionRenderer
  // that directly took from it's cached front buffer
  RenderTarget target(gl_resources_);
  export_cam.FlipWorldToDevice();
  target.Resize(out->size_px);
  target.Clear(glm::vec4(0));

  if (WantDraw(drawables_options)) {
    root_renderer_->DrawDrawables(draw_time, RootRenderer::RenderOrder::Start);
    root_renderer_->DrawDrawables(draw_time,
                                  RootRenderer::RenderOrder::PreBackground);
  }

  if (WantDraw(background_options)) {
    BackgroundRenderer bg_renderer(gl_resources_, page_bounds_);
    bg_renderer.Draw(export_cam, draw_time);
  }

  const Tool* tool =
      WantDraw(current_tool_options) ? tools_->EnabledTool() : nullptr;
  if (tool) {
    tool->BeforeSceneDrawn(export_cam, draw_time);
  }

  if (WantDraw(drawables_options)) {
    root_renderer_->DrawDrawables(draw_time,
                                  RootRenderer::RenderOrder::PreScene);
  }

  renderer.Draw(export_cam, draw_time, *scene_graph_, blit_attrs::Blit());

  if (WantDraw(drawables_options)) {
    root_renderer_->DrawDrawables(draw_time,
                                  RootRenderer::RenderOrder::PreTool);
  }

  if (tool) {
    tool->Draw(export_cam, draw_time);
  }
  if (WantDraw(drawables_options)) {
    root_renderer_->DrawDrawables(draw_time,
                                  RootRenderer::RenderOrder::PostTool);
  }
  if (tool) {
    tool->AfterSceneDrawn(export_cam, draw_time);
  }
  if (WantDraw(drawables_options)) {
    root_renderer_->DrawDrawables(draw_time, RootRenderer::RenderOrder::End);
  }

  // Read back pixels from target
  target.GetPixels(&out->bytes);
}

glm::ivec2 DefaultImageExporter::BestTextureSizeWithinAvailableLimits(
    uint32_t max_dimension_px, const Rect& world_rect) const {
  int max_texture_size = 0;
  gl_resources_->gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  EXPECT(max_texture_size > 0);

  // Cap the max texture size at 4k even if the device actually supports
  // something larger than that, because someone who is asking for a 16k image
  // export on a Nexus 9 is almost definitely making a mistake.
  max_texture_size = std::min(max_texture_size, 4096);

  if (static_cast<uint32_t>(max_texture_size) < max_dimension_px) {
    SLOG(SLOG_WARNING, "Capping requested size at max texture size");
    max_dimension_px = max_texture_size;
  }
  return BestTextureSize(world_rect, max_dimension_px);
}
}  // namespace ink
