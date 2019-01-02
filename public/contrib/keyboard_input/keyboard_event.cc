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

#include "ink/public/contrib/keyboard_input/keyboard_event.h"

namespace ink {
namespace input {
namespace keyboard {

KeyEvent::KeyEvent(Keycode virtualcode, char state_flags)
    : virtualcode(virtualcode), state_flags(state_flags) {}

bool KeyEvent::operator==(const KeyEvent& other) const {
  return other.virtualcode == virtualcode && other.state_flags == state_flags;
}

}  // namespace keyboard
}  // namespace input
}  // namespace ink
