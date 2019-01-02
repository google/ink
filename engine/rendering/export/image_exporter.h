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

#ifndef INK_ENGINE_RENDERING_EXPORT_IMAGE_EXPORTER_H_
#define INK_ENGINE_RENDERING_EXPORT_IMAGE_EXPORTER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/exported_image.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

class ImageExporter {
 public:
  // Creates a Image encoded image of the sceneGraph from world coords
  // "imageExportWorldBounds", with image size in px is "maxDimensionPx" on
  // the longer side of either width or height, with the aspect ratio matching
  // "imageExportWorldBounds".
  //
  // If render_only_group is set to something other than kInvalidElementId,
  // then only elements in that group will be rendered.
  //
  // If either the width or height (in px) exceeds GL_MAX_TEXTURE_SIZE, the
  // output is scaled so no dimension is too large but the aspect ratio is
  // preserved (see "bestTextureSize()").
  static void Render(uint32_t max_dimension_px,
                     const Rect& image_export_world_bounds,
                     FrameTimeS draw_time,
                     std::shared_ptr<GLResourceManager> gl_resources,
                     std::shared_ptr<PageBounds> page_bounds,
                     std::shared_ptr<WallClockInterface> wall_clock,
                     const SceneGraph& scene_graph, bool should_draw_background,
                     GroupId render_only_group, ExportedImage* out);

  // Returns the size in pixels of the largest rectangle such that:
  //   1. The aspect ratio matches "worldRect"
  //   2. No dimension exceeds "maxDimPix"
  static glm::ivec2 BestTextureSize(const Rect& world_rect,
                                    uint32_t max_dim_px);
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_EXPORT_IMAGE_EXPORTER_H_
