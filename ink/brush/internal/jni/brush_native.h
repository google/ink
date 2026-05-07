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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushNative_create(
    void* jni_env_pass_through, int64_t family_native_pointer, float color_red,
    float color_green, float color_blue, float color_alpha, int color_space_id,
    float size, float epsilon,
    void (*throw_from_status_callback)(void*, int, const char*));

void BrushNative_free(int64_t native_pointer);

int64_t BrushNative_computeComposeColorLong(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t (*compute_compose_color_long_from_components_callback)(
        void*, int, float, float, float, float));

float BrushNative_getSize(int64_t native_pointer);

float BrushNative_getEpsilon(int64_t native_pointer);

int64_t BrushNative_newCopyOfBrushFamily(int64_t native_pointer);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_NATIVE_H_
