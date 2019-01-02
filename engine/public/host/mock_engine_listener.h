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

#ifndef INK_ENGINE_PUBLIC_HOST_MOCK_ENGINE_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_MOCK_ENGINE_LISTENER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "logs/proto/research/ink/ink_event_portable_proto.pb.h"
#include "testing/base/public/gmock.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/service/dependencies.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

class MockEngineListener : public IEngineListener {
 public:
  MOCK_METHOD4(ImageExportComplete, void(uint32_t width_px, uint32_t height_px,
                                         const std::vector<uint8_t>& img_bytes,
                                         uint64_t fingerprint));
  MOCK_METHOD1(SequencePointReached, void(int32_t id));
  MOCK_METHOD1(ToolEvent, void(const proto::ToolEvent& event));
  MOCK_METHOD2(UndoRedoStateChanged, void(bool canUndo, bool canRedo));
  MOCK_METHOD2(FlagChanged, void(const proto::Flag& which, bool enabled));
  MOCK_METHOD1(LoggingEventFired,
               void(const ::logs::proto::research::ink::InkEvent&));
  MOCK_METHOD1(PdfSaveComplete, void(const std::string& bytes));
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_MOCK_ENGINE_LISTENER_H_
