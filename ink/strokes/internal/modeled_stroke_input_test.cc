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

#include "ink/strokes/internal/modeled_stroke_input.h"

#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/type_matchers.h"
#include "ink/strokes/internal/type_matchers.h"
#include "ink/types/duration.h"
#include "ink/types/type_matchers.h"

namespace ink::strokes_internal {
namespace {

constexpr float kFloatMax = std::numeric_limits<float>::max();

TEST(ModeledStrokeInputLerpTest, ZeroT) {
  ModeledStrokeInput a = {.position = {0, 0},
                          .velocity = {0, 0},
                          .traveled_distance = 73.f,
                          .elapsed_time = Duration32::Seconds(52),
                          .pressure = .3f,
                          .tilt = kQuarterTurn,
                          .orientation = kFullTurn / 16};
  ModeledStrokeInput b = {.position = {10, 10},
                          .velocity = {5, 5},
                          .traveled_distance = 79.f,
                          .elapsed_time = Duration32::Seconds(56),
                          .pressure = .7f,
                          .tilt = kFullTurn / 8,
                          .orientation = kFullTurn / 8};

  ModeledStrokeInput result = Lerp(a, b, 0);
  EXPECT_THAT(result, ModeledStrokeInputEq(a));
}

TEST(ModeledStrokeInputLerpTest, OneT) {
  ModeledStrokeInput a = {.position = {0, 0},
                          .velocity = {0, 0},
                          .traveled_distance = 73.f,
                          .elapsed_time = Duration32::Seconds(52),
                          .pressure = .3f,
                          .tilt = kQuarterTurn,
                          .orientation = kFullTurn / 16};
  ModeledStrokeInput b = {.position = {10, 10},
                          .velocity = {5, 5},
                          .traveled_distance = 79.f,
                          .elapsed_time = Duration32::Seconds(56),
                          .pressure = .7f,
                          .tilt = kFullTurn / 8,
                          .orientation = kFullTurn / 8};

  ModeledStrokeInput result = Lerp(a, b, 1);
  EXPECT_THAT(result, ModeledStrokeInputEq(b));
}

TEST(ModeledStrokeInputLerpTest, TBetweenZeroAndOne) {
  ModeledStrokeInput a = {.position = {0, 0},
                          .velocity = {0, 0},
                          .traveled_distance = 73.f,
                          .elapsed_time = Duration32::Seconds(52),
                          .pressure = .3f,
                          .tilt = kQuarterTurn,
                          .orientation = kFullTurn / 16};
  ModeledStrokeInput b = {.position = {10, 10},
                          .velocity = {5, 5},
                          .traveled_distance = 79.f,
                          .elapsed_time = Duration32::Seconds(56),
                          .pressure = .7f,
                          .tilt = kFullTurn / 8,
                          .orientation = kFullTurn / 8};

  ModeledStrokeInput result = Lerp(a, b, 0.2);
  EXPECT_THAT(result.position, PointEq({2, 2}));
  EXPECT_THAT(result.velocity, VecEq({1, 1}));
  EXPECT_FLOAT_EQ(result.traveled_distance, 74.2);
  EXPECT_THAT(result.elapsed_time, Duration32Eq(Duration32::Seconds(52.8)));
  EXPECT_FLOAT_EQ(result.pressure, 0.38);
  EXPECT_THAT(result.tilt, AngleEq(0.45 * kHalfTurn));
  EXPECT_THAT(result.orientation, AngleEq(0.15 * kHalfTurn));
}

TEST(ModeledStrokeInputLerpTest, AboveOneT) {
  ModeledStrokeInput a = {.position = {0, 0},
                          .velocity = {0, 0},
                          .traveled_distance = 73.f,
                          .elapsed_time = Duration32::Seconds(52),
                          .pressure = .3f,
                          .tilt = kQuarterTurn,
                          .orientation = kFullTurn / 16};
  ModeledStrokeInput b = {.position = {10, 10},
                          .velocity = {5, 5},
                          .traveled_distance = 79.f,
                          .elapsed_time = Duration32::Seconds(56),
                          .pressure = .7f,
                          .tilt = kFullTurn / 8,
                          .orientation = kFullTurn / 8};

  ModeledStrokeInput result = Lerp(a, b, 1.1);
  EXPECT_THAT(result.position, PointEq({11, 11}));
  EXPECT_THAT(result.velocity, VecEq({5.5, 5.5}));
  EXPECT_FLOAT_EQ(result.traveled_distance, 79.6);
  EXPECT_THAT(result.elapsed_time, Duration32Eq(Duration32::Seconds(56.4)));
  EXPECT_FLOAT_EQ(result.pressure, 0.74);
  EXPECT_THAT(result.tilt, AngleEq(0.225 * kHalfTurn));
  EXPECT_THAT(result.orientation, AngleEq(0.2625 * kHalfTurn));
}

TEST(ModeledStrokeInputLerpTest, BelowZeroT) {
  ModeledStrokeInput a = {.position = {0, 0},
                          .velocity = {0, 0},
                          .traveled_distance = 73.f,
                          .elapsed_time = Duration32::Seconds(52),
                          .pressure = .3f,
                          .tilt = kQuarterTurn,
                          .orientation = kFullTurn / 16};
  ModeledStrokeInput b = {.position = {10, 10},
                          .velocity = {5, 5},
                          .traveled_distance = 79.f,
                          .elapsed_time = Duration32::Seconds(56),
                          .pressure = .7f,
                          .tilt = kFullTurn / 8,
                          .orientation = kFullTurn / 8};

  ModeledStrokeInput result = Lerp(a, b, -0.1);
  EXPECT_THAT(result.position, PointEq({-1, -1}));
  EXPECT_THAT(result.velocity, VecEq({-.5, -.5}));
  EXPECT_FLOAT_EQ(result.traveled_distance, 72.4);
  EXPECT_THAT(result.elapsed_time, Duration32Eq(Duration32::Seconds(51.6)));
  EXPECT_FLOAT_EQ(result.pressure, 0.26);
  EXPECT_THAT(result.tilt, AngleEq(0.525 * kHalfTurn));
  EXPECT_THAT(result.orientation, AngleEq(0.1125 * kHalfTurn));
}

}  // namespace
}  // namespace ink::strokes_internal
