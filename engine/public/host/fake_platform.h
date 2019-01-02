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

#ifndef INK_ENGINE_PUBLIC_HOST_FAKE_PLATFORM_H_
#define INK_ENGINE_PUBLIC_HOST_FAKE_PLATFORM_H_

#include <cstdint>
#include <memory>
#include <string>

#include "ink/engine/public/host/iplatform.h"

namespace ink {
class FakePlatform : public IPlatform {
 public:
  void RequestFrame() override {}
  void SetTargetFPS(uint32_t fps) override {}
  uint32_t GetTargetFPS() const override { return 0; }
  void BindScreen() override {}
  void RequestImage(const std::string& uri) override {}
  std::unique_ptr<ClientBitmap> RenderText(const proto::text::Text& text,
                                           int width, int height) override {
    return nullptr;
  }
  std::string GetPlatformId() const override { return ""; }
  bool ShouldPreloadShaders() const override { return false; }
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_FAKE_PLATFORM_H_
