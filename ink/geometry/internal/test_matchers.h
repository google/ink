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

#ifndef INK_GEOMETRY_INTERNAL_TEST_MATCHERS_H_
#define INK_GEOMETRY_INTERNAL_TEST_MATCHERS_H_

#include <algorithm>
#include <vector>

#include "gmock/gmock.h"
#include "ink/geometry/internal/circle.h"
#include "ink/geometry/internal/outline_processing.h"

namespace ink::geometry_internal {

::testing::Matcher<Circle> CircleEq(const Circle& expected);
::testing::Matcher<Circle> CircleNear(const Circle& expected, float tolerance);

MATCHER_P(IsCyclicPermutationOf, expected, "") {
  // Two arrays are cyclic permutations of each other iff they are the same
  // size, and one is a subarray of the other concatenated with itself.
  if (arg.size() != expected.size()) return false;
  auto doubled = std::vector(expected.begin(), expected.end());
  doubled.insert(doubled.end(), expected.begin(), expected.end());
  return std::search(doubled.begin(), doubled.end(), arg.begin(), arg.end()) !=
         doubled.end();
}

::testing::Matcher<ShapeOutline> ShapeOutlineNear(const ShapeOutline& expected,
                                                  float tolerance);

}  // namespace ink::geometry_internal

#endif  // INK_GEOMETRY_INTERNAL_TEST_MATCHERS_H_
