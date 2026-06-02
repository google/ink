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

#ifndef INK_STORAGE_INTERNAL_JNI_BYTE_ARRAY_NATIVE_HELPER_H_
#define INK_STORAGE_INTERNAL_JNI_BYTE_ARRAY_NATIVE_HELPER_H_

#include <cstdint>

#include "absl/base/nullability.h"
#include "google/protobuf/message_lite.h"

namespace ink::native {

// Allocates a byte array and serializes the given proto into it, returning a
// pointer to the first element.
//
// alloc_native_array_callback is responsible for synchronously allocating an
// array of the size needed to encode the proto. It must return a pointer to the
// first element that persists until ownership is handed off to JNI or Kotlin
// code. alloc_native_array_pass_through is the pass-through parameter for that
// callback (e.g. in Kotlin-Native this will point to
// StableRef<androidx.ink.storage.ByteArrayAlloc>, in JNI it will point
// to ink::jni::JvmByteArrayNativeAlloc).
int8_t* NewNativeByteArrayFromProto(
    void* absl_nonnull alloc_native_array_pass_through,
    const google::protobuf::MessageLite& src,
    int8_t* absl_nullable (*absl_nonnull alloc_native_array_callback)(
        void* absl_nonnull pass_through, int size));

}  // namespace ink::native

#endif  // INK_STORAGE_INTERNAL_JNI_BYTE_ARRAY_NATIVE_HELPER_H_
