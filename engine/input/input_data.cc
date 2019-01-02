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

#include "ink/engine/input/input_data.h"

#include <sstream>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace input {

InputData::InputData() : InputData(InputType::Invalid, 0, 0) {}

InputData::InputData(InputType type, uint32_t id, uint32_t flags)
    : type(type),
      id(id),
      flags(flags),
      n_down(0),
      world_pos(glm::vec2(0)),
      last_world_pos(glm::vec2(0)),
      screen_pos(glm::vec2(0)),
      last_screen_pos(glm::vec2(0)),
      time(0),
      last_time(0),
      wheel_delta_x(0),
      wheel_delta_y(0),
      pressure(0),
      tilt(0),
      orientation(0) {}

void InputData::Set(Flag flag, bool on) {
  auto bit = static_cast<uint32_t>(flag);
  if (on) {
    flags = flags | bit;
  } else {
    flags = flags & ~bit;
  }
}

bool InputData::Get(Flag flag) const {
  return (flags & static_cast<uint32_t>(flag)) != 0;
}

glm::vec2 InputData::DeltaWorld() const { return world_pos - last_world_pos; }

glm::vec2 InputData::DeltaScreen() const {
  return screen_pos - last_screen_pos;
}

DurationS InputData::DeltaTime() const { return time - last_time; }

std::string InputData::ToString() const {
  return Substitute("id: $0, flags: $1", id, StringFlags(flags));
}

std::string InputData::ToStringExtended() const {
  return Substitute(
      "id: $0, flags: $1, screenPos: $2, worldPos: $3, pressure: $4, "
      "time: ($5), type: $6, wheel dx/dy: $7/$8",
      id, StringFlags(flags), screen_pos, world_pos, pressure, time,
      InputTypeString(type), static_cast<float>(wheel_delta_x),
      static_cast<float>(wheel_delta_y));
}

bool InputData::operator==(const InputData& other) const {
  bool res = (type == other.type && id == other.id && flags == other.flags &&
              screen_pos == other.screen_pos && time == other.time);

  return res;
}

bool InputData::operator!=(const InputData& other) const {
  return !(*this == other);
}

// static
InputData InputData::MakeCancel(const InputData& data) {
  InputData cancel = data;
  if (data.Get(Flag::InContact) || data.Get(Flag::TUp)) {
    cancel.flags = 0;
    cancel.Set(Flag::TUp, true);
    cancel.Set(Flag::Cancel, true);
  }
  return cancel;
}

std::string StringFlags(uint32_t flags) {
  std::stringstream buf;
  bool add_sep = false;

  auto add = [&buf, &add_sep, flags](Flag flag, const char* name) {
    if ((flags & static_cast<uint32_t>(flag)) != 0) {
      if (add_sep) buf << '|';
      add_sep = true;
      buf << name;
    }
  };

  add(Flag::Shift, "Shift");
  add(Flag::Control, "Control");
  add(Flag::Alt, "Alt");
  add(Flag::Meta, "Meta");
  add(Flag::InContact, "InContact");
  add(Flag::Left, "Left");
  add(Flag::Right, "Right");
  add(Flag::TDown, "TDown");
  add(Flag::TUp, "TUp");
  add(Flag::Wheel, "Wheel");
  add(Flag::Cancel, "Cancel");
  add(Flag::Primary, "Primary");

  return buf.str();
}

namespace {
void CorrectPersistentFlag(input::Flag flag, std::string flag_name,
                           InputData* data, const InputData* last_data) {
  if (data->Get(flag) && !last_data->Get(flag)) {
    SLOG(SLOG_INPUT_CORRECTION, "Flag $0 added midstream. data: $1, last: $2",
         flag_name, *data, *last_data);
    data->Set(flag, false);
  }

  if (!data->Get(flag) && last_data->Get(flag)) {
    SLOG(SLOG_INPUT_CORRECTION, "Flag $0 not persisted. data: $1, last: $2",
         flag_name, *data, *last_data);
    data->Set(flag, true);
  }
}
}  // namespace

// static
bool InputData::CorrectPacket(InputData* data, const InputData* last_data) {
  auto odata = *data;
  bool should_attempt_cancel = false;

  ASSERT(data);

  if (last_data && last_data->Get(Flag::InContact)) {
    // last was in contact

    if (data->Get(Flag::TDown)) {
      SLOG(SLOG_INPUT_CORRECTION, "Duplicate down detected. data: $0, last: $1",
           *data, *last_data);
      data->Set(Flag::TDown, false);
    }

    CorrectPersistentFlag(Flag::Right, "Right", data, last_data);
    CorrectPersistentFlag(Flag::Left, "Left", data, last_data);
    CorrectPersistentFlag(Flag::Eraser, "Eraser", data, last_data);

    if (data->Get(Flag::InContact) && data->Get(Flag::TUp)) {
      SLOG(SLOG_INPUT_CORRECTION,
           "InContact and Up set. (Should be either/or) data: $0, last: $1",
           *data, *last_data);
      data->Set(Flag::InContact, false);
    }

    if (!data->Get(Flag::InContact) && !data->Get(Flag::TUp)) {
      SLOG(SLOG_INPUT_CORRECTION, "Missing up. data: $0, last: $1", *data,
           *last_data);
      data->Set(Flag::TUp, true);
    }
  } else {
    // last was not in contact

    if (data->Get(Flag::TUp)) {
      SLOG(SLOG_INPUT_CORRECTION, "Up without prior data. data: $0", *data);

      // we can't generate a down for this to make sense
      should_attempt_cancel = true;
    }

    if (data->Get(Flag::InContact) && !data->Get(Flag::TDown)) {
      SLOG(SLOG_INPUT_CORRECTION, "Missing down. data: $0", *data);
      data->Set(Flag::TDown, true);
    }
  }

  if (data->Get(Flag::TDown) && !data->Get(Flag::InContact)) {
    SLOG(SLOG_INPUT_CORRECTION, "Down not in contact. data: $0", *data);
    data->Set(Flag::InContact, true);
  }

  if (data->Get(Flag::TUp) && data->Get(Flag::InContact)) {
    SLOG(SLOG_INPUT_CORRECTION, "Up in contact. data: $0", *data);
    data->Set(Flag::InContact, false);
  }

  if (last_data && data->time < last_data->time) {
    SLOG(SLOG_INPUT_CORRECTION, "Out of order input. data: $0, lastData: $1",
         *data, *last_data);
    data->time = last_data->time;
  }

  /////////////////////////////////////////////////////////////////
  // At this point we've finished inspecting/correcting the packet.
  // Report the result
  /////////////////////////////////////////////////////////////////

  bool should_send_packet = true;
  if (should_attempt_cancel) {
    if (last_data && last_data->Get(Flag::InContact)) {
      *data = InputData::MakeCancel(*data);
      SLOG(SLOG_ERROR,
           "Cancelling input due to bad packet. Original: $0, corrected: $1",
           odata, *data);
    } else {
      // Can't cancel an input stream that doesn't exist
      // Just don't send the packet
      should_send_packet = false;
      SLOG(SLOG_ERROR, "Couldn't correct or cancel packet $0. Ignoring", *data);
    }
  }

  if (*data != odata) {
    SLOG(SLOG_INPUT_CORRECTION,
         "Packet had to be corrected! original: $0, corrected: $1", odata,
         *data);
  }

  // This needs to be checked after all packet modifications
  if (last_data && *last_data == *data) {
    SLOG(SLOG_INPUT_CORRECTION, "Duplicate packet, data: $0, lastData: $1",
         *data, *last_data);
    should_send_packet = false;
  }

  return should_send_packet;
}

// static
void InputData::SetLastPacketInfo(InputData* data, const InputData* last_data) {
  if (last_data) {
    data->last_screen_pos = last_data->screen_pos;
    data->last_world_pos = last_data->world_pos;
    data->last_time = last_data->time;
    data->Set(Flag::Primary, last_data->Get(Flag::Primary));
  } else {
    data->last_screen_pos = data->screen_pos;
    data->last_world_pos = data->world_pos;
    data->last_time = data->time;
  }
}

}  // namespace input
}  // namespace ink
