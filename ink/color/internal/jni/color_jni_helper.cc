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

#include "ink/color/internal/jni/color_jni_helper.h"

#include "absl/log/check.h"
#include "ink/color/color_space.h"

namespace ink {

ColorSpace JIntToColorSpace(jint color_space_id) {
  switch (color_space_id) {
    case kJniColorSpaceIdSrgb:
      return ColorSpace::kSrgb;
    case kJniColorSpaceIdDisplayP3:
      return ColorSpace::kDisplayP3;
    default:
      CHECK(false) << "Unknown color space id: " << color_space_id;
      break;
  }
}

}  // namespace ink
