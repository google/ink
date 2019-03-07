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

#ifndef INK_ENGINE_INPUT_CURSOR_H_
#define INK_ENGINE_INPUT_CURSOR_H_

#include "ink/engine/public/types/color.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace input {

enum class CursorType {
  DEFAULT,
  BRUSH,
  CROSSHAIR,
  GRAB,
  GRABBING,
  MOVE,
  RESIZE_EW,
  RESIZE_NS,
  RESIZE_NESW,
  RESIZE_NWSE,
  TEXT,
};

class Cursor {
 public:
  Cursor() : Cursor(CursorType::DEFAULT) {}

  explicit Cursor(CursorType type) : type_(type), brush_size_(0.0) {}

  Cursor(CursorType type, Color color, float size)
      : type_(type), brush_color_(color), brush_size_(size) {}

  Cursor(const Cursor& cursor) = default;
  Cursor& operator=(const Cursor& cursor) = default;

  CursorType type() const { return type_; }
  Color brush_color() const { return brush_color_; }
  float brush_size() const { return brush_size_; }

  inline bool operator==(const Cursor& other) const {
    return type_ == other.type_ && brush_color_ == other.brush_color_ &&
           brush_size_ == other.brush_size_;
  }
  inline bool operator!=(const Cursor& other) const {
    return !(*this == other);
  }

  static void WriteToProto(ink::proto::Cursor* proto_cursor,
                           const Cursor& obj_cursor);

 private:
  CursorType type_;
  Color brush_color_;
  float brush_size_;
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_CURSOR_H_
