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

#include <jni.h>

#include <cstdint>
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/storage/internal/jni/byte_vector_native.h"
#include "ink/storage/internal/jni/byte_vector_native_helper.h"

using ::ink::native::CastToByteVector;

extern "C" {

JNI_METHOD(storage, ByteVectorNative, jbyteArray, getData)
(JNIEnv* env, jobject object, jlong native_pointer) {
  // This could use GetByteArrayElements along with ByteVectorNative_copyTo,
  // but using SetByteArrayRegion instead avoids a copy.
  const std::vector<int8_t>& byte_vector = CastToByteVector(native_pointer);
  jbyteArray byte_array = env->NewByteArray(byte_vector.size());
  ABSL_CHECK(byte_array != nullptr);
  env->SetByteArrayRegion(byte_array, 0, byte_vector.size(),
                          reinterpret_cast<const jbyte*>(byte_vector.data()));
  return byte_array;
}

JNI_METHOD(storage, ByteVectorNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  ByteVectorNative_free(native_pointer);
}

}  // extern "C"
