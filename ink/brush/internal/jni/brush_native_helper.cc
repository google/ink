// Copyright 2025 Google LLC
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

#include "ink/brush/internal/jni/brush_native_helper.h"

#include <cstdint>
#include <limits>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/version.h"

namespace ink::native {

absl::StatusOr<Version> IntToVersion(int version) {
  switch (version) {
    case 0:
      return Version::k0Jetpack1_0_0();
    case 1:
      return Version::k1Jetpack1_1_0Alpha01();
    case std::numeric_limits<int32_t>::max():
      return Version::kDevelopment();
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid version: ", version));
  }
}

}  // namespace ink::native
