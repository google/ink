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

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_TEST_HELPERS_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_TEST_HELPERS_H_

#include "testing/base/public/gmock.h"  // MATCHER_Pn
#include "ink/engine/geometry/primitives/primitive_test_helpers.h"
#include "ink/engine/scene/page/page_info.h"

// Defines some common helper functions for page layout tests.
namespace ink {
namespace testing {

// Create a PageSpec object to the given dimensions. Doesn't set the
// group id.
inline PageSpec MakeOriginalPage(float width, float height) {
  // Ignore the group id.
  PageSpec page_info;
  page_info.dimensions = glm::vec2(width, height);
  return page_info;
}

// Ensures that the bounds returned for a transformed page info object
// are the bounds represented by the matcher. Also validates the transform
// is for a Rect from 0,0 -> page dimensions to the new bounds.
MATCHER_P4(TransformedPageApproxEq, x, y, x2, y2,
           string(::testing::PrintToString(Rect(x, y, x2, y2)))) {
  // Ignore the group id.
  auto bounds = Rect(glm::vec2(x, y), glm::vec2(x2, y2));
  auto actual = Rect(glm::vec2(0, 0), bounds.Dim());
  auto xform = actual.CalcTransformTo(bounds);
  return ::testing::Matches(RectEq(bounds))(arg.bounds) &&
         ::testing::Matches(Mat4Eq(xform))(arg.transform);
}

}  // namespace testing
}  // namespace ink

#endif  // INK_ENGINE_SCENE_PAGE_PAGE_LAYOUT_TEST_HELPERS_H_
