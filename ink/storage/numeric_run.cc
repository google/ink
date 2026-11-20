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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/storage/proto/coded_numeric_run.pb.h"
#include "ink/types/iterator_range.h"

namespace ink {
namespace {

bool IsInt32(float number) {
  // `static_cast<float>(INT32_MAX)` rounds up to 2^31 (2147483648.0f), which is
  // outside the range of `int32_t`, so the upper bound must be strict (`<`).
  // `INT32_MIN` (-2^31) is exact in IEEE-754 float.
  return number >= static_cast<float>(std::numeric_limits<int32_t>::min()) &&
         number < static_cast<float>(std::numeric_limits<int32_t>::max()) &&
         number == static_cast<float>(static_cast<int32_t>(number));
}

}  // namespace

absl::StatusOr<iterator_range<CodedNumericRunIterator<float>>>
DecodeFloatNumericRun(const proto::CodedNumericRun& run) {
  if (!std::isfinite(run.offset())) {
    return absl::InvalidArgumentError(
        "invalid float numeric run: non-finite offset");
  }
  if (!std::isfinite(run.scale())) {
    return absl::InvalidArgumentError(
        "invalid float numeric run: non-finite scale");
  }
  return iterator_range<CodedNumericRunIterator<float>>{
      CodedNumericRunIterator<float>(&run, 0),
      CodedNumericRunIterator<float>(&run, run.deltas_size())};
}

absl::StatusOr<iterator_range<CodedNumericRunIterator<int32_t>>>
DecodeIntNumericRun(const proto::CodedNumericRun& run) {
  if (!IsInt32(run.offset())) {
    return absl::InvalidArgumentError(
        "invalid int numeric run: non-integer offset");
  }
  if (!IsInt32(run.scale())) {
    return absl::InvalidArgumentError(
        "invalid int numeric run: non-integer scale");
  }
  const int64_t scale = static_cast<int64_t>(run.scale());
  if (scale != 0) {
    const int64_t offset = static_cast<int64_t>(run.offset());
    // Since `offset` is in `[INT32_MIN, INT32_MAX]`, the two numerators below
    // are `<= 0` and `>= 0` respectively. Signed integer division truncates
    // toward zero, so dividing by `scale` yields the exact closed integer
    // interval of `cumulative_delta` values whose decoded `offset + scale *
    // cumulative_delta` lies in `[INT32_MIN, INT32_MAX]`.
    const int64_t first_cumulative_delta_bound =
        (std::numeric_limits<int32_t>::min() - offset) / scale;
    const int64_t second_cumulative_delta_bound =
        (std::numeric_limits<int32_t>::max() - offset) / scale;
    const int64_t min_cumulative_delta =
        std::min(first_cumulative_delta_bound, second_cumulative_delta_bound);
    const int64_t max_cumulative_delta =
        std::max(first_cumulative_delta_bound, second_cumulative_delta_bound);
    int64_t cumulative_delta = 0;
    for (int32_t delta : run.deltas()) {
      cumulative_delta += delta;
      if (cumulative_delta < min_cumulative_delta ||
          cumulative_delta > max_cumulative_delta) {
        return absl::InvalidArgumentError(
            "invalid int numeric run: decoded value out of int32_t range");
      }
    }
  }
  return iterator_range<CodedNumericRunIterator<int32_t>>{
      CodedNumericRunIterator<int32_t>(&run, 0),
      CodedNumericRunIterator<int32_t>(&run, run.deltas_size())};
}

}  // namespace ink
