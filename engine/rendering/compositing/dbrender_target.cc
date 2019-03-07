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

#include "ink/engine/rendering/compositing/dbrender_target.h"

#include <atomic>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/gl.h"
#include "ink/engine/rendering/baseGL/msaa.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/logging_perf_timer.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

AntialiasingStrategy BackAAStrat(const GLResourceManager& gl_resources) {
  // Use MSAA for the backbuffer if it is available
  if (gl_resources.IsMSAASupported()) {
    return AntialiasingStrategy::kMSAA;
  }

  // If the MSAA extension is unavailable then just don't antialias: it is most
  // likely a low end mobile device with a small screen that is still relatively
  // high dpi compared to desktop, or an older underpowered laptop or desktop
  // device.
  return AntialiasingStrategy::kNone;
}

DBRenderTarget::DBRenderTarget(std::shared_ptr<WallClockInterface> wall_clock,
                               std::shared_ptr<GLResourceManager> gl_resources,
                               TextureMapping front_buffer_mapping)
    : front_(gl_resources, AntialiasingStrategy::kNone, front_buffer_mapping),
      back_(gl_resources, BackAAStrat(*gl_resources), TextureMapping::Nearest),
      fb_blit_timer_(wall_clock, std::string("db backToFront blit time")),
      front_blit_timer_(wall_clock, std::string("db front blit time")),
      front_combined_blit_timer_(wall_clock,
                                 std::string("db frontCombined blit time")) {}

void DBRenderTarget::Resize(glm::ivec2 size) {
  front_.Resize(size);
  back_.Resize(size);
}

void DBRenderTarget::DrawFront(const Camera& cam,
                               const blit_attrs::BlitAttrs& attrs) const {
  auto timer = const_cast<LoggingPerfTimer*>(&front_blit_timer_);
  timer->Begin();
  front_.Draw(cam, attrs);
  timer->End();
}

// Blits the front rendertarget
// See rendertarget::blit(cam, attrs, src, dst)
void DBRenderTarget::DrawFront(const Camera& cam,
                               const blit_attrs::BlitAttrs& attrs,
                               const RotRect& buffer_source,
                               const RotRect& world_dest) const {
  auto timer = const_cast<LoggingPerfTimer*>(&front_blit_timer_);
  timer->Begin();
  front_.Draw(cam, attrs, buffer_source, world_dest);
  timer->End();
}

void DBRenderTarget::BlitBackToFront(absl::optional<Rect> area) {
  fb_blit_timer_.Begin();
  back_.Blit(&front_, area);
  fb_blit_timer_.End();
}

}  // namespace ink
