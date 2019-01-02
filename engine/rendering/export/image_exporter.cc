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

using std::vector;

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

void ImageExporter::Render(uint32_t max_dimension_px,
                           const Rect& image_export_world_bounds,
                           FrameTimeS draw_time,
                           std::shared_ptr<GLResourceManager> gl_resources,
                           std::shared_ptr<PageBounds> page_bounds,
                           std::shared_ptr<WallClockInterface> wall_clock,
                           const SceneGraph& scene_graph,
                           bool should_draw_background,
                           GroupId render_only_group, ExportedImage* out) {
  ASSERT(image_export_world_bounds.Width() > 0);
  ASSERT(image_export_world_bounds.Height() > 0);

  int max_texture_size = 0;
  gl_resources->gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  EXPECT(max_texture_size > 0);

  // Cap the max texture size at 4k even if the device actually supports
  // something larger than that, because someone who is asking for a 16k image
  // export on a Nexus 9 is almost definitely making a mistake.
  max_texture_size = std::min(max_texture_size, 4096);

  if (static_cast<uint32_t>(max_texture_size) < max_dimension_px) {
    SLOG(SLOG_WARNING, "Requested image size was larger than max texture size");
    max_dimension_px = max_texture_size;
  }

  out->size_px = BestTextureSize(image_export_world_bounds, max_dimension_px);

  SLOG(SLOG_INFO, "Creating image: widthPx: $0, heightPx: $1, world bounds: $2",
       out->size_px.x, out->size_px.y, image_export_world_bounds);

  Camera export_cam;
  export_cam.SetScreenDim(out->size_px);
  export_cam.SetWorldWindow(image_export_world_bounds);
  SinglePartitionRenderer renderer(wall_clock, gl_resources);

  ink::Fingerprinter fingerprinter;

  std::vector<ElementId> all_elements;
  scene_graph.ElementsInScene(std::back_inserter(all_elements));
  for (ElementId id : all_elements) {
    // The fingerprinter cares about whether the elements relative to their
    // groups are the same, not if (for example) the pages are re-layed out.
    if (id.Type() != GROUP) {
      ElementMetadata md = scene_graph.GetElementMetadata(id);
      fingerprinter.Note(md.uuid, md.group_transform);
    }
  }
  out->fingerprint = fingerprinter.GetFingerprint();

  RegionQuery query = RegionQuery::MakeCameraQuery(export_cam)
                          .SetGroupFilter(render_only_group);
  auto elements_by_group = scene_graph.ElementsInRegionByGroup(query);

  renderer.AssignPartitionData(PartitionData(1, elements_by_group));
  renderer.Resize(out->size_px);

  // Draw scene to target
  while (renderer.CacheState() != PartitionCacheState::Complete) {
    Timer t(wall_clock, 1);
    renderer.Update(t, export_cam, draw_time, scene_graph);
  }

  // This is an extra copy that we don't really need.
  // We could expose a captureToBuffer function on SinglePartitionRenderer
  // that directly took from it's cached front buffer
  RenderTarget target(gl_resources);
  export_cam.FlipWorldToDevice();
  target.Resize(out->size_px);
  target.Clear(glm::vec4(0));

  if (should_draw_background) {
    BackgroundRenderer bg_renderer(gl_resources, page_bounds);
    bg_renderer.Draw(export_cam, draw_time);
  }

  renderer.Draw(export_cam, draw_time, scene_graph, blit_attrs::Blit());

  // Read back pixels from target
  target.GetPixels(&out->bytes);
}

}  // namespace ink
