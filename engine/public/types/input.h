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

#ifndef INK_ENGINE_PUBLIC_TYPES_INPUT_H_
#define INK_ENGINE_PUBLIC_TYPES_INPUT_H_

#include <cstdint>
#include <string>

/*
 * PORTABLE
 * No STL or sketchology headers permitted here.
 */

namespace ink {
namespace input {

// Helper ids to keep mouse ids consistent.
// Simultaneous mouse buttons must use different ids in InputData::id
//
// These are provided as a helper for input sources. Input sinks should never
// read id to determine the input type or action. (Instead use InputType/Flag)
//
// Warning: If you send simultaneous mouse and touch, make sure these ids don't
// overlap with your touch id range!
enum MouseIds : uint32_t {
  MouseHover = (1 << 16) + 0,
  MouseLeft = (1 << 16) + 1,
  MouseRight = (1 << 16) + 2,
  MouseWheel = (1 << 16) + 3,
};

enum class Flag {
  InContact = 1 << 0,  // In Contact
  Left = 1 << 1,       // Left mouse button
  Right = 1 << 2,      // Right mouse button
  TDown = 1 << 3,      // Transitioned to down
  TUp = 1 << 4,        // Transitioned to up
  Wheel = 1 << 5,      // Mouse wheel
  UNUSED = 1 << 6,     // Former event flag; unused but still exists in some
                       // serialized unit test inputs.
  Cancel = 1 << 7,     // Canceled stroke
  Fake = 1 << 8,       // Faked input data
  Primary = 1 << 9,    // First input down.
  Eraser = 1 << 10,    // Eraser button.
  Shift = 1 << 11,     // The shift key is down.
  Control = 1 << 12,   // The control key is down.
  Alt = 1 << 13,       // The alt key is down.
  Meta = 1 << 14,      // The meta key is down.
};

// Combine an arbitrary list of Flags into a single packed bit field
// Ex flagBitfield(Flag::TDown, Flag::InContact) == TDown|InContact
constexpr uint32_t FlagBitfield(Flag flag) {
  return static_cast<uint32_t>(flag);
}

template <typename... Ts>
constexpr uint32_t FlagBitfield(Flag flag, Ts... flags) {
  return FlagBitfield(flag) | FlagBitfield(flags...);
}

// Type of input device that generated the data.
//
// Invalid is used to represent uninitialized input data or parameter fields.
enum class InputType {
  Invalid = 0,
  Mouse = 1,
  Touch = 2,
  Pen = 3,
};

inline std::string InputTypeString(InputType t) {
  switch (t) {
    case InputType::Invalid:
      return "Invalid";
    case InputType::Mouse:
      return "Mouse";
    case InputType::Touch:
      return "Touch";
    case InputType::Pen:
      return "Pen";
  }
}

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_INPUT_H_
