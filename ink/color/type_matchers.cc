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

#include "ink/color/type_matchers.h"

#include <array>
#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "ink/color/color.h"

namespace ink {
namespace {

using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Field;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Matcher;
using ::testing::Matches;

MATCHER_P(NearIdentityMatrixMatcher, eps,
          absl::StrCat(negation ? "isn't" : "is", " within ", eps,
                       " of the identity matrix")) {
  return Matches(ElementsAre(
      FloatNear(1.0, eps), FloatNear(0.0, eps), FloatNear(0.0, eps),
      FloatNear(0.0, eps), FloatNear(1.0, eps), FloatNear(0.0, eps),
      FloatNear(0.0, eps), FloatNear(0.0, eps), FloatNear(1.0, eps)))(arg);
}

MATCHER_P2(Vec4NearMatcher, expected, eps,
           absl::StrFormat("%s within %e of {%f, %f, %f, %f}",
                           negation ? "isn't" : "is", eps, expected[0],
                           expected[1], expected[2], expected[3])) {
  return Matches(ElementsAre(
      FloatNear(expected[0], eps), FloatNear(expected[1], eps),
      FloatNear(expected[2], eps), FloatNear(expected[3], eps)))(arg);
}

MATCHER_P(FloatNearlyBetweenZeroAndOneMatcher, eps,
          absl::StrFormat("%s within %e of the interval [0, 1]",
                          negation ? "isn't" : "is", eps)) {
  return Matches(AllOf(Ge(-eps), Le(1.0f + eps)))(arg);
}

MATCHER_P(ColorNearlyEqualsMatcher, expected, "") {
  return arg.NearlyEquals(expected);
}

MATCHER_P2(RgbaFloatNearMatcher, expected, tolerance,
           absl::StrCat(negation ? "isn't" : "is", " within ", tolerance,
                        " of ", expected)) {
  return ExplainMatchResult(
      AllOf(Field("r", &Color::RgbaFloat::r, FloatNear(expected.r, tolerance)),
            Field("g", &Color::RgbaFloat::g, FloatNear(expected.g, tolerance)),
            Field("b", &Color::RgbaFloat::b, FloatNear(expected.b, tolerance)),
            Field("a", &Color::RgbaFloat::a, FloatNear(expected.a, tolerance))),
      arg, result_listener);
}

MATCHER_P(RgbaFloatEqMatcher, expected,
          absl::StrCat(negation ? "isn't " : "is ", expected)) {
  return ExplainMatchResult(
      AllOf(Field("r", &Color::RgbaFloat::r, FloatEq(expected.r)),
            Field("g", &Color::RgbaFloat::g, FloatEq(expected.g)),
            Field("b", &Color::RgbaFloat::b, FloatEq(expected.b)),
            Field("a", &Color::RgbaFloat::a, FloatEq(expected.a))),
      arg, result_listener);
}

MATCHER_P(RgbaUint8EqMatcher, expected,
          absl::StrCat(negation ? "isn't " : "is ", expected)) {
  return ExplainMatchResult(
      AllOf(Field("r", &Color::RgbaUint8::r, Eq(expected.r)),
            Field("g", &Color::RgbaUint8::g, Eq(expected.g)),
            Field("b", &Color::RgbaUint8::b, Eq(expected.b)),
            Field("a", &Color::RgbaUint8::a, Eq(expected.a))),
      arg, result_listener);
}

MATCHER_P2(OklabFloatNearMatcher, expected, tolerance,
           absl::StrCat(negation ? "isn't" : "is", " within ", tolerance,
                        " of ", expected)) {
  return ExplainMatchResult(AllOf(Field("ok_L", &Color::OklabFloat::ok_L,
                                        FloatNear(expected.ok_L, tolerance)),
                                  Field("ok_a", &Color::OklabFloat::ok_a,
                                        FloatNear(expected.ok_a, tolerance)),
                                  Field("ok_b", &Color::OklabFloat::ok_b,
                                        FloatNear(expected.ok_b, tolerance)),
                                  Field("alpha", &Color::OklabFloat::alpha,
                                        FloatNear(expected.alpha, tolerance))),
                            arg, result_listener);
}

}  // namespace

Matcher<std::array<float, 9>> NearIdentityMatrix(double eps) {
  return NearIdentityMatrixMatcher(eps);
}

Matcher<std::array<float, 4>> Vec4Near(const std::array<float, 4>& expected,
                                       float eps) {
  return Vec4NearMatcher(expected, eps);
}

Matcher<float> FloatNearlyBetweenZeroAndOne(float eps) {
  return FloatNearlyBetweenZeroAndOneMatcher(eps);
}

Matcher<Color> ColorNearlyEquals(const Color& expected) {
  return ColorNearlyEqualsMatcher(expected);
}

Matcher<Color::RgbaFloat> RgbaFloatNear(Color::RgbaFloat expected,
                                        float tolerance) {
  return RgbaFloatNearMatcher(expected, tolerance);
}

Matcher<Color::RgbaFloat> RgbaFloatEq(Color::RgbaFloat expected) {
  return RgbaFloatEqMatcher(expected);
}

Matcher<Color::RgbaUint8> RgbaUint8Eq(Color::RgbaUint8 expected) {
  return RgbaUint8EqMatcher(expected);
}

Matcher<Color::OklabFloat> OklabFloatNear(Color::OklabFloat expected,
                                          float tolerance) {
  return OklabFloatNearMatcher(expected, tolerance);
}

}  // namespace ink
