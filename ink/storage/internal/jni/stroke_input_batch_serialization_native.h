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

#ifndef INK_STORAGE_INTERNAL_JNI_STROKE_INPUT_BATCH_SERIALIZATION_NATIVE_H_
#define INK_STORAGE_INTERNAL_JNI_STROKE_INPUT_BATCH_SERIALIZATION_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// byte_array contains a serialized ink.proto.CodedStrokeInputBatch. If
// the array is empty, byte_array will be nullptr.
int64_t StrokeInputBatchSerializationNative_createFromProto(
    void* jni_env_pass_through, const int8_t* byte_array, int size,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

// native_ptr is a raw pointer to an ink::StrokeInputBatch.
//
// alloc_native_array_callback is responsible for synchronously allocating an
// array of the size needed to encode the proto and returning a pointer to the
// first element in a context that is later read and cleaned up by the caller.
// alloc_native_array_pass_through is the pass-through parameter for that
// callback (e.g. in Kotlin-Native this will point to
// StableRef<androidx.ink.storage.ByteArrayAlloc>, in JNI it will point
// to ink::jni::JvmByteArrayNativeAlloc).
int8_t* StrokeInputBatchSerializationNative_encode(
    void* alloc_native_array_pass_through, int64_t native_pointer,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size));

#ifdef __cplusplus
}
#endif

#endif  // INK_STORAGE_INTERNAL_JNI_STROKE_INPUT_BATCH_SERIALIZATION_NATIVE_H_
