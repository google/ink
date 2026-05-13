// Copyright 2024-2026 Google LLC
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

#include "ink/strokes/internal/jni/stroke_input_native_helper.h"

#include "ink/strokes/input/stroke_input.h"

namespace ink::native {

// Convert an int to an StrokeInput::ToolType enum.
//
// This should match the enum in InputToolType.kt.
StrokeInput::ToolType IntToToolType(int val) {
  return static_cast<StrokeInput::ToolType>(val);
}

int ToolTypeToInt(StrokeInput::ToolType type) {
  switch (type) {
    case StrokeInput::ToolType::kMouse:
      return 1;
    case StrokeInput::ToolType::kTouch:
      return 2;
    case StrokeInput::ToolType::kStylus:
      return 3;
    default:
      return 0;
  }
}

}  // namespace ink::native
