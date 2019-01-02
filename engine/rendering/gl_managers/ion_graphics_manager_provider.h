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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_ION_GRAPHICS_MANAGER_PROVIDER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_ION_GRAPHICS_MANAGER_PROVIDER_H_

#include "geo/render/ion/base/staticsafedeclare.h"
#include "geo/render/ion/gfx/graphicsmanager.h"

namespace ink {

class IonGraphicsManagerProvider {
 public:
  IonGraphicsManagerProvider() {}
  virtual ~IonGraphicsManagerProvider() {}

  virtual ion::gfx::GraphicsManagerPtr GetGraphicsManager() {
    ION_DECLARE_SAFE_STATIC_POINTER_WITH_CONSTRUCTOR(
        ion::gfx::GraphicsManagerPtr, sShared,
        (new ion::gfx::GraphicsManagerPtr(new ion::gfx::GraphicsManager())));
    return *sShared;
  }
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_ION_GRAPHICS_MANAGER_PROVIDER_H_
