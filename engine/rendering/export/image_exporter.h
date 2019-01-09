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

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/exported_image.h"
#include "ink/engine/realtime/tool_controller.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/service/registry.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
// This service provides a means for rendering an Ink SceneGraph to a rasterized
// buffer (which you can export to your preferred image format for taking
// screenshots, recording videos, etc.).
class ImageExporter {
 public:
  // Returns the size in pixels of the largest rectangle such that:
  //   1. The aspect ratio matches "worldRect"
  //   2. No dimension exceeds "maxDimPix"
  static glm::ivec2 BestTextureSize(const Rect& world_rect,
                                    uint32_t max_dim_px);

  // Indicates whether the canvas background should be drawn.
  enum class BackgroundOptions {
    kSkip = 0,
    kDraw,
  };

  // Indicates whether the activity of the currently active tool (if there is
  // one) should be drawn.
  enum class CurrentToolOptions {
    kSkip = 0,
    kDraw,
  };

  // Indicates whether in-scene drawables (like buffered points from a tool that
  // is idling but hasn't pushed its updates to the scene graph yet) should be
  // drawn.
  enum class DrawablesOptions {
    kSkip = 0,
    kDraw,
  };

  virtual ~ImageExporter() {}

  // Creates an image of the current from world coords
  // "image_export_world_bounds", with image size in px is "max_dimension_px" on
  // the longer side of either width or height, with the aspect ratio matching
  // "image_export_world_bounds".
  //
  // If render_only_group is set to something other than kInvalidElementId, then
  // only elements in that group will be rendered.
  //
  // If either the width or height (in px) exceeds GL_MAX_TEXTURE_SIZE, the
  // output is scaled so no dimension is too large but the aspect ratio is
  // preserved (see "BestTextureSize()").
  virtual void Render(uint32_t max_dimension_px,
                      const Rect& image_export_world_bounds,
                      BackgroundOptions background_options,
                      CurrentToolOptions current_tool_options,
                      DrawablesOptions drawables_options,
                      GroupId render_only_group, ExportedImage* out) = 0;

  // Returns the dimensions, in pixels, of the rectangle that fits within a GL
  // texture that can be provided and whose aspect ratio matches that of
  // world_rect. These dimensions may be as large as allowed by world_rect and
  // max_dimension_px, or they may be smaller, if the resulting rectangle would
  // be so large that a texture for it is not available.
  //
  // Images exported with Render() will have their size determined by this
  // method. It may be used to precompute the size of images that Render() will
  // provide.
  virtual glm::ivec2 BestTextureSizeWithinAvailableLimits(
      uint32_t max_dimension_px, const Rect& world_rect) const = 0;
};

class DefaultImageExporter : public ImageExporter {
 public:
  using SharedDeps =
      service::Dependencies<SceneGraph, GLResourceManager, PageBounds,
                            WallClockInterface, RootRenderer, ToolController,
                            FrameState>;

  explicit DefaultImageExporter(
      const service::Registry<DefaultImageExporter>& registry);

  void Render(uint32_t max_dimension_px, const Rect& image_export_world_bounds,
              BackgroundOptions background_options,
              CurrentToolOptions current_tool_options,
              DrawablesOptions drawables_options, GroupId render_only_group,
              ExportedImage* out) override;

  glm::ivec2 BestTextureSizeWithinAvailableLimits(
      uint32_t max_dimension_px, const Rect& world_rect) const override;

 private:
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<WallClockInterface> wall_clock_;
  std::shared_ptr<RootRenderer> root_renderer_;
  std::shared_ptr<ToolController> tools_;
  std::shared_ptr<FrameState> frame_state_;
};
}  // namespace ink

#endif  // INK_ENGINE_RENDERING_EXPORT_IMAGE_EXPORTER_H_
