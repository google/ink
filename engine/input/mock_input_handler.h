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

#ifndef INK_ENGINE_INPUT_MOCK_INPUT_HANDLER_H_
#define INK_ENGINE_INPUT_MOCK_INPUT_HANDLER_H_

#include "testing/base/public/gmock.h"
#include "ink/engine/input/input_handler.h"

namespace ink {
namespace input {

class MockInputHandler : public IInputHandler {
 public:
  MOCK_METHOD2(OnInput,
               CaptureResult(const InputData& data, const Camera& camera));
  MOCK_METHOD0(RefuseAllNewInput, bool());
  MOCK_CONST_METHOD1(CurrentCursor,
                     absl::optional<Cursor>(const Camera& camera));
  MOCK_CONST_METHOD0(InputPriority, Priority());
  inline std::string ToString() const override { return "<MockInputHandler>"; }
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_MOCK_INPUT_HANDLER_H_
