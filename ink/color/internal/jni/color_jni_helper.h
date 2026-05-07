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

#ifndef INK_COLOR_INTERNAL_JNI_COLOR_JNI_HELPER_H_
#define INK_COLOR_INTERNAL_JNI_COLOR_JNI_HELPER_H_

#include <jni.h>

#include <cstdint>

namespace ink::jni {

// Converts an Ink `Color` into a Kotlin `ColorLong`, suitable for use from
// C-compatible Kotlin native interface.
int64_t ComposeColorLongFromComponentsCallback(
    void* jni_env, int color_space_id, float red_gamma_corrected,
    float green_gamma_corrected, float blue_gamma_corrected, float alpha);

}  // namespace ink::jni

#endif  // INK_COLOR_INTERNAL_JNI_COLOR_JNI_HELPER_H_
