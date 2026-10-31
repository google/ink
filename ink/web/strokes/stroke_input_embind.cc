// Copyright 2026 Google LLC
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

#include <emscripten/bind.h>

#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_stroke_input_bindings) {
  emscripten::enum_<StrokeInput::ToolType>("ToolType")
      .value("UNKNOWN", StrokeInput::ToolType::kUnknown)
      .value("MOUSE", StrokeInput::ToolType::kMouse)
      .value("TOUCH", StrokeInput::ToolType::kTouch)
      .value("STYLUS", StrokeInput::ToolType::kStylus);

  emscripten::class_<StrokeInput>("StrokeInput")
      .constructor<>()
      .constructor(+[](Point position, Duration32 elapsed_time) {
        return StrokeInput{
            .position = position,
            .elapsed_time = elapsed_time,
        };
      })
      .constructor(+[](StrokeInput::ToolType tool_type, Point position,
                       Duration32 elapsed_time,
                       PhysicalDistance stroke_unit_length, float pressure,
                       Angle tilt, Angle orientation) {
        return StrokeInput{
            .tool_type = tool_type,
            .position = position,
            .elapsed_time = elapsed_time,
            .stroke_unit_length = stroke_unit_length,
            .pressure = pressure,
            .tilt = tilt,
            .orientation = orientation,
        };
      })
      .class_function(
          "create",
          +[](Point position, Duration32 elapsed_time) {
            return StrokeInput{
                .position = position,
                .elapsed_time = elapsed_time,
            };
          })
      .property("toolType", &StrokeInput::tool_type)
      .property("position", &StrokeInput::position)
      .property("elapsedTime", &StrokeInput::elapsed_time)
      .property("strokeUnitLength", &StrokeInput::stroke_unit_length)
      .property("pressure", &StrokeInput::pressure)
      .property("tilt", &StrokeInput::tilt)
      .property("orientation", &StrokeInput::orientation)
      .property("barrelTwist", &StrokeInput::barrel_twist)
      .function("hasStrokeUnitLength", &StrokeInput::HasStrokeUnitLength)
      .function("hasPressure", &StrokeInput::HasPressure)
      .function("hasTilt", &StrokeInput::HasTilt)
      .function("hasOrientation", &StrokeInput::HasOrientation)
      .function("hasBarrelTwist", &StrokeInput::HasBarrelTwist);
}

}  // namespace
}  // namespace ink::web
