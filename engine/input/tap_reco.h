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

#ifndef INK_ENGINE_INPUT_TAP_RECO_H_
#define INK_ENGINE_INPUT_TAP_RECO_H_

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"

namespace ink {
namespace input {

// TapStatus follows the following state machine:
//
// TapData always starts as NotStarted
//
// NotStarted:
//   * becomes Ambiguous on pointer down
// Ambiguous:
//   * becomes NoTap if the pointer moves sufficiently or Cancel seen
//   * becomes Tap if the pointer is released.
//   * becomes LongPressHeld if the user is still holding after
//     kLongPressThreshold time has passed.
// LongPressHeld:
//   * becomes NoTap if the pointer moves sufficiently or Cancel seen
//   * becomes LongPressReleased if the pointer is released
// Tap:
//   * terminal (new input will start back as NotStarted)
// LongPressReleased:
//   * terminal (new input will start back as NotStarted)
// NoTap:
//   * terminal (new input will start back as NotStarted)
//
// Besides those state changes, all states change to NoTap if we get a Cancel
// input.
//
// The following InputData members are correct to read off of TapData based on
// the current
// TapStatus:
//
// NotStarted: []
// Ambiguous, LongPressedHeld, NoTap: [down, current]
// Tap, LongPressedReleased: [down, current, up]

class TapData;
typedef std::function<void(TapData)> TapStateChangeHandler;

enum class TapStatus {
  NotStarted,     // no tdown seen yet
  Ambiguous,      // tdown seen, but we aren't yet sure if the user is tapping.
  LongPressHeld,  // long press still held down
  NoTap,          // we know the user did not tap in this stream
  Tap,            // the user tapped
  LongPressReleased,  // long press released (without moving too far)
};

class TapData {
 public:
  TapStatus status;
  InputData down_data;
  InputData up_data;
  InputData current_data;

  TapData();

  bool IsTap() const;
  bool IsAmbiguous() const;
  bool IsTerminalState() const;

  std::string ToString() const;
};

// TapReco keeps track of the TapStatus for each separate pointer. It also
// optionally dispatches callbacks to the TapStateChangeHandler whenever a
// TapStatus changes state.
class TapReco {
 public:
  TapReco() = default;
  TapReco(const TapReco&) = delete;
  TapReco& operator=(const TapReco&) = delete;
  virtual ~TapReco() = default;

  void Reset();
  virtual TapData OnInput(const InputData& data, const Camera& camera);

  TapStateChangeHandler change_handler_;

 protected:
  std::unordered_map<uint32_t, TapData> id_to_tap_data_;
};

namespace impl {
// The constants below are exposed only for testing purposes.

// The largest movement, in cm, from the down touch, to still be recognized as
// a long press.
constexpr float kMaxLongPressDistanceCm = 0.7f;

// The fastest movement, in cm/s, allowed between input events to still be
// recognized as a long press.
constexpr float kMaxLongPressSpeedCmPerSec = 10.0f;

// The minimum amount of time that touch needs to be maintained before it
// becomes a long press. This matches WebKit's delay of 300ms when deciding
// whether a tap is a tap.
constexpr DurationS kLongPressThreshold = 0.3f;
}  // namespace impl

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_TAP_RECO_H_
