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

#ifndef INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_NATIVE_H_
#define INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushFamilySerializationNative_createFromProto(
    void* jni_env_pass_through, void* on_decode_texture_pass_through,
    const int8_t* byte_array, int size, int max_version,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str),
    const char* (*on_decode_texture_callback)(void* pass_through,
                                              const char* encoded_id,
                                              const int8_t* bitmap,
                                              int bitmap_size));

int8_t* BrushFamilySerializationNative_encode(
    void* alloc_native_array_pass_through, int64_t native_pointer,
    const char** texture_ids, const int8_t** bitmaps, const int* bitmap_lengths,
    int num_textures,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size));

int8_t* BrushFamilySerializationNative_encodeMultiple(
    void* alloc_native_array_pass_through, int64_t* native_pointers,
    int num_brush_families, const char** texture_ids, const int8_t** bitmaps,
    const int* bitmap_lengths, int num_textures,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size));

int64_t MultipleBrushFamiliesNative_createFromProto(
    void* jni_env_pass_through, void* on_decode_texture_pass_through,
    const int8_t* byte_array, int size, int max_version,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str),
    const char* (*on_decode_texture_callback)(void* pass_through,
                                              const char* encoded_id,
                                              const int8_t* bitmap,
                                              int bitmap_size));

int MultipleBrushFamiliesNative_getBrushFamilyCount(int64_t native_pointer);

int64_t MultipleBrushFamiliesNative_releaseBrushFamily(int64_t native_pointer,
                                                       int index);

void MultipleBrushFamiliesNative_free(int64_t native_pointer);

#ifdef __cplusplus
}
#endif

#endif  // INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_NATIVE_H_
