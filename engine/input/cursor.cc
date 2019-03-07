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

#include "ink/engine/input/cursor.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace input {

namespace {
ink::proto::Cursor::CursorType CursorTypeToProto(CursorType type) {
  switch (type) {
    case CursorType::DEFAULT:
      return ink::proto::Cursor::DEFAULT;
    case CursorType::BRUSH:
      return ink::proto::Cursor::BRUSH;
    case CursorType::CROSSHAIR:
      return ink::proto::Cursor::CROSSHAIR;
    case CursorType::GRAB:
      return ink::proto::Cursor::GRAB;
    case CursorType::GRABBING:
      return ink::proto::Cursor::GRABBING;
    case CursorType::MOVE:
      return ink::proto::Cursor::MOVE;
    case CursorType::RESIZE_EW:
      return ink::proto::Cursor::RESIZE_EW;
    case CursorType::RESIZE_NS:
      return ink::proto::Cursor::RESIZE_NS;
    case CursorType::RESIZE_NESW:
      return ink::proto::Cursor::RESIZE_NESW;
    case CursorType::RESIZE_NWSE:
      return ink::proto::Cursor::RESIZE_NWSE;
    case CursorType::TEXT:
      return ink::proto::Cursor::TEXT;
  }
  SLOG(SLOG_WARNING, "invalid CursorType value: $0", static_cast<int>(type));
  return ink::proto::Cursor::DEFAULT;
}
}  // namespace

// static
void Cursor::WriteToProto(ink::proto::Cursor* proto_cursor,
                          const Cursor& obj_cursor) {
  proto_cursor->set_type(CursorTypeToProto(obj_cursor.type()));
  proto_cursor->set_brush_rgba(
      obj_cursor.brush_color().AsNonPremultipliedUintRGBA());
  proto_cursor->set_brush_size(obj_cursor.brush_size());
}

}  // namespace input
}  // namespace ink
