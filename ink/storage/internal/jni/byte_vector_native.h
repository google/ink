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

#ifndef INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_H_
#define INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ByteVectorNative_getSize(int64_t byte_vector_native_pointer);

void ByteVectorNative_copyTo(int64_t byte_vector_native_pointer,
                             int8_t* byte_array);

void ByteVectorNative_free(int64_t byte_vector_native_pointer);

#ifdef __cplusplus
}
#endif

#endif  // INK_STORAGE_INTERNAL_JNI_BYTE_VECTOR_NATIVE_H_
