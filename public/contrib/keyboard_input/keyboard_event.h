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

#ifndef INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_EVENT_H_
#define INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_EVENT_H_

#include <cstdint>
#include <string>
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/types/variant.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {
namespace input {
namespace keyboard {

// Numerical keycodes.
// Try and map directly to ASCII where possible.
// IMPORTANT! Don't renumber any value marked as ascii-mapped. Translation
// is handled implicitly / via casting in some places.
enum Keycode : unsigned char {
  kLowerLimit = 0,  // The lowest keycode
  kInvalid = 0,
  kLeftArrow = 1,
  kUpArrow = 2,
  kRightArrow = 3,
  kDownArrow = 4,
  kLeftControl = 5,
  kLeftShift = 6,
  kLeftAlt = 7,
  kBackspace = 8,  // ASCII mapped.
  kTab = 9,        // ASCII mapped.
  kRightControl = 10,
  kRightShift = 11,
  kRightAlt = 12,
  kEnter = 13,  // ASCII mapped.
  kPageUp = 14,
  kPageDown = 15,
  kHome = 16,
  kEnd = 17,
  kLeftSuper = 18,
  kRightSuper = 19,
  kEscape = 27,  // ASCII mapped.
  kPrintableRangeStart1 = 32,
  kSpace = 32,         // ASCII mapped.
  kExclamation = 33,   // ASCII mapped.
  kDoubleQuote = 34,   // ASCII mapped.
  kHashtag = 35,       // ASCII mapped.
  kDollar = 36,        // ASCII mapped.
  kModulo = 37,        // ASCII mapped.
  kAmpersand = 38,     // ASCII mapped.
  kSingleQuote = 39,   // ASCII mapped.
  kOpenParen = 40,     // ASCII mapped.
  kCloseParen = 41,    // ASCII mapped.
  kMultiply = 42,      // ASCII mapped.
  kPlus = 43,          // ASCII mapped.
  kComma = 44,         // ASCII mapped.
  kMinus = 45,         // ASCII mapped.
  kPeriod = 46,        // ASCII mapped.
  kForwardSlash = 47,  // ASCII mapped.
  kZero = 48,          // ASCII mapped.
  kOne = 49,           // ASCII mapped.
  kTwo = 50,           // ASCII mapped.
  kThree = 51,         // ASCII mapped.
  kFour = 52,          // ASCII mapped.
  kFive = 53,          // ASCII mapped.
  kSix = 54,           // ASCII mapped.
  kSeven = 55,         // ASCII mapped.
  kEight = 56,         // ASCII mapped.
  kNine = 57,          // ASCII mapped.
  kColon = 58,         // ASCII mapped.
  kSemicolon = 59,     // ASCII mapped.
  kLessThan = 60,      // ASCII mapped.
  kEquals = 61,        // ASCII mapped.
  kGreaterThan = 62,   // ASCII mapped.
  kQuestion = 63,      // ASCII mapped.
  kAt = 64,            // ASCII mapped.
  kPrintableRangeEnd1 = 64,
  kPrintableRangeStart2 = 91,
  kOpenBracket = 91,   // ASCII mapped.
  kBackslash = 92,     // ASCII mapped.
  kCloseBracket = 93,  // ASCII mapped.
  kCaret = 94,         // ASCII mapped.
  kUnderscore = 95,    // ASCII mapped.
  kBacktick = 96,      // ASCII mapped.
  kA = 97,             // ASCII mapped.
  kB = 98,             // ASCII mapped.
  kC = 99,             // ASCII mapped.
  kD = 100,            // ASCII mapped.
  kE = 101,            // ASCII mapped.
  kF = 102,            // ASCII mapped.
  kG = 103,            // ASCII mapped.
  kH = 104,            // ASCII mapped.
  kI = 105,            // ASCII mapped.
  kJ = 106,            // ASCII mapped.
  kK = 107,            // ASCII mapped.
  kL = 108,            // ASCII mapped.
  kM = 109,            // ASCII mapped.
  kN = 110,            // ASCII mapped.
  kO = 111,            // ASCII mapped.
  kP = 112,            // ASCII mapped.
  kQ = 113,            // ASCII mapped.
  kR = 114,            // ASCII mapped.
  kS = 115,            // ASCII mapped.
  kT = 116,            // ASCII mapped.
  kU = 117,            // ASCII mapped.
  kV = 118,            // ASCII mapped.
  kW = 119,            // ASCII mapped.
  kX = 120,            // ASCII mapped.
  kY = 121,            // ASCII mapped.
  kZ = 122,            // ASCII mapped.
  kOpenCurly = 123,    // ASCII mapped.
  kPipe = 124,         // ASCII mapped.
  kCloseCurly = 125,   // ASCII mapped.
  kTilde = 126,        // ASCII mapped.
  kPrintableRangeEnd2 = 126,
  kDelete = 127,      // ASCII mapped.
  kUpperLimit = 128,  // 1 past the maximum keycode.
};

constexpr Keycode CharToKeyCode(char c) { return static_cast<Keycode>(c); }

enum StateFlags : char {
  kTDown = 1ul << 0,   // Represents a state transition: up to down.
  kTUp = 1ul << 1,     // Represents a state transition: down to up.
  kRepeat = 1ul << 2,  // Represents that the key has been held long enough to
                       // trigger key repetition.
};

// Describes a raw keyboard event.
// If you're looking at this, you should have first ruled out using translation
// maps and UTF8InputEvents.
struct KeyEvent {
  // The virtual keycode. This value has gone through scancode decoding but
  // nothing more.
  Keycode virtualcode = kInvalid;
  // A bitfield of StateFlags.
  char state_flags = 0;

  KeyEvent() {}
  KeyEvent(Keycode virtualcode, char state_flags);
  bool operator==(const KeyEvent& other) const;
};

// Describes a high-level text insertion event. Typically text will be
// a single grapheme that has gone through both scancode decoding and utf
// translation.
struct UTF8InputEvent {
  std::string text;
  bool operator==(const UTF8InputEvent& other) const {
    return text == other.text;
  }
};

using Event = absl::variant<KeyEvent, UTF8InputEvent>;

}  // namespace keyboard
}  // namespace input

template <>
inline std::string Str(const input::keyboard::Keycode& t) {
  return Str(static_cast<unsigned char>(t));
}

}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_EVENT_H_
