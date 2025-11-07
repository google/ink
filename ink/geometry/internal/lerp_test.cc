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

#include "ink/geometry/internal/lerp.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "ink/color/color.h"
#include "ink/color/type_matchers.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/point.h"
#include "ink/geometry/type_matchers.h"
#include "ink/geometry/vec.h"
#include "ink/types/duration.h"
#include "ink/types/type_matchers.h"

namespace ink {
namespace geometry_internal {
namespace {

using ::fuzztest::Filter;
using ::fuzztest::Finite;
using ::fuzztest::InRange;
using ::fuzztest::PairOf;
using ::testing::IsNan;

constexpr float kInfinity = std::numeric_limits<float>::infinity();

TEST(FloatLerpTest, AmountBetweenZeroAndOne) {
  EXPECT_FLOAT_EQ(Lerp(100.0, 200.0, 0.1), 110.0);
}

TEST(FloatLerpTest, AmountGreaterThanOne) {
  EXPECT_FLOAT_EQ(Lerp(100.0, 200.0, 1.1), 210.0);
}

TEST(FloatLerpTest, AmountLessThanZero) {
  EXPECT_FLOAT_EQ(Lerp(100.0, 200.0, -0.1), 90.0);
}

TEST(FloatLerpTest, ExtremeEndpoints) {
  float min_float = -std::numeric_limits<float>::max();
  float max_float = std::numeric_limits<float>::max();
  EXPECT_FLOAT_EQ(Lerp(min_float, max_float, 0), min_float);
  EXPECT_FLOAT_EQ(Lerp(min_float, max_float, 1), max_float);
  EXPECT_FLOAT_EQ(Lerp(min_float, max_float, 0.5), 0);
}

// If a and b are finite, then t=0 should map to exactly a, and t=1 should map
// to exactly b.
void LerpFloatReturnsExactEndpoints(float a, float b) {
  EXPECT_EQ(Lerp(a, b, 0), a);
  EXPECT_EQ(Lerp(a, b, 1), b);
}
FUZZ_TEST(FloatLerpTest, LerpFloatReturnsExactEndpoints)
    .WithDomains(Finite<float>(), Finite<float>());

// If a and b are finite and t is in [0, 1], the result should be finite (no
// float overflow) and in [a, b]
void LerpFloatFiniteInterpolation(float a, float b, float t) {
  float result = Lerp(a, b, t);
  EXPECT_TRUE(std::isfinite(result));
  EXPECT_GE(result, std::min(a, b));
  EXPECT_LE(result, std::max(a, b));
}
FUZZ_TEST(FloatLerpTest, LerpFloatFiniteInterpolation)
    .WithDomains(Finite<float>(), Finite<float>(), InRange<float>(0, 1));

// If a = b and both are finite, the result should be exactly a for all finite
// t.
void LerpFloatEqualEndpointsFiniteT(float a, float t) {
  EXPECT_EQ(Lerp(a, a, t), a);
}
FUZZ_TEST(FloatLerpTest, LerpFloatEqualEndpointsFiniteT)
    .WithDomains(Finite<float>(), Finite<float>());

// If a = b and both are finite, the result should be NaN for all infinite t.
void LerpFloatEqualEndpointsInfiniteT(float a) {
  EXPECT_THAT(Lerp(a, a, kInfinity), IsNan());
  EXPECT_THAT(Lerp(a, a, -kInfinity), IsNan());
}
FUZZ_TEST(FloatLerpTest, LerpFloatEqualEndpointsInfiniteT)
    .WithDomains(Finite<float>());

// If a != b, both are finite, and t is infinite, the result should be infinite
// (not NaN).
void LerpFloatUnequalEndpointsInfiniteT(std::pair<float, float> a_b) {
  float a = a_b.first;
  float b = a_b.second;
  float expected = a < b ? kInfinity : -kInfinity;
  EXPECT_EQ(Lerp(a, b, kInfinity), expected);
  EXPECT_EQ(Lerp(a, b, -kInfinity), -expected);
}
FUZZ_TEST(FloatLerpTest, LerpFloatUnequalEndpointsInfiniteT)
    .WithDomains(Filter(
        [](std::pair<float, float> a_b) {
          return a_b.second - a_b.first != 0.0f;
        },
        PairOf(Finite<float>(), Finite<float>())));

TEST(InverseLerpTest, InverseLerp) {
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 200.0, 140.0), 0.4);
}

TEST(InverseLerpTest, InverseLerpOnZeroWidthInterval) {
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 100.0, 140.0), 0.f);
}

TEST(InverseLerpTest, InverseLerpIsInverseOfLerp) {
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 200, Lerp(100, 200, 0.1)), 0.1);
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 200, Lerp(100, 200, 1.1)), 1.1);
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 200, Lerp(100, 200, -0.1)), -0.1);

  // For a zero-width interval Lerp cannot be be inversed because the original
  // `t` doesn't impact the result of the Lerp function.
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 100, Lerp(100, 100, 0.1)), 0.0f);
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 100, Lerp(100, 100, 1.1)), 0.0f);
  EXPECT_FLOAT_EQ(InverseLerp(100.0, 100, Lerp(100, 100, -0.1)), 0.0f);
}

TEST(LinearMapTest, LinearMap) {
  EXPECT_FLOAT_EQ(LinearMap(10, {0, 100}, {0, 200}), 20);
  EXPECT_FLOAT_EQ(LinearMap(10, {0, 100}, {100, 150}), 105);
  EXPECT_FLOAT_EQ(LinearMap(10, {0, 100}, {0, -100}), -10);
}

TEST(LinearMapTest, LinearMapOnZeroWidthInputInterval) {
  EXPECT_FLOAT_EQ(LinearMap(0, {0, 0}, {0, 100}), 0);
  EXPECT_FLOAT_EQ(LinearMap(150, {150, 150}, {0, 100}), 0);
}

TEST(LinearMapTest, LinearMapOnZeroWidthTargetInterval) {
  EXPECT_FLOAT_EQ(LinearMap(0, {0, 100}, {100, 100}), 100);
  EXPECT_FLOAT_EQ(LinearMap(50, {0, 100}, {100, 100}), 100);
}

TEST(LinearMapTest, LinearMapWithValueOutsideRange) {
  EXPECT_FLOAT_EQ(LinearMap(-10, {0, 100}, {0, 200}), -20);
}

TEST(LinearMapTest, LinearMapLinearMapofLinearMapIsOriginalValue) {
  EXPECT_FLOAT_EQ(
      LinearMap(LinearMap(10, {0, 100}, {0, 200}), {0, 200}, {0, 100}), 10);
}

TEST(PointLerpTest, AmountBetweenZeroAndOne) {
  EXPECT_THAT(Lerp(Point({100.0, 100.0}), Point({200.0, 200.0}), 0.1),
              PointEq(Point({110.0, 110.0})));
}

TEST(PointLerpTest, AmountGreaterThanOne) {
  EXPECT_THAT(Lerp(Point({100.0, 100.0}), Point({200.0, 200.0}), 1.1),
              PointEq(Point({210.0, 210.0})));
}

TEST(PointLerpTest, AmountLessThanZero) {
  EXPECT_THAT(Lerp(Point({100.0, 100.0}), Point({200.0, 200.0}), -0.1),
              PointEq(Point({90.0, 90.0})));
}

TEST(ColorRgbaFloatLerpTest, AmountBetweenZeroAndOne) {
  EXPECT_THAT(Lerp(Color::RgbaFloat({100.0, 100.0, 100.0, 100.0}),
                   Color::RgbaFloat({200.0, 200.0, 200.0, 200.0}), 0.1),
              ChannelStructEqFloats({110.0, 110.0, 110.0, 110.0}));
}

TEST(ColorRgbaFloatLerpTest, AmountGreaterThanOne) {
  EXPECT_THAT(Lerp(Color::RgbaFloat({100.0, 100.0, 100.0, 100.0}),
                   Color::RgbaFloat({200.0, 200.0, 200.0, 200.0}), 1.1),
              ChannelStructEqFloats({210.0, 210.0, 210.0, 210.0}));
}

TEST(ColorRgbaFloatLerpTest, AmountLessThanZero) {
  EXPECT_THAT(Lerp(Color::RgbaFloat({100.0, 100.0, 100.0, 100.0}),
                   Color::RgbaFloat({200.0, 200.0, 200.0, 200.0}), -0.1),
              ChannelStructEqFloats({90.0, 90.0, 90.0, 90.0}));
}

TEST(AngleLerpTest, AmountBetweenZeroAndOne) {
  EXPECT_THAT(Lerp(Angle::Radians(1), Angle::Radians(2), 0.3),
              AngleEq(Angle::Radians(1.3)));
}

TEST(AngleLerpTest, AmountLessThanZero) {
  EXPECT_THAT(Lerp(Angle::Radians(0.5), Angle::Radians(1.5), -2),
              AngleEq(Angle::Radians(-1.5)));
}

TEST(AngleLerpTest, AmountGreaterThanOne) {
  EXPECT_THAT(Lerp(Angle::Radians(-0.3), Angle::Radians(0.7), 5),
              AngleEq(Angle::Radians(4.7)));
}

TEST(AngleLerpTest, DifferenceGreaterThanTwoPi) {
  EXPECT_THAT(Lerp(Angle::Radians(-6), Angle::Radians(4), 0.7),
              AngleEq(Angle::Radians(1)));
}

TEST(AngleLerpTest, ExtremeEndpoints) {
  Angle min_angle = Angle::Radians(-std::numeric_limits<float>::max());
  Angle max_angle = Angle::Radians(std::numeric_limits<float>::max());
  EXPECT_THAT(Lerp(min_angle, max_angle, 0), AngleEq(min_angle));
  EXPECT_THAT(Lerp(min_angle, max_angle, 1), AngleEq(max_angle));
  EXPECT_THAT(Lerp(min_angle, max_angle, 0.5), AngleEq(Angle()));
}

TEST(NormalizedAngleLerpTest, DifferenceSmallerThanPiWithBGreaterA) {
  EXPECT_THAT(NormalizedAngleLerp(Angle::Radians(1), Angle::Radians(2), 0.3),
              AngleEq(Angle::Radians(1.3)));
}

TEST(NormalizedAngleLerpTest, DifferenceGreaterThanPiWithBGreaterA) {
  EXPECT_THAT(
      NormalizedAngleLerp(Angle::Radians(0.5), Angle::Radians(5.5), 0.3),
      AngleNear(Angle::Radians(0.116), 0.01));
}

TEST(NormalizedAngleLerpTest, DifferenceSmallerThanPiWithAGreaterB) {
  EXPECT_THAT(
      NormalizedAngleLerp(Angle::Radians(5.5), Angle::Radians(0.5), 0.3),
      AngleNear(Angle::Radians(5.884), 0.01));
}

TEST(NormalizedAngleLerpTest, DifferenceGreaterThanPiWithAGreaterB) {
  EXPECT_THAT(
      NormalizedAngleLerp(Angle::Radians(5.5), Angle::Radians(4.5), 0.3),
      AngleEq(Angle::Radians(5.2)));
}

TEST(DurationLerpTest, AmountBetweenZeroAndOne) {
  EXPECT_THAT(Lerp(Duration32::Millis(100), Duration32::Millis(200), 0.25),
              Duration32Eq(Duration32::Millis(125)));
}

TEST(DurationLerpTest, AmountLessThanZero) {
  EXPECT_THAT(Lerp(Duration32::Millis(-100), Duration32::Millis(200), -1),
              Duration32Eq(Duration32::Millis(-400)));
}

TEST(DurationLerpTest, AmountGreaterThanOne) {
  EXPECT_THAT(Lerp(Duration32::Millis(100), Duration32::Millis(50), 1.5),
              Duration32Eq(Duration32::Millis(25)));
}

TEST(DurationLerpTest, ExtremeEndpoints) {
  Duration32 min_duration =
      Duration32::Seconds(-std::numeric_limits<float>::max());
  Duration32 max_duration =
      Duration32::Seconds(std::numeric_limits<float>::max());
  EXPECT_THAT(Lerp(min_duration, max_duration, 0), Duration32Eq(min_duration));
  EXPECT_THAT(Lerp(min_duration, max_duration, 1), Duration32Eq(max_duration));
  EXPECT_THAT(Lerp(min_duration, max_duration, 0.5),
              Duration32Eq(Duration32::Zero()));
}

TEST(VecLerpTest, NonZeroInputVectorsWithDifferentDirections) {
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, -5), VecEq({24, -25}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, -4), VecEq({20, -20}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, -1), VecEq({8, -5}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, 0), VecEq({4, 0}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, 0.5), VecEq({2, 2.5}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, 1), VecEq({0, 5}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, 5}, 3), VecEq({-8, 15}));

  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, -1), VecEq({8, 3}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, 0), VecEq({4, 0}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, 0.5), VecEq({2, -1.5}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, 1), VecEq({0, -3}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, 4), VecEq({-12, -12}));
  EXPECT_THAT(Lerp(Vec{4, 0}, Vec{0, -3}, 5), VecEq({-16, -15}));
}

TEST(VecLerpTest, NonZeroInputVectorsWithSameDirection) {
  EXPECT_THAT(Lerp(Vec{1, 1}, Vec{3, 3}, -1), VecNear({-1, -1}, 0.001));
  EXPECT_THAT(Lerp(Vec{1, 1}, Vec{3, 3}, 0), VecEq({1, 1}));
  EXPECT_THAT(Lerp(Vec{1, 1}, Vec{3, 3}, 0.5), VecEq({2, 2}));
  EXPECT_THAT(Lerp(Vec{1, 1}, Vec{3, 3}, 1), VecEq({3, 3}));
  EXPECT_THAT(Lerp(Vec{1, 1}, Vec{3, 3}, 2), VecEq({5, 5}));
}

TEST(VecLerpTest, ZeroInputVectors) {
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 2}, -1), VecEq({0, -2}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 2}, 0), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 2}, 0.75), VecEq({0, 1.5}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 2}, 1), VecEq({0, 2}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 2}, 2), VecEq({0, 4}));

  EXPECT_THAT(Lerp(Vec{-3, 0}, Vec{0, 0}, -1), VecEq({-6, 0}));
  EXPECT_THAT(Lerp(Vec{-3, 0}, Vec{0, 0}, 0), VecEq({-3, 0}));
  EXPECT_THAT(Lerp(Vec{-3, 0}, Vec{0, 0}, 0.5), VecEq({-1.5, 0}));
  EXPECT_THAT(Lerp(Vec{-3, 0}, Vec{0, 0}, 1), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{-3, 0}, Vec{0, 0}, 2), VecEq({3, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 0}, -1), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 0}, 0), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 0}, 0.2), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 0}, 1), VecEq({0, 0}));
  EXPECT_THAT(Lerp(Vec{0, 0}, Vec{0, 0}, 2), VecEq({0, 0}));
}

}  // namespace
}  // namespace geometry_internal
}  // namespace ink
