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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_SCISSOR_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_SCISSOR_H_

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/rect.h"

namespace ink {

// Scissor will capture the state of GL_SCISSOR_TEST and (potentially)
// GL_SCISSOR_BOX on construction.
//
// Users can then call SetScissor to manipulate the scissor state. The actual
// scissor bounds that will be sent to GL will be the intersection of the
// new bounds and the initial bound (if present).
//
// On destruction, the original state of scissoring will be set back. If
// scissoring was enabled prior to object creation, the original bounds will be
// reset.
//
// Setting Parent::IGNORE on the constructor will force scissor to
// act as if there was no scissor defined, regardless of the truth. This is
// useful when you want to assume complete control of the scissor parameters.
//
// Usage:
// {
//   Scissor scissor(gl);  // Captures the parent state.
//   scissor.SetScissor(cam, Rect(0,0,5,5), CoordType::kWorld);
//   <draw calls>
//   scissor.SetScissor(cam, Rect(0,0,10,10), CoordType::kWorld);
//   <other draw calls>
// }  // Scissor is back to the original.
class Scissor {
 public:
  enum class Parent { INTERSECT = 0, IGNORE = 1 };

  // When parent is INHERIT, this will capture the current state of the
  // scissor, if one exists. Any calls to SetScissor will be set to an
  // intersection of the original parent.
  // When parent is IGNORE, this will ignore the current state of
  // the scissor, assuming it is off by default. When destructed, scissor
  // will be disabled.
  explicit Scissor(ion::gfx::GraphicsManagerPtr gl,
                   Parent mode = Parent::INTERSECT);
  ~Scissor();

  // Sets the scissor. The bounds will be transformed into
  // screen coordinates for scissoring. Note that if there was a parent
  // scissor present, the bounds will be clipped to the parent state.
  // If there was a parent present AND the new bounds are out of the
  // parent's bounds, then the bounds are effectively empty, resulting
  // in all draw calls to be thrown out.
  void SetScissor(const Camera& camera, const Rect& bounds,
                  CoordType bounds_coord_type);

  // If you wish to query and manipulate GL scissor state, you must do it
  // through these functions, which maintain a shadow scissor state in CPU.
  static Rect GetScissorBox(ion::gfx::GraphicsManagerPtr gl);
  static void SetScissorBox(ion::gfx::GraphicsManagerPtr gl, Rect box);
  static bool GetScissorEnabled(ion::gfx::GraphicsManagerPtr gl);
  static void SetScissorEnabled(ion::gfx::GraphicsManagerPtr gl, bool enabled);

 private:
  ion::gfx::GraphicsManagerPtr gl_;
  bool initially_enabled_ = false;
  bool enabled_ = false;
  Rect initial_scissor_;  // initial state if was_enabled_
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_SCISSOR_H_
