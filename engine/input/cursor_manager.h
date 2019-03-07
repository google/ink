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

#ifndef INK_ENGINE_INPUT_CURSOR_MANAGER_H_
#define INK_ENGINE_INPUT_CURSOR_MANAGER_H_

#include <memory>

#include "ink/engine/input/cursor.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {
namespace input {

class CursorManager {
 public:
  using SharedDeps = service::Dependencies<IPlatform, InputDispatch>;

  explicit CursorManager(const service::Registry<CursorManager>& registry);
  explicit CursorManager(const service::UncheckedRegistry& registry);
  CursorManager(std::shared_ptr<IPlatform> platform,
                std::shared_ptr<InputDispatch> input_dispatch);

  // Disallow copy and assign.
  CursorManager(const CursorManager&) = delete;
  CursorManager& operator=(const CursorManager&) = delete;

  // Determines what the current cursor should be, and if it has changed, sends
  // a SetCursor() callback to the IPlatform.
  void Update(const Camera& camera);

 private:
  // Calls platform_->SetCursor() with current_cursor_.
  void SendCursorCallback();

  std::shared_ptr<IPlatform> platform_;
  std::shared_ptr<InputDispatch> input_dispatch_;
  Cursor current_cursor_;
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_CURSOR_MANAGER_H_
