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

// The interface that defines the services that the engine requires from the
// embedding platform, i.e., things that are platform-dependent.

#ifndef INK_ENGINE_PUBLIC_HOST_IPLATFORM_H_
#define INK_ENGINE_PUBLIC_HOST_IPLATFORM_H_

#include <cstdint>
#include <functional>
#include <string>

#include "ink/engine/public/types/client_bitmap.h"
#include "ink/proto/sengine_portable_proto.pb.h"
#include "ink/proto/text_portable_proto.pb.h"

namespace ink {

class IPlatform {
 public:
  virtual ~IPlatform() {}

  // RequestFrame can be run from any arbitary thread.
  // At least 1 call to sengine.draw must occur *after* this notification.
  virtual void RequestFrame() = 0;

  virtual void SetTargetFPS(uint32_t fps) = 0;
  virtual uint32_t GetTargetFPS() const = 0;
  virtual void BindScreen() = 0;
  virtual void RequestImage(const std::string& uri) = 0;
  // Render the given text proto at the given bitmap dimensions.
  // This function is called from the Ink engine's task thread; it is not called
  // on the GL thread (nor, where it's different, on the embedding host's main
  // thread).
  virtual std::unique_ptr<ClientBitmap> RenderText(
      const proto::text::Text& text, int width_px, int height_px) = 0;
  virtual std::string GetPlatformId() const = 0;
  virtual bool ShouldPreloadShaders() const = 0;
  virtual void SetCursor(const ink::proto::Cursor& cursor) = 0;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_IPLATFORM_H_
