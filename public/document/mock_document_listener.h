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

#ifndef INK_PUBLIC_DOCUMENT_MOCK_DOCUMENT_LISTENER_H_
#define INK_PUBLIC_DOCUMENT_MOCK_DOCUMENT_LISTENER_H_

#include "testing/base/public/gmock.h"
#include "ink/public/document/idocument_listener.h"

namespace ink {

class MockDocumentListener : public IDocumentListener {
 public:
  MOCK_METHOD2(UndoRedoStateChanged, void(bool canUndo, bool canRedo));
  MOCK_METHOD1(EmptyStateChanged, void(bool empty));
};

};  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_MOCK_DOCUMENT_LISTENER_H_
