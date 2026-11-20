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

#include "ink/storage/numeric_run.h"

#include <cstdint>
#include <limits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/storage/proto/coded_numeric_run.pb.h"
#include "ink/types/iterator_range.h"

namespace ink {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::HasSubstr;

constexpr float kNonFiniteFloats[] = {
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity(),
    std::numeric_limits<float>::quiet_NaN(),
};
constexpr float kNonInt32Floats[] = {
    2.5f,
    -0.5f,
    // static_cast<float>(INT32_MAX) rounds up to 2^31 (2147483648.0f), which is
    // out of range for int32_t.
    static_cast<float>(std::numeric_limits<int32_t>::max()),
    1e15f,   // integral, but too big for int32_t
    -1e15f,  // integral, but too small for int32_t
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity(),
    std::numeric_limits<float>::quiet_NaN(),
};

TEST(NumericRunTest, DecodeEmptyFloatNumericRun) {
  proto::CodedNumericRun coded;
  EXPECT_THAT(DecodeFloatNumericRun(coded), IsOkAndHolds(ElementsAre()));
}

TEST(NumericRunTest, DecodeEmptyIntNumericRun) {
  proto::CodedNumericRun coded;
  EXPECT_THAT(DecodeIntNumericRun(coded), IsOkAndHolds(ElementsAre()));
}

TEST(NumericRunTest, DecodeValidFloatNumericRun) {
  proto::CodedNumericRun coded;
  coded.set_scale(0.5f);
  coded.set_offset(2.5f);
  coded.add_deltas(1);
  coded.add_deltas(-2);
  coded.add_deltas(3);
  coded.add_deltas(4);
  coded.add_deltas(5);

  EXPECT_THAT(DecodeFloatNumericRun(coded),
              IsOkAndHolds(ElementsAre(3.0f, 2.0f, 3.5f, 5.5f, 8.0f)));
}

TEST(NumericRunTest, DecodeValidIntNumericRun) {
  proto::CodedNumericRun coded;
  coded.add_deltas(1);
  coded.add_deltas(-2);
  coded.add_deltas(3);
  coded.add_deltas(4);
  coded.add_deltas(5);

  EXPECT_THAT(DecodeIntNumericRun(coded),
              IsOkAndHolds(ElementsAre(1, -1, 2, 6, 11)));
}

TEST(NumericRunTest, DecodeFloatNumericRunWithNonFiniteOffset) {
  for (float invalid : kNonFiniteFloats) {
    proto::CodedNumericRun coded;
    coded.set_offset(invalid);
    EXPECT_THAT(DecodeFloatNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("non-finite offset")));
  }
}

TEST(NumericRunTest, DecodeFloatNumericRunWithNonFiniteScale) {
  for (float invalid : kNonFiniteFloats) {
    proto::CodedNumericRun coded;
    coded.set_scale(invalid);
    EXPECT_THAT(DecodeFloatNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("non-finite scale")));
  }
}

TEST(NumericRunTest, DecodeFloatNumericRunOverflowDoesNotTriggerUb) {
  proto::CodedNumericRun coded;
  coded.set_offset(std::numeric_limits<float>::max());
  coded.set_scale(std::numeric_limits<float>::max());
  coded.add_deltas(10);
  EXPECT_THAT(
      DecodeFloatNumericRun(coded),
      IsOkAndHolds(ElementsAre(std::numeric_limits<float>::infinity())));
}

void DecodeFloatNumericRunDoesNotCrashOnArbitraryInput(
    const proto::CodedNumericRun& coded) {
  absl::StatusOr<iterator_range<CodedNumericRunIterator<float>>> range =
      DecodeFloatNumericRun(coded);
  if (!range.ok()) return;
  for (float value : *range) {
    (void)value;
  }
}
FUZZ_TEST(NumericRunTest, DecodeFloatNumericRunDoesNotCrashOnArbitraryInput);

TEST(NumericRunTest, DecodeIntNumericRunWithNonIntegerOffset) {
  for (float invalid : kNonInt32Floats) {
    proto::CodedNumericRun coded;
    coded.set_offset(invalid);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("non-integer offset")));
  }
}

TEST(NumericRunTest, DecodeIntNumericRunWithNonIntegerScale) {
  for (float invalid : kNonInt32Floats) {
    proto::CodedNumericRun coded;
    coded.set_scale(invalid);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("non-integer scale")));
  }
}

TEST(NumericRunTest, DecodeIntNumericRunAtInt32Boundaries) {
  constexpr int32_t kInt32Min = std::numeric_limits<int32_t>::min();
  constexpr int32_t kInt32Max = std::numeric_limits<int32_t>::max();
  // Largest float strictly less than 2^31 (2^31 - 128).
  constexpr float kMaxInt32Float = 2147483520.0f;

  {
    proto::CodedNumericRun coded;
    coded.set_offset(static_cast<float>(kInt32Min));
    coded.set_scale(1.0f);
    coded.add_deltas(0);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(1);
    EXPECT_THAT(
        DecodeIntNumericRun(coded),
        IsOkAndHolds(ElementsAre(kInt32Min, -1, kInt32Max - 1, kInt32Max)));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_offset(kMaxInt32Float);
    coded.set_scale(0.0f);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                IsOkAndHolds(ElementsAre(2147483520, 2147483520, 2147483520)));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_offset(0.0f);
    coded.set_scale(-1.0f);
    coded.add_deltas(-kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(1);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                IsOkAndHolds(ElementsAre(kInt32Max, 0, -kInt32Max, kInt32Min)));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_offset(-1.0f);
    coded.set_scale(2.0f);
    coded.add_deltas(-1073741823);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                IsOkAndHolds(ElementsAre(-kInt32Max)));
  }
}

TEST(NumericRunTest, DecodeIntNumericRunOutOfRangeReturnsError) {
  constexpr int32_t kInt32Min = std::numeric_limits<int32_t>::min();
  constexpr int32_t kInt32Max = std::numeric_limits<int32_t>::max();
  constexpr float kMaxInt32Float = 2147483520.0f;

  {
    // Overflow by 1 above INT32_MAX, even if a later delta returns in range.
    proto::CodedNumericRun coded;
    coded.add_deltas(kInt32Max);
    coded.add_deltas(1);
    coded.add_deltas(-1);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    // Overflow by 1 below INT32_MIN.
    proto::CodedNumericRun coded;
    coded.add_deltas(kInt32Min);
    coded.add_deltas(-1);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    // Negative scale with delta = INT32_MIN yields +2^31 (INT32_MAX + 1).
    proto::CodedNumericRun coded;
    coded.set_scale(-1.0f);
    coded.add_deltas(kInt32Min);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    // Non-unit scale overflowing by 1 below INT32_MIN (-1 + 2 * -1073741824).
    proto::CodedNumericRun coded;
    coded.set_offset(-1.0f);
    coded.set_scale(2.0f);
    coded.add_deltas(-1073741824);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    // Large scale and cumulative deltas that previously overflowed int64_t.
    proto::CodedNumericRun coded;
    coded.set_scale(kMaxInt32Float);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_scale(static_cast<float>(kInt32Min));
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    coded.add_deltas(kInt32Max);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_scale(static_cast<float>(kInt32Min));
    coded.add_deltas(kInt32Min);
    coded.add_deltas(kInt32Min);
    coded.add_deltas(kInt32Min);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }

  {
    proto::CodedNumericRun coded;
    coded.set_scale(kMaxInt32Float);
    coded.add_deltas(kInt32Min);
    coded.add_deltas(kInt32Min);
    coded.add_deltas(kInt32Min);
    EXPECT_THAT(DecodeIntNumericRun(coded),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("out of int32_t range")));
  }
}

void DecodeIntNumericRunDoesNotCrashOnArbitraryInput(
    const proto::CodedNumericRun& coded) {
  absl::StatusOr<iterator_range<CodedNumericRunIterator<int32_t>>> range =
      DecodeIntNumericRun(coded);
  if (!range.ok()) return;
  for (int32_t value : *range) {
    (void)value;
  }
}
FUZZ_TEST(NumericRunTest, DecodeIntNumericRunDoesNotCrashOnArbitraryInput);

TEST(NumericRunTest, IteratorPostIncrement) {
  proto::CodedNumericRun coded;
  coded.add_deltas(1);
  coded.add_deltas(2);
  coded.add_deltas(3);

  absl::StatusOr<iterator_range<CodedNumericRunIterator<int32_t>>> range =
      DecodeIntNumericRun(coded);
  ASSERT_THAT(range, IsOk());
  CodedNumericRunIterator<int32_t> iter = range->begin();
  const CodedNumericRunIterator<int32_t> iter0 = iter++;
  const CodedNumericRunIterator<int32_t> iter1 = iter++;
  const CodedNumericRunIterator<int32_t> iter2 = iter++;

  EXPECT_EQ(*iter0, 1);
  EXPECT_EQ(*iter1, 3);
  EXPECT_EQ(*iter2, 6);
  EXPECT_EQ(iter, range->end());
}

TEST(NumericRunTest, EncodeEmptyIntNumericRun) {
  std::vector<int32_t> values = {};
  proto::CodedNumericRun coded;
  EncodeIntNumericRun(values.begin(), values.end(), &coded);
  EXPECT_THAT(DecodeIntNumericRun(coded), IsOkAndHolds(ElementsAre()));
}

TEST(NumericRunTest, EncodeSignedIntNumericRun) {
  std::vector<int32_t> values = {1, 5, -8, 23, -3};
  proto::CodedNumericRun coded;
  EncodeIntNumericRun(values.begin(), values.end(), &coded);
  EXPECT_THAT(DecodeIntNumericRun(coded),
              IsOkAndHolds(ElementsAre(1, 5, -8, 23, -3)));
}

TEST(NumericRunTest, EncodeUnsignedIntNumericRun) {
  std::vector<uint32_t> values = {8, 23, 0, 19, 3};
  proto::CodedNumericRun coded;
  EncodeIntNumericRun(values.begin(), values.end(), &coded);
  EXPECT_THAT(DecodeIntNumericRun(coded),
              IsOkAndHolds(ElementsAre(8, 23, 0, 19, 3)));
}

// Test that arbitrary-ish integer sequences can round-trip through a
// CodedNumericRun proto.  The encoding doesn't support integers too near the
// limit of the int32 range (because the deltas are stored as int32s), but
// numbers of up to, say, +/- a billion should work fine.
void EncodeSignedIntNumericRunRoundTrip(const std::vector<int32_t>& values) {
  proto::CodedNumericRun coded;
  EncodeIntNumericRun(values.begin(), values.end(), &coded);
  EXPECT_THAT(DecodeIntNumericRun(coded),
              IsOkAndHolds(ElementsAreArray(values)));
}
FUZZ_TEST(NumericRunTest, EncodeSignedIntNumericRunRoundTrip)
    .WithDomains(fuzztest::VectorOf(fuzztest::InRange<int32_t>(-1'000'000'000,
                                                               1'000'000'000)));

void EncodeUnsignedIntNumericRunRoundTrip(const std::vector<uint32_t>& values) {
  proto::CodedNumericRun coded;
  EncodeIntNumericRun(values.begin(), values.end(), &coded);
  EXPECT_THAT(DecodeIntNumericRun(coded),
              IsOkAndHolds(ElementsAreArray(values)));
}
FUZZ_TEST(NumericRunTest, EncodeUnsignedIntNumericRunRoundTrip)
    .WithDomains(
        fuzztest::VectorOf(fuzztest::InRange<uint32_t>(0, 1'000'000'000)));

}  // namespace
}  // namespace ink
