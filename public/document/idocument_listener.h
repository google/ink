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

#ifndef INK_PUBLIC_DOCUMENT_IDOCUMENT_LISTENER_H_
#define INK_PUBLIC_DOCUMENT_IDOCUMENT_LISTENER_H_

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

class IDocumentListener : public EventListener<IDocumentListener> {
 public:
  virtual void UndoRedoStateChanged(bool canUndo, bool canRedo) = 0;
  virtual void EmptyStateChanged(bool empty) = 0;
};

};  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_IDOCUMENT_LISTENER_H_
