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

#include <jni.h>

#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/storage/internal/jni/stroke_input_batch_serialization_native.h"

using ::ink::jni::JvmByteArrayNativeAlloc;
using ::ink::jni::JvmByteArrayNativeAllocCallback;
using ::ink::jni::JvmBytes;
using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

// Constructs a `StrokeInputBatch` from a serialized `CodedStrokeInputBatch`,
// which can be passed in as either a direct `ByteBuffer` or as an array of
// bytes. This returns the address of a heap-allocated `StrokeInputBatch`, which
// must later be freed by the caller.
JNI_METHOD(storage, StrokeInputBatchSerializationNative, jlong, createFromProto)
(JNIEnv* env, jobject object, jbyteArray byte_array, jint length) {
  JvmBytes jvm_bytes = JvmBytes::FromByteArray(env, byte_array);
  jlong native_pointer = StrokeInputBatchSerializationNative_createFromProto(
      env, jvm_bytes.NativeBytes(), length, &ThrowExceptionFromStatusCallback);
  return native_pointer;
}

JNI_METHOD(storage, StrokeInputBatchSerializationNative, jbyteArray, encode)
(JNIEnv* env, jobject object, jlong native_pointer) {
  JvmByteArrayNativeAlloc byte_array_allocator(env);
  return byte_array_allocator.Release(
      StrokeInputBatchSerializationNative_encode(
          &byte_array_allocator, native_pointer,
          &JvmByteArrayNativeAllocCallback));
}

}  // extern "C"
