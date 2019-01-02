// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/input/input_handler.h"

#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace input {

InputHandler::InputHandler() : InputHandler(Priority::Default) {}

InputHandler::InputHandler(Priority priority)
    : is_registered_(false),
      registration_token_(0),
      refuse_all_new_input_(false),
      priority_(priority) {}

InputHandler::~InputHandler() {
  if (is_registered_) {
    dispatch_->UnregisterHandler(registration_token_);
  }
}

void InputHandler::RegisterForInput(std::shared_ptr<InputDispatch> dispatch) {
  dispatch_ = dispatch;

  registration_token_ = dispatch_->RegisterHandler(this);
  is_registered_ = true;
}

}  // namespace input
}  // namespace ink
