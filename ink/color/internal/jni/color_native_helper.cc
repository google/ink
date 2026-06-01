// Copyright 2026 Google LLC
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

#include "ink/color/internal/jni/color_native_helper.h"

#include <cstdint>

#include "ink/color/color.h"
#include "ink/color/color_space.h"

namespace ink::native {

namespace {

bool ColorSpaceIsSupportedInJetpack(ColorSpace color_space) {
  // Currently this is defensive coding, all of the color spaces supported in
  // Ink C++ code are also supported in the Color class used in Jetpack.
  switch (color_space) {
    case ColorSpace::kSrgb:
    case ColorSpace::kDisplayP3:
      return true;
  }
  return false;
}

}  // namespace

int64_t ComputeColorLong(
    void* jni_env_pass_through, const Color& color,
    int64_t (*compute_compose_color_long_from_components_callback)(
        void*, int, float, float, float, float)) {
  const ColorSpace& original_color_space = color.GetColorSpace();
  // This is currently defensive coding, at this time there aren't color
  // spaces supported here that aren't supported in Ink Jetpack.
  bool color_space_is_supported =
      ColorSpaceIsSupportedInJetpack(original_color_space);
  const Color& supported_color =
      color_space_is_supported ? color
                               : color.InColorSpace(ColorSpace::kDisplayP3);
  const Color::RgbaFloat rgba =
      supported_color.AsFloat(Color::Format::kGammaEncoded);
  return compute_compose_color_long_from_components_callback(
      jni_env_pass_through, static_cast<int>(supported_color.GetColorSpace()),
      rgba.r, rgba.g, rgba.b, rgba.a);
}

}  // namespace ink::native
