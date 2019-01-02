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

#ifndef INK_ENGINE_PUBLIC_HOST_FAKE_ENGINE_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_FAKE_ENGINE_LISTENER_H_

#include "ink/engine/public/host/iengine_listener.h"

namespace ink {

class FakeEngineListener : public IEngineListener {
 public:
  void ImageExportComplete(uint32_t widthPx, uint32_t heightPx,
                           const std::vector<uint8_t>& imgBytes,
                           uint64_t fingerprint) override {}
  void ToolEvent(const proto::ToolEvent& tool_event) override {}
  void SequencePointReached(int32_t sequencePointId) override {}
  void UndoRedoStateChanged(bool canUndo, bool canRedo) override {}
  void FlagChanged(const proto::Flag& which, bool enabled) override {}
  void LoggingEventFired(
      const ::logs::proto::research::ink::InkEvent& event) override {}
  void PdfSaveComplete(const std::string& pdf_bytes) override {}
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_FAKE_ENGINE_LISTENER_H_
