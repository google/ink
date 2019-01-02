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

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_PRIMITIVE_TEST_HELPERS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_PRIMITIVE_TEST_HELPERS_H_

#include <vector>

#include "testing/base/public/gmock.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/geometry/primitives/triangle.h"

namespace ink {

// These matchers perform component-wise comparisons using ::testing::FloatEq.
::testing::Matcher<glm::vec2> Vec2Eq(glm::vec2 v);
::testing::Matcher<glm::vec3> Vec3Eq(glm::vec3 v);
::testing::Matcher<glm::vec4> Vec4Eq(glm::vec4 v);
::testing::Matcher<glm::mat4> Mat4Eq(glm::mat4 m);
::testing::Matcher<Segment> SegmentEq(Segment s);
::testing::Matcher<geometry::Triangle> TriangleEq(geometry::Triangle t);
::testing::Matcher<Rect> RectEq(Rect r);
::testing::Matcher<RotRect> RotRectEq(RotRect r);
::testing::Matcher<std::vector<glm::vec2>> PolylineEq(std::vector<glm::vec2> p);

// These matchers perform component-wise comparisons using ::testing::FloatNear.
::testing::Matcher<glm::vec2> Vec2Near(glm::vec2 v, float max_abs_error);
::testing::Matcher<glm::vec3> Vec3Near(glm::vec3 v, float max_abs_error);
::testing::Matcher<glm::vec4> Vec4Near(glm::vec4 v, float max_abs_error);
::testing::Matcher<glm::mat4> Mat4Near(glm::mat4 m, float max_abs_error);
::testing::Matcher<Segment> SegmentNear(Segment s, float max_abs_error);
::testing::Matcher<geometry::Triangle> TriangleNear(geometry::Triangle t,
                                                    float max_abs_error);
::testing::Matcher<Rect> RectNear(Rect r, float max_abs_error);
::testing::Matcher<RotRect> RotRectNear(RotRect r, float max_abs_error);
::testing::Matcher<std::vector<glm::vec2>> PolylineNear(
    std::vector<glm::vec2> p, float max_abs_error);

// These polygon matchers function similarly to PolylineEq() and PolylineNear(),
// but the also match if there exists a circular shift (go/wiki/Circular_shift)
// of the expected polygon that matches the given polygon.
::testing::Matcher<geometry::Polygon> PolygonEq(geometry::Polygon p);
::testing::Matcher<geometry::Polygon> PolygonEq(std::vector<glm::vec2> p);
::testing::Matcher<geometry::Polygon> PolygonNear(geometry::Polygon p,
                                                  float max_abs_error);
::testing::Matcher<geometry::Polygon> PolygonNear(std::vector<glm::vec2> p,
                                                  float max_abs_error);
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_PRIMITIVE_TEST_HELPERS_H_
