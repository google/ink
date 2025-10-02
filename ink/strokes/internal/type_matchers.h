// Copyright 2024 Google LLC
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

#ifndef INK_STROKES_INTERNAL_TYPE_MATCHERS_H_
#define INK_STROKES_INTERNAL_TYPE_MATCHERS_H_

#include "gtest/gtest.h"
#include "ink/strokes/internal/brush_tip_shape.h"
#include "ink/strokes/internal/brush_tip_state.h"
#include "ink/strokes/internal/modeled_stroke_input.h"

namespace ink::strokes_internal {

::testing::Matcher<BrushTipState> BrushTipStateEq(
    const BrushTipState& expected);
::testing::Matcher<BrushTipState> BrushTipStateNear(
    const BrushTipState& expected, float tolerance);

::testing::Matcher<BrushTipShape> BrushTipShapeEq(
    const BrushTipShape& expected);
::testing::Matcher<BrushTipShape> BrushTipShapeNear(
    const BrushTipShape& expected, float tolerance);

::testing::Matcher<ModeledStrokeInput> ModeledStrokeInputEq(
    const ModeledStrokeInput& expected);
::testing::Matcher<ModeledStrokeInput> ModeledStrokeInputNear(
    const ModeledStrokeInput& expected, float tolerance);

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_TYPE_MATCHERS_H_
