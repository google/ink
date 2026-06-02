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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushFamilyNative_create(
    void* jni_env_pass_through, const int64_t* coat_native_pointers,
    int num_coats, int64_t input_model_pointer,
    const char* client_brush_family_id, const char* developer_comment,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void BrushFamilyNative_free(int64_t native_pointer);

// The caller must free the returned string.
const char* BrushFamilyNative_getClientBrushFamilyId(int64_t native_pointer);

// The caller must free the returned string.
const char* BrushFamilyNative_getDeveloperComment(int64_t native_pointer);

int64_t BrushFamilyNative_getTextureAnimationLoopDurationMillis(
    int64_t native_pointer);

int64_t BrushFamilyNative_getBrushCoatCount(int64_t native_pointer);

int64_t BrushFamilyNative_newCopyOfBrushCoat(int64_t native_pointer, int index);

int BrushFamilyNative_getInputModelType(int64_t native_pointer);

int64_t BrushFamilyNative_newCopyOfInputModel(int64_t native_pointer);

int BrushFamilyNative_calculateMinimumRequiredVersion(int64_t native_pointer);

bool BrushFamilyNative_hasFallbacks(int64_t native_pointer);

int64_t InputModelNative_createNoParametersModel(int type);

int64_t InputModelNative_createSlidingWindowModel(int64_t window_size_millis,
                                                  int upsampling_frequency_hz);

int64_t InputModelNative_createSlidingWindowModelWithDefaultParameters();

void InputModelNative_free(int64_t native_pointer);

int64_t InputModelNative_getSlidingWindowDurationMillis(int64_t native_pointer);

int InputModelNative_getSlidingUpsamplingFrequencyHz(int64_t native_pointer);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_NATIVE_H_
