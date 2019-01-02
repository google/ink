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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_GL_RESOURCE_MANAGER_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_GL_RESOURCE_MANAGER_H_

#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "geo/render/ion/gfx/graphicsmanager.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/rendering/gl_managers/background_state.h"
#include "ink/engine/rendering/gl_managers/ion_graphics_manager_provider.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/shader_manager.h"
#include "ink/engine/rendering/gl_managers/texture_manager.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {

class GLResourceManager {
 public:
  using SharedDeps = service::Dependencies<IPlatform, FrameState, ITaskRunner,
                                           IonGraphicsManagerProvider>;

  explicit GLResourceManager(const service::UncheckedRegistry& registry);
  GLResourceManager(
      std::shared_ptr<IPlatform> platform,
      std::shared_ptr<FrameState> frame_state,
      std::shared_ptr<ITaskRunner> task_runner,
      std::shared_ptr<IonGraphicsManagerProvider> graphics_manager_provider);

  // Disallow copy and assign.
  GLResourceManager(const GLResourceManager&) = delete;
  GLResourceManager& operator=(const GLResourceManager&) = delete;

  ~GLResourceManager();

  /**
   * All GL calls should go through this object, which handles the dirty work of
   * finding the correct symbol and linking it, if needed. So, instead of
   * calling glFooBar(GL_SOME_ENUM); you should do
   *   resource_manager_->gl->FooBar(GL_SOME_ENUM);
   * You can also safely copy the gl field into your own
   * ion::gfx::GraphicsManagerPtr, which is a shared ptr with ref counting.
   */
  ion::gfx::GraphicsManagerPtr gl;

  // These are all shared pointers because they have mutual dependencies, but
  // all are ultimately owned by GLResourceManager.
  std::shared_ptr<TextureManager> texture_manager;
  const std::string platform_id;
  std::shared_ptr<BackgroundState> background_state;
  std::shared_ptr<MeshVBOProvider> mesh_vbo_provider;

  // ShaderManager must be last because it needs access to the other managers.
  std::shared_ptr<ShaderManager> shader_manager;

  bool IsMSAASupported() const { return msaa_supported_; }

 private:
  bool msaa_supported_;

  bool DetectMSAASupport();

  // Platforms that might claim to support MSAA, but that we don't trust.
  // Currently we aren't allowing MSAA on pre-KitKat Android devices (ICS
  // through JB_MR2) since there are some catastrophic behavior from some older
  // GL drivers, eg b/25497041 MSAA disabled on Native Client on MacOS <10.12.5
  // due to Intel GPU driver bug: b/38280481
  const std::vector<std::string> kPlatformMsaaBlacklist{
      "Android/14", "Android/15", "Android/16",
      "Android/17", "Android/18", "NaCl/NoMSAA"};

  // Regexes of GPU names that might support MSAA but are known to perform badly
  // with MSAA on. Mali-T600 range are the main known-bad perf targets here.
  // The currently known good GPUS are any NVidia Tegra 3+, Adreno 300 and 400s.
  const std::vector<std::regex> kGpuRegexMsaaBlacklist{std::regex("Mali-T6..")};
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_GL_RESOURCE_MANAGER_H_
