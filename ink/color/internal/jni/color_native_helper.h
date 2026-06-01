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

#ifndef INK_COLOR_INTERNAL_JNI_COLOR_NATIVE_HELPER_H_
#define INK_COLOR_INTERNAL_JNI_COLOR_NATIVE_HELPER_H_

#include <cstdint>

#include "ink/color/color.h"

namespace ink::native {

int64_t ComputeColorLong(
    void* jni_env_pass_through, const Color& color,
    int64_t (*compute_compose_color_long_from_components_callback)(
        void*, int, float, float, float, float));

}  // namespace ink::native

#endif  // INK_COLOR_INTERNAL_JNI_COLOR_NATIVE_HELPER_H_
