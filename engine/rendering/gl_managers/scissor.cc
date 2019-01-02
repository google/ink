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

#include "ink/engine/rendering/gl_managers/scissor.h"

#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/util/dbg/glerrors.h"

namespace ink {

// A CPU shadow for scissor state.
// This only works if everybody does their GL scissoring through this class.
// Without this shadow state, querying GL on every instantiation of this class
// adds a 30% CPU overhead on WebGL.
namespace {
std::array<GLint, 4> shadow_scissor_box;
bool shadow_scissor_enabled{false};
bool shadow_scissor_initialized{false};
}  // namespace

void SetupShadowScissorState(ion::gfx::GraphicsManagerPtr gl) {
  shadow_scissor_initialized = true;
  gl->GetIntegerv(GL_SCISSOR_BOX, shadow_scissor_box.data());
  shadow_scissor_enabled = gl->IsEnabled(GL_SCISSOR_TEST);
}

/* static */ Rect Scissor::GetScissorBox(ion::gfx::GraphicsManagerPtr gl) {
  if (!shadow_scissor_initialized) SetupShadowScissorState(gl);
  return Rect(shadow_scissor_box[0],                           // left
              shadow_scissor_box[1],                           // bottom
              shadow_scissor_box[0] + shadow_scissor_box[2],   // left + width
              shadow_scissor_box[1] + shadow_scissor_box[3]);  // top + height
}

/* static */ void Scissor::SetScissorBox(ion::gfx::GraphicsManagerPtr gl,
                                         Rect box) {
  if (!shadow_scissor_initialized) SetupShadowScissorState(gl);
  shadow_scissor_box[0] = box.Left();
  shadow_scissor_box[1] = box.Bottom();
  shadow_scissor_box[2] = box.Width();
  shadow_scissor_box[3] = box.Height();
  gl->Scissor(shadow_scissor_box[0], shadow_scissor_box[1],
              shadow_scissor_box[2], shadow_scissor_box[3]);
}

/* static */ bool Scissor::GetScissorEnabled(ion::gfx::GraphicsManagerPtr gl) {
  if (!shadow_scissor_initialized) SetupShadowScissorState(gl);
  return shadow_scissor_enabled;
}

/* static */ void Scissor::SetScissorEnabled(ion::gfx::GraphicsManagerPtr gl,
                                             bool enabled) {
  if (!shadow_scissor_initialized) SetupShadowScissorState(gl);
  shadow_scissor_enabled = enabled;
  if (enabled) {
    gl->Enable(GL_SCISSOR_TEST);
  } else {
    gl->Disable(GL_SCISSOR_TEST);
  }
}

Scissor::Scissor(ion::gfx::GraphicsManagerPtr gl, Scissor::Parent mode)
    : gl_(gl) {
  // Capture the initial state of the scissor.
  if (mode == Parent::IGNORE) {
    // Ignore the old state. Force the scissor test off,
    // as the assumption is that we want to define a top-level
    // scissor scope.
    initially_enabled_ = false;
    return;
  }
  initially_enabled_ = GetScissorEnabled(gl);
  if (initially_enabled_) {
    enabled_ = true;
    initial_scissor_ = GetScissorBox(gl);
  }
}

Scissor::~Scissor() {
  if (initially_enabled_) {
    // Put the old version back.
    SetScissorBox(gl_, initial_scissor_);
    return;
  }
  // We were not initially enabled if we are here.
  if (enabled_) {
    // We must have turned scissoring on. Turn it off.
    SetScissorEnabled(gl_, false);
    enabled_ = false;
  }
}

void Scissor::SetScissor(const Camera& camera, const Rect& bounds,
                         CoordType bounds_coord_type) {
  if (!enabled_) {
    SetScissorEnabled(gl_, true);
    enabled_ = true;
  }
  Rect b = bounds_coord_type == CoordType::kScreen
               ? bounds
               : geometry::Transform(bounds, camera.WorldToScreen());
  if (initially_enabled_) {
    geometry::Intersection(b, initial_scissor_, &b);
  }
  SetScissorBox(gl_, b);
}

}  // namespace ink
