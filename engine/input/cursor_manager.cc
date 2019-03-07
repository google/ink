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

#include "ink/engine/input/cursor_manager.h"

#include <memory>

#include "ink/engine/input/cursor.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace input {

CursorManager::CursorManager(const service::Registry<CursorManager>& registry)
    : CursorManager(registry.GetShared<IPlatform>(),
                    registry.GetShared<InputDispatch>()) {}

CursorManager::CursorManager(const service::UncheckedRegistry& registry)
    : CursorManager(registry.GetShared<IPlatform>(),
                    registry.GetShared<InputDispatch>()) {}

CursorManager::CursorManager(std::shared_ptr<IPlatform> platform,
                             std::shared_ptr<InputDispatch> input_dispatch)
    : platform_(platform), input_dispatch_(input_dispatch) {}

void CursorManager::Update(const Camera& camera) {
  Cursor cursor = input_dispatch_->GetCurrentCursor(camera);
  if (cursor != current_cursor_) {
    current_cursor_ = cursor;
    SendCursorCallback();
  }
}

void CursorManager::SendCursorCallback() {
  ink::proto::Cursor cursor;
  ink::util::WriteToProto(&cursor, current_cursor_);
  platform_->SetCursor(cursor);
}

}  // namespace input
}  // namespace ink
