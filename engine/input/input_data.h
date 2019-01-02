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

// The basic types for working with input in sketchology
//
// Equivalents on other platforms: POINTER_INFO on Windows, MotionEvent on
// Android and UITouch on iOS.

#ifndef INK_ENGINE_INPUT_INPUT_DATA_H_
#define INK_ENGINE_INPUT_INPUT_DATA_H_

#include <cstdint>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/input.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace input {

std::string StringFlags(uint32_t flags);

// Data for a single piece of input
// id, flags (along with the helper function get()), worldPos, and time
// are the most used parts of this class
//
// ids are used to stably track a single source of input over time.
//
// The series of input points from down to up for a single id is referred to
// as a stroke
//
// Ex with 3 fingers down each finger would have a unique id that tracked it's
// position over time. The input source (ie the client) is responsible for
// maintaining id uniqueness.
//
// Several guarantees are provided for input data:
//   - A down will always be followed by a separate up packet
//   - Only one down will be sent until the up is seen
//   - If a cancel is sent, up will also be marked
//   - No ups will be sent without a preceding down
//   - Left|Right will be stable over a stroke
//   - InContact will be stable over a stroke
//   - Input can hover -- InContact won't be set (ex from a stylus or a mouse)
//   - Data will be delivered in order
//   - Time monotonically increases
//   - Duplicate packets are not delivered
//   - Primary is set for the first down seen (and reset on all up)
//   - Primary is stable over a stroke
//
// Ex input stream, 1 finger
//   id:1, flags: TDown|InContact|Primary
//   id:1, flags: InContact|Primary
//   id:1, flags: InContact|Primary
//   id:1, flags: TUp|Primary
//
// Ex input stream, 1 finger, cancelled
//   id:1, flags: TDown|InContact|Primary
//   id:1, flags: InContact|Primary
//   id:1, flags: InContact|Primary
//   id:1, flags: TUp|Cancel|Primary
//
// Ex input stream, 2 fingers
//   id:1, flags: TDown|InContact|Primary
//   id:2, flags: TDown|InContact
//   id:1, flags: InContact|Primary
//   id:2, flags: InContact
//   id:1, flags: InContact|Primary
//   id:2, flags: InContact
//   id:1, flags: TUp|Primary
//   id:2, flags: InContact
//   id:2, flags: InContact
//   id:2, flags: TUp
struct InputData {
  InputType type;

  // The id of this packet. ids track a single contact over time, from down
  // to up (and hover, but a new id may be assigned). ids may or may not be
  // re-used between strokes.
  uint32_t id;

  // Bit field combination of input::Flags
  uint32_t flags;

  // Total number of points in contact
  uint32_t n_down;

  // Position and time data
  // Note when doing calculations on input always use the precalculated last*
  // fields set here. An animating camera can cause manual coordinate
  // conversion to be incorrect.
  glm::vec2 world_pos{0, 0};
  glm::vec2 last_world_pos{0, 0};

  glm::vec2 screen_pos{0, 0};
  glm::vec2 last_screen_pos{0, 0};

  InputTimeS time, last_time;

  // Mousewheel or trackpad scroll amount
  double wheel_delta_x;
  double wheel_delta_y;

  // Pressure for this input. [0, 1]
  // The range isn't enforced and normalization is expected to happen outside
  // of sketchology. A value close to zero represents the lightest possible
  // touch, while close to 1 represents a heavy touch. Values > 1 may be
  // provided to represent exceptional touch events (e.g. a whole finger
  // dragging across a screen), but should generally be avoided.
  // Negative values mean pressure is unreported.
  float pressure;

  // Angle of the stylus to the screen in radians [0, PI/2]. A value of 0 means
  // the stylus is upright and a value of PI/2 means it is flat against the
  // screen. If no information is available, the value is 0.
  float tilt;
  // Orientation of the stylus in radians [0. 2*PI). This is the orientation of
  // the ray from the stylus tip to its end. A value of 0 is along the positive
  // x axis, increasing counter-clockwise.
  // If no information is available, the value is 0.
  float orientation;

  InputData();
  InputData(InputType type, uint32_t id, uint32_t flags);
  bool operator==(const InputData& other) const;
  bool operator!=(const InputData& other) const;

  void Set(Flag flag, bool on);
  bool Get(Flag flag) const;

  glm::vec2 DeltaWorld() const;
  glm::vec2 DeltaScreen() const;
  DurationS DeltaTime() const;

  std::string ToString() const;
  std::string ToStringExtended() const;

  // Creates a copy of "data," with flags manipulated to represent a cancel
  static InputData MakeCancel(const InputData& data);

  // Detects and fixes bad input.
  //
  // If the packet can be corrected, this attempts to change it in the minimum
  // possible way. If we detect an unrecoverable state the packet is instead
  // modified to be a cancel.
  //
  // Returns "true" if the corrected packet is valid and should be processed
  // Returns "false" if the packet could not be corrected and should be
  // discarded
  static bool CorrectPacket(InputData* data, const InputData* last_data);

  // Sets last* fields based on "lastData"
  static void SetLastPacketInfo(InputData* data, const InputData* last_data);
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_DATA_H_
