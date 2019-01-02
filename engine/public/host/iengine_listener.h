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

#ifndef INK_ENGINE_PUBLIC_HOST_IENGINE_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_IENGINE_LISTENER_H_

#include <memory>
#include <vector>

#include "logs/proto/research/ink/ink_event_portable_proto.pb.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"
#include "ink/public/document/document.h"

namespace ink {

class IEngineListener : public EventListener<IEngineListener> {
 public:
  ~IEngineListener() override {}

  virtual void ImageExportComplete(uint32_t widthPx, uint32_t heightPx,
                                   const std::vector<uint8_t>& imgBytes,
                                   uint64_t fingerprint) = 0;
  virtual void PdfSaveComplete(const std::string& pdf_bytes) = 0;
  virtual void ToolEvent(const proto::ToolEvent& tool_event) = 0;
  virtual void SequencePointReached(int32_t sequencePointId) = 0;
  virtual void UndoRedoStateChanged(bool canUndo, bool canRedo) = 0;
  virtual void FlagChanged(const proto::Flag& which, bool enabled) = 0;
  virtual void LoggingEventFired(
      const ::logs::proto::research::ink::InkEvent& event) = 0;
};

}  // namespace ink
#endif  // INK_ENGINE_PUBLIC_HOST_IENGINE_LISTENER_H_
