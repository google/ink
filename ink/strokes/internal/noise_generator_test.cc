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

#include "ink/strokes/internal/noise_generator.h"

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"

using ::testing::AllOf;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Pointwise;

namespace ink::strokes_internal {
namespace {

TEST(NoiseGeneratorTest, RandomSequenceIsFixedForAGivenSeed) {
  // Given the same parameters, the output of a NoiseGenerator should never
  // change, even across Ink library releases.
  NoiseGenerator generator(12345);
  std::vector<float> actual;
  for (int i = 0; i < 30; ++i) {
    actual.push_back(generator.CurrentOutputValue());
    generator.AdvanceInputBy(0.1);
  }
  std::vector<float> expected = {
      0.933754683, 0.915264189, 0.865075827, 0.791113913, 0.701303065,
      0.603567779, 0.505832434, 0.416021585, 0.342059731, 0.291871309,
      0.273380876, 0.276017755, 0.283174962, 0.293722451, 0.306530118,
      0.320467860, 0.334405601, 0.347213268, 0.357760727, 0.364917934,
      0.367554814, 0.363935769, 0.354112744, 0.339636654, 0.322058588,
      0.302929521, 0.283800423, 0.266222358, 0.251746297, 0.241923273};
  EXPECT_THAT(actual, Pointwise(FloatEq(), expected));
}

// Tests that all values emitted from the NoiseGenerator are in [0, 1].
void EmitsValuesBetweenZeroAndOne(uint64_t seed, float advance_by) {
  NoiseGenerator generator(seed);
  for (int i = 0; i < 1000; ++i) {
    generator.AdvanceInputBy(advance_by);
    EXPECT_THAT(generator.CurrentOutputValue(), AllOf(Ge(0), Le(1)));
  }
}
FUZZ_TEST(NoiseGeneratorTest, EmitsValuesBetweenZeroAndOne)
    .WithDomains(fuzztest::Arbitrary<uint64_t>(),
                 fuzztest::InRange<float>(0, 3));

// Tests that small advances through the sequence result in small changes to the
// output value.
void EmitsContinuousValues(uint64_t seed) {
  NoiseGenerator generator(seed);
  float prev_value = generator.CurrentOutputValue();
  for (int i = 0; i < 1000; ++i) {
    generator.AdvanceInputBy(0.01);
    float value = generator.CurrentOutputValue();
    EXPECT_THAT(value, FloatNear(prev_value, 0.05));
    prev_value = value;
  }
}
FUZZ_TEST(NoiseGeneratorTest, EmitsContinuousValues)
    .WithDomains(fuzztest::Arbitrary<uint64_t>());

// Tests that you can copy a NoiseGenerator (even mid-way through its sequence),
// and from that point on the two generators will emit identical sequences.
void CopiedGeneratorEmitsSameSequence(uint64_t seed, float advance_by) {
  NoiseGenerator generator1(seed);
  for (int i = 0; i < 100; ++i) {
    generator1.AdvanceInputBy(advance_by);
  }
  NoiseGenerator generator2 = generator1;
  for (int i = 0; i < 1000; ++i) {
    generator1.AdvanceInputBy(advance_by);
    generator2.AdvanceInputBy(advance_by);
    EXPECT_THAT(generator2.CurrentOutputValue(),
                FloatEq(generator1.CurrentOutputValue()));
  }
}
FUZZ_TEST(NoiseGeneratorTest, CopiedGeneratorEmitsSameSequence)
    .WithDomains(fuzztest::Arbitrary<uint64_t>(),
                 fuzztest::InRange<float>(0, 3));

}  // namespace
}  // namespace ink::strokes_internal
