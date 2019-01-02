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

#include "ink/public/contrib/keyboard_input/keyboard_state.h"

namespace ink {
namespace input {
namespace keyboard {

State::State() { data_.resize(Keycode::kUpperLimit, false); }
bool State::IsDown(Keycode k) const { return data_[k]; }
bool State::IsUp(Keycode k) const { return !IsDown(k); }
void State::SetDown(Keycode k, bool is_down) { data_[k] = is_down; }
void State::SetAllUp() { data_.assign(Keycode::kUpperLimit, false); }

bool State::IsModifierDown(KeyModifier modifier) const {
  switch (modifier) {
    case kAltModifier:
      return IsDown(kLeftAlt) || IsDown(kRightAlt);
    case kControlModifier:
      return IsDown(kLeftControl) || IsDown(kRightControl);
    case kShiftModifier:
      return IsDown(kLeftShift) || IsDown(kRightShift);
    case kSuperModifier:
      return IsDown(kLeftSuper) || IsDown(kRightSuper);
  }
}

bool State::IsModifierExclusivelyDown(KeyModifier modifier) const {
  bool alt = IsModifierDown(KeyModifier::kAltModifier);
  bool control = IsModifierDown(KeyModifier::kControlModifier);
  bool shift = IsModifierDown(KeyModifier::kShiftModifier);
  bool super = IsModifierDown(KeyModifier::kSuperModifier);

  switch (modifier) {
    case kAltModifier:
      return alt && !control && !shift && !super;
    case kControlModifier:
      return !alt && control && !shift && !super;
    case kShiftModifier:
      return !alt && !control && shift && !super;
    case kSuperModifier:
      return !alt && !control && !shift && super;
  }
}

bool State::IsModifierUp(KeyModifier modifier) const {
  return !IsModifierDown(modifier);
}

bool State::operator==(const State& other) const {
  return data_ == other.data_;
}

}  // namespace keyboard
}  // namespace input
}  // namespace ink
