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

#ifndef INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_STATE_H_
#define INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_STATE_H_

#include "third_party/absl/types/optional.h"
#include "ink/public/contrib/keyboard_input/keyboard_event.h"

namespace ink {
namespace input {
namespace keyboard {

// Logical modifiers. Can be set by multiple physical keys.
enum KeyModifier {
  kAltModifier,
  kControlModifier,
  kShiftModifier,
  kSuperModifier
};

// Represents a keyboard state.
class State {
 public:
  State();
  bool IsDown(Keycode k) const;
  bool IsUp(Keycode k) const;
  void SetDown(Keycode k, bool is_down);
  void SetAllUp();

  bool IsModifierDown(KeyModifier modifier) const;

  // Checks whether the specified modifier key (i.e., one of
  // alt/ctrl/shift/super) is the *only* one that's currently pressed down. This
  // allows you to easily distinguish between a "ctrl+z" vs.
  // "ctrl+shift+z" without having to make multiple calls to
  // IsModifierDown(), the non-exclusive version of this method.
  bool IsModifierExclusivelyDown(KeyModifier modifier) const;
  bool IsModifierUp(KeyModifier modifier) const;

  bool operator==(const State& other) const;

 private:
  std::vector<bool> data_;
};

}  // namespace keyboard
}  // namespace input
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_STATE_H_
