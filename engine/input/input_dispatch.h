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

#ifndef INK_ENGINE_INPUT_INPUT_DISPATCH_H_
#define INK_ENGINE_INPUT_INPUT_DISPATCH_H_

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/cursor.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"

namespace ink {
namespace input {
class IInputHandler;

class InputDispatch {
 public:
  using SharedDeps = service::Dependencies<settings::Flags>;

  explicit InputDispatch(std::shared_ptr<settings::Flags> flags);

  // Disallow copy and assign.
  InputDispatch(const InputDispatch&) = delete;
  InputDispatch& operator=(const InputDispatch&) = delete;

  uint32_t RegisterHandler(IInputHandler* handler);
  void UnregisterHandler(uint32_t token);

  void Dispatch(const Camera& cam, InputData data);
  void ForceAllUp(const Camera& cam);

  // Determines what the current mouse cursor should be, by dispatching to input
  // handlers (that haven't refused input) and returning the first non-null
  // cursor specified.  If no input handler wants to specify a cursor, returns
  // the default cursor.
  Cursor GetCurrentCursor(const Camera& cam) const;

  size_t GetNContacts() const { return input_id_to_last_input_.size(); }

 private:
  void OnHandlersChanged();
  void ReleaseCapturedStreams(IInputHandler* handler);
  void AddRefusedHandler(IInputHandler* handler);

  std::unordered_map<uint32_t, IInputHandler*> input_id_to_capturer_;
  std::unordered_map<uint32_t, IInputHandler*> token_to_handler_;
  std::vector<IInputHandler*> sorted_handlers_;
  std::unordered_map<uint32_t, InputData> input_id_to_last_input_;
  std::unordered_set<IInputHandler*> refused_handlers_;
  std::shared_ptr<settings::Flags> flags_;
  uint32_t next_token_;
  bool pen_used_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_INPUT_DISPATCH_H_
