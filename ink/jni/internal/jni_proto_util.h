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

#ifndef INK_JNI_INTERNAL_JNI_PROTO_UTIL_H_
#define INK_JNI_INTERNAL_JNI_PROTO_UTIL_H_

#include <jni.h>

#include <cstdint>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "google/protobuf/message_lite.h"

namespace ink::jni {

// A read-only view of bytes from the JVM passed via either a direct
// java.nio.ByteBuffer or a jbyteArray. Handles cleanup when the object goes out
// of scope.
class JvmBytes {
 public:
  static JvmBytes FromDirectByteBuffer(JNIEnv* env, jobject direct_byte_buffer);
  static JvmBytes FromByteArray(JNIEnv* env, jbyteArray byte_array);
  static JvmBytes FromEither(JNIEnv* env, jobject direct_byte_buffer,
                             jbyteArray byte_array);
  ~JvmBytes();

  // Not copyable or movable, meant to handle access and cleanup in a single
  // scope.
  JvmBytes(const JvmBytes&) = delete;
  JvmBytes& operator=(const JvmBytes&) = delete;
  JvmBytes(JvmBytes&&) = delete;
  JvmBytes& operator=(JvmBytes&&) = delete;

  const int8_t* NativeBytes() const {
    return reinterpret_cast<int8_t*>(bytes_);
  }

 private:
  JvmBytes(JNIEnv* env, absl_nullable jobject direct_byte_buffer,
           absl_nullable jbyteArray byte_array);

  JNIEnv* env_;
  jobject direct_byte_buffer_;
  jbyteArray byte_array_;
  jbyte* bytes_;
};

// A helper class for allocating JVM byte arrays in the middle of KMP-compatible
// native code. Passes back a pointer to the array elements and handles copying
// back of the elements and cleanup when the object goes out of scope.
class JvmByteArrayNativeAlloc {
 public:
  explicit JvmByteArrayNativeAlloc(JNIEnv* env);
  ~JvmByteArrayNativeAlloc();

  int8_t* Allocate(int size);
  jbyteArray Release(int8_t* native_bytes);

 private:
  JNIEnv* env_;
  absl::flat_hash_map<int8_t*, jbyteArray> byte_arrays_;
};

int8_t* JvmByteArrayNativeAllocCallback(void* allocator_pass_through, int size);

// Attempts to parse a serialized proto from either a direct java.nio.ByteBuffer
// or a jbyteArray, one of which must be non-null. If the proto doesn't parse,
// returns a non-OK absl::Status. `offset` is the starting point of the data
// within the `serialized_proto` array, and `size` is how far beyond `offset`
// the data continues.
absl::Status ParseProtoFromEither(JNIEnv* env,
                                  jobject serialized_proto_direct_buffer,
                                  jbyteArray serialized_proto_array,
                                  jint offset, jint size,
                                  google::protobuf::MessageLite& dest);

// Attempts to parse a serialized proto. If the proto doesn't parse, returns
// a non-OK absl::Status.
absl::Status ParseProtoFromByteArray(JNIEnv* env, jbyteArray serialized_proto,
                                     google::protobuf::MessageLite& dest);

// Attempts to parse a serialized proto. If the proto doesn't parse, returns
// a non-OK absl::Status. `offset` is the starting point of the data within the
// `serialized_proto` array, and `size` is how far beyond `offset` the data
// continues.
absl::Status ParseProtoFromByteArray(JNIEnv* env, jbyteArray serialized_proto,
                                     jint offset, jint size,
                                     google::protobuf::MessageLite& dest);

// Attempts to parse a serialized proto from a direct java.nio.ByteBuffer. If
// the proto doesn't parse, returns a non-OK absl::Status. `offset` is the
// starting point of the data within the `serialized_proto` array, and `size`
// is how far beyond `offset` the data continues.
// Note: This has a different name than `ParseProto`, as jbyteArray is a type of
// jobject so the two definitions would clash rather than serve as overloads.
absl::Status ParseProtoFromBuffer(JNIEnv* env,
                                  jobject serialized_proto_direct_buffer,
                                  jint offset, jint size,
                                  google::protobuf::MessageLite& dest);

// Serializes a proto into a newly-allocated Java byte array.
[[nodiscard]] jbyteArray SerializeProto(JNIEnv* env,
                                        const google::protobuf::MessageLite& src);

}  // namespace ink::jni

#endif  // INK_JNI_INTERNAL_JNI_PROTO_UTIL_H_
