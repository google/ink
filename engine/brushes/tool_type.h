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

#ifndef INK_ENGINE_BRUSHES_TOOL_TYPE_H_
#define INK_ENGINE_BRUSHES_TOOL_TYPE_H_

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

namespace Tools {

// The int values here MUST be kept in sync with sengine.proto's ToolType list.
enum ToolType {
  MinTool = 0,
  Line = 1,
  Edit = 2,
  MagicEraser = 3,
  Query = 4,
  NoTool = 5,
  FilterChooser = 6,
  Pusher = 7,
  Crop = 8,
  TextHighlighterTool = 9,
  StrokeEditingEraser = 10,
  SmartHighlighterTool = 11,
  MaxTool = 12,
};

}  // namespace Tools

template <>
inline std::string Str(const Tools::ToolType& t) {
  switch (t) {
    case Tools::ToolType::MinTool:
      return "MinTool";
    case Tools::ToolType::NoTool:
      return "NoTool";
    case Tools::ToolType::Edit:
      return "Edit";
    case Tools::ToolType::MagicEraser:
      return "MagicEraser";
    case Tools::ToolType::Line:
      return "Line";
    case Tools::ToolType::FilterChooser:
      return "FilterChooser";
    case Tools::ToolType::Pusher:
      return "Pusher";
    case Tools::ToolType::MaxTool:
      return "MaxTool";
    case Tools::ToolType::TextHighlighterTool:
      return "TextHighlighterTool";
    case Tools::ToolType::StrokeEditingEraser:
      return "StrokeEditingEraser";
    case Tools::ToolType::SmartHighlighterTool:
      return "SmartHighlighterTool";
    default:
      RUNTIME_ERROR("Str not implemented for ToolType $0", static_cast<int>(t));
      return "Unknown";
  }
}

}  // namespace ink
#endif  // INK_ENGINE_BRUSHES_TOOL_TYPE_H_
