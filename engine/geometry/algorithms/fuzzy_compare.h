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

// Approximate comparisons between geometric objects.

#ifndef INK_ENGINE_GEOMETRY_ALGORITHMS_FUZZY_COMAPRE_H_
#define INK_ENGINE_GEOMETRY_ALGORITHMS_FUZZY_COMAPRE_H_

#include "ink/engine/geometry/primitives/rect.h"

namespace ink {
namespace geometry {

// Determines whether two rectangles are equivalent based on their
// degree of overlap.
//
// Returns true iff the difference between 1.0 and the proportion of
// the area of rect1 (or rect2) that is accounted for by the area of
// the overlap between rect1 and rect2 does not exceed tolerance.
//
// A tolerance of 1.0 will cause disjoint rectangles to be considered
// equivalent.
//
// At default tolerance, 99.9% of the area of rect1 and of rect2 needs
// to be accounted for by the area of their intersection.
bool Equivalent(const Rect &rect1, const Rect &rect2, float tolerance = 0.001);

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_ALGORITHMS_FUZZY_COMAPRE_H_
