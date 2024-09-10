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

#include "ink/types/fuzz_domains.h"

#include <string>
#include <utility>

#include "fuzztest/fuzztest.h"
#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"
#include "ink/types/uri.h"

namespace ink {

fuzztest::Domain<Duration32> ArbitraryDuration32() {
  return fuzztest::Map(&Duration32::Seconds, fuzztest::Arbitrary<float>());
}

fuzztest::Domain<Duration32> FiniteNonNegativeDuration32() {
  return fuzztest::Filter(
      &Duration32::IsFinite,
      fuzztest::Map(&Duration32::Seconds, fuzztest::NonNegative<float>()));
}

fuzztest::Domain<PhysicalDistance> ArbitraryPhysicalDistance() {
  return fuzztest::Map(&PhysicalDistance::Centimeters,
                       fuzztest::Arbitrary<float>());
}

fuzztest::Domain<PhysicalDistance> FinitePositivePhysicalDistance() {
  return fuzztest::Filter(&PhysicalDistance::IsFinite,
                          fuzztest::Map(&PhysicalDistance::Centimeters,
                                        fuzztest::Positive<float>()));
}

fuzztest::Domain<std::string> ValidUriString() {
  return fuzztest::InRegexp(
      "^(ink:|INK:)?(//[a-z-]+)?/"
      "(brush-family|texture):[a-z-]+(:[1-9]{1,9})?");
}

fuzztest::Domain<Uri> ArbitraryUri() {
  return fuzztest::Map(
      [](absl::string_view uri_string) {
        absl::StatusOr<Uri> uri = Uri::Parse(uri_string);
        CHECK_OK(uri);
        return *std::move(uri);
      },
      ValidUriString());
}

}  // namespace ink
