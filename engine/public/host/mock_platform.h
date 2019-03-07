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

#ifndef INK_ENGINE_PUBLIC_HOST_MOCK_PLATFORM_H_
#define INK_ENGINE_PUBLIC_HOST_MOCK_PLATFORM_H_

#include <cstdint>
#include <string>
#include <unordered_set>

#include "testing/base/public/gmock.h"
#include "ink/engine/public/host/iplatform.h"

namespace ink {
class MockPlatform : public IPlatform {
 public:
  MOCK_METHOD0(RequestFrame, void());
  MOCK_METHOD1(SetTargetFPS, void(uint32_t));
  MOCK_CONST_METHOD0(GetTargetFPS, uint32_t());
  MOCK_METHOD0(BindScreen, void());
  MOCK_METHOD1(RequestImage, void(const std::string&));
  MOCK_CONST_METHOD0(GetPlatformId, std::string());
  MOCK_METHOD3(RenderText,
               std::unique_ptr<ClientBitmap>(const proto::text::Text&, int,
                                             int));
  MOCK_METHOD3(GetTextLayout,
               std::unique_ptr<proto::text::Layout>(const proto::text::Text&,
                                                    int, int));
  MOCK_METHOD1(SetCursor, void(const ink::proto::Cursor& cursor));
  bool ShouldPreloadShaders() const override { return false; }
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_MOCK_PLATFORM_H_
