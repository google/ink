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

#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"

#include <algorithm>

#include "geo/render/ion/portgfx/glheaders.h"
#include "geo/render/ion/portgfx/isextensionsupported.h"
#include "ink/engine/gl.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

GLResourceManager::GLResourceManager(const service::UncheckedRegistry& registry)
    : GLResourceManager(registry.GetShared<IPlatform>(),
                        registry.GetShared<FrameState>(),
                        registry.GetShared<ITaskRunner>(),
                        registry.GetShared<IonGraphicsManagerProvider>()) {}

GLResourceManager::GLResourceManager(
    std::shared_ptr<IPlatform> platform,
    std::shared_ptr<FrameState> frame_state,
    std::shared_ptr<ITaskRunner> task_runner,
    std::shared_ptr<IonGraphicsManagerProvider> graphics_manager_provider)
    : gl(graphics_manager_provider->GetGraphicsManager()),
      platform_id(platform->GetPlatformId()) {
  texture_manager = std::make_shared<TextureManager>(gl, platform, frame_state,
                                                     std::move(task_runner));
  background_state = std::make_shared<BackgroundState>();
  mesh_vbo_provider = std::make_shared<MeshVBOProvider>(gl);
  shader_manager = std::make_shared<ShaderManager>(
      gl, background_state, mesh_vbo_provider, texture_manager);
  msaa_supported_ = DetectMSAASupport();
}

GLResourceManager::~GLResourceManager() {
  background_state->ClearImage(texture_manager.get());
}

bool GLResourceManager::DetectMSAASupport() {
  using ion::gfx::GraphicsManager;
  // Query Ion for multisampling support.
  if (!gl->IsFeatureAvailable(GraphicsManager::kRenderbufferMultisample)) {
    SLOG(SLOG_INFO,
         "MSAA not supported because renderbuffer multisampling is not "
         "supported by OpenGL.");
    return false;
  }
  if (!gl->IsFeatureAvailable(GraphicsManager::kFramebufferBlit) &&
      !gl->IsFeatureAvailable(
          GraphicsManager::kMultisampleFramebufferResolve)) {
    SLOG(SLOG_INFO,
         "MSAA not supported because framebuffer resolve functions are not "
         "supported by OpenGL.");
    return false;
  }
  SLOG(SLOG_INFO, "platform_id = $0", platform_id);
  if (std::find(kPlatformMsaaBlacklist.begin(), kPlatformMsaaBlacklist.end(),
                platform_id) != kPlatformMsaaBlacklist.end()) {
    // Don't believe their lies.
    SLOG(SLOG_INFO, "MSAA blacklisted explicitly for this model");
    return false;
  }

  if (const char *renderer_cstr =
          reinterpret_cast<const char *>(gl->GetString(GL_RENDERER))) {
    const std::string gpu_name(renderer_cstr);
    SLOG(SLOG_INFO, "gpuName = $0", gpu_name);
    for (auto &regex : kGpuRegexMsaaBlacklist) {
      if (std::regex_search(gpu_name, regex)) {
        SLOG(SLOG_INFO, "MSAA blacklisted for this gpu");
        return false;
      }
    }
  }
  SLOG(SLOG_INFO, "MSAA is supported");
  return true;
}

}  // namespace ink
