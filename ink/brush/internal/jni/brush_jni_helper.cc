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

#include "ink/brush/internal/jni/brush_jni_helper.h"

#include <jni.h>

#include <cstdint>
#include <limits>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "ink/brush/version.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/color/internal/jni/color_jni_helper.h"
#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

jlong ComputeColorLong(JNIEnv* env, const Color& color) {
  const ColorSpace& original_color_space = color.GetColorSpace();
  // This is currently defensive coding, at this time there aren't color
  // spaces supported here that aren't supported in Ink Jetpack.
  bool color_space_is_supported =
      ColorSpaceIsSupportedInJetpack(original_color_space);
  const Color::RgbaFloat rgba =
      (color_space_is_supported ? color
                                : color.InColorSpace(ColorSpace::kDisplayP3))
          .AsFloat(Color::Format::kGammaEncoded);
  return env->CallStaticLongMethod(
      ClassColorNative(env),
      MethodColorNativeComposeColorLongFromComponents(env),
      ColorSpaceToJInt(color_space_is_supported ? original_color_space
                                                : ColorSpace::kDisplayP3),
      rgba.r, rgba.g, rgba.b, rgba.a);
}

absl::StatusOr<Version> JIntToVersion(jint version) {
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

}  // namespace ink::jni
