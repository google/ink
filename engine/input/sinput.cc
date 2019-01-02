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

#include "ink/engine/input/sinput.h"

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

SInput::SInput() {}

SInput::SInput(const input::InputData& data, const Camera& cam)
    : type(data.type),
      id(data.id),
      flags(data.flags),
      time_s(data.time),
      screen_pos(data.screen_pos),
      pressure(data.pressure),
      tilt(data.tilt),
      orientation(data.orientation),
      wheel_delta_x(data.wheel_delta_x),
      wheel_delta_y(data.wheel_delta_y) {
  screen_pos.y = cam.ScreenDim().y - screen_pos.y;
}

bool SInput::operator==(const SInput& other) const {
  return type == other.type && id == other.id && flags == other.flags &&
         time_s == other.time_s && screen_pos == other.screen_pos &&
         pressure == other.pressure && wheel_delta_x == other.wheel_delta_x &&
         wheel_delta_y == other.wheel_delta_y;
}

std::string SInput::ToString() const {
  return Substitute(
      "id: $0, flags: $1, screenPos: $2, pressure: $3, "
      "time: ($4)",
      id, input::StringFlags(flags), screen_pos, pressure, time_s);
}

// static
SInput SInput::Lerpnc(SInput from, SInput to, float amount) {
  ASSERT(from.type == to.type);
  ASSERT(from.id == to.id);
  SInput res;
  res.screen_pos = util::Lerpnc(from.screen_pos, to.screen_pos, amount);
  res.time_s = util::Lerpnc(from.time_s, to.time_s, amount);
  res.pressure = util::Lerpnc(from.pressure, to.pressure, amount);
  res.wheel_delta_x =
      util::Lerpnc(from.wheel_delta_x, to.wheel_delta_x, amount);
  res.wheel_delta_y =
      util::Lerpnc(from.wheel_delta_y, to.wheel_delta_y, amount);
  res.type = from.type;
  res.id = from.id;
  res.flags = from.flags;
  return res;
}

// static
Status SInput::ReadFromProto(const proto::SInput& unsafe_proto, SInput* res) {
  *res = SInput();
  switch (unsafe_proto.type()) {
    case proto::SInput::MOUSE:
      res->type = input::InputType::Mouse;
      break;
    case proto::SInput::TOUCH:
      res->type = input::InputType::Touch;
      break;
    case proto::SInput::PEN:
      res->type = input::InputType::Pen;
      break;
    case proto::SInput::ERASER:
      res->type = input::InputType::Pen;
      SLOG(SLOG_ERROR, "Eraser not implemented; using Pen.");
      break;
    default:
      res->type = input::InputType::Invalid;
      return status::InvalidArgument("SInput must specify an input type");
  }
  res->id = unsafe_proto.id();
  res->flags = unsafe_proto.flags();
  res->time_s = InputTimeS(unsafe_proto.time_s());
  if (res->time_s < InputTimeS(0)) {
    return status::InvalidArgument("SInput must specify time >= 0");
  }
  res->screen_pos.x = unsafe_proto.screen_pos_x();
  res->screen_pos.y = unsafe_proto.screen_pos_y();
  res->pressure = unsafe_proto.pressure();
  res->tilt = unsafe_proto.tilt();
  res->orientation = unsafe_proto.orientation();
  res->wheel_delta_x = unsafe_proto.wheel_delta_x();
  res->wheel_delta_y = unsafe_proto.wheel_delta_y();
  return OkStatus();
}

// static
void SInput::WriteToProto(ink::proto::SInput* proto_sinput,
                          const SInput& obj_sinput) {
  *proto_sinput = proto::SInput();
  switch (obj_sinput.type) {
    case input::InputType::Mouse:
      proto_sinput->set_type(proto::SInput::MOUSE);
      break;
    case input::InputType::Touch:
      proto_sinput->set_type(proto::SInput::TOUCH);
      break;
    case input::InputType::Pen:
      proto_sinput->set_type(proto::SInput::PEN);
      break;
    default:
      RUNTIME_ERROR("SInput must specify an input type");
      break;
  }
  proto_sinput->set_id(obj_sinput.id);
  proto_sinput->set_flags(obj_sinput.flags);
  proto_sinput->set_time_s(static_cast<double>(obj_sinput.time_s));
  proto_sinput->set_screen_pos_x(obj_sinput.screen_pos.x);
  proto_sinput->set_screen_pos_y(obj_sinput.screen_pos.y);
  proto_sinput->set_pressure(obj_sinput.pressure);
  proto_sinput->set_tilt(obj_sinput.tilt);
  proto_sinput->set_orientation(obj_sinput.orientation);
  proto_sinput->set_wheel_delta_x(obj_sinput.wheel_delta_x);
  proto_sinput->set_wheel_delta_y(obj_sinput.wheel_delta_y);
}

// static
bool SInput::LessThan(const SInput& lhs, const SInput& rhs) {
  if (lhs.time_s < rhs.time_s)
    return true;
  else if (lhs.time_s == rhs.time_s)
    return lhs.id < rhs.id;
  return false;
}

namespace util {
template <>
SInput Lerpnc(SInput from, SInput to, float amount) {
  return SInput::Lerpnc(from, to, amount);
}
}  // namespace util

}  // namespace ink
