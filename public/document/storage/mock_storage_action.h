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

#ifndef INK_PUBLIC_DOCUMENT_STORAGE_MOCK_STORAGE_ACTION_H_
#define INK_PUBLIC_DOCUMENT_STORAGE_MOCK_STORAGE_ACTION_H_

#include "ink/public/document/storage/storage_action.h"

#include "testing/base/public/gmock.h"
#include "ink/engine/public/types/status.h"

namespace ink {

class MockStorageAction : public StorageAction {
 public:
  MockStorageAction(
      std::shared_ptr<DocumentStorage> storage,
      std::shared_ptr<EventDispatch<IElementListener>> dispatch,
      std::shared_ptr<EventDispatch<IMutationListener>> mutation_dispatch)
      : StorageAction(storage, dispatch, mutation_dispatch) {}
  MOCK_CONST_METHOD0(AffectedUUIDs, std::vector<UUID>());
  MOCK_METHOD0(UndoImpl, Status());
  MOCK_METHOD0(RedoImpl, Status());

  void Apply() {
    EXPECT(state_ == State::kUninitialized);
    state_ = State::kApplied;
  }

  std::string ToString() const override { return "<MockStorageAction>"; }

 protected:
  void WriteFieldsToProto(ink::proto::StorageAction* proto) const override {}
  void RestoreFieldsFromProto(
      const ink::proto::StorageAction& proto) override{};
};

}  // namespace ink

#endif  // INK_PUBLIC_DOCUMENT_STORAGE_MOCK_STORAGE_ACTION_H_
