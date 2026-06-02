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

#include "ink/jni/internal/jni_proto_util.h"

#include <jni.h>

#include <cstddef>
#include <cstdint>
#include <limits>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/message_lite.h"

namespace ink::jni {

JvmBytes JvmBytes::FromDirectByteBuffer(JNIEnv* env,
                                        jobject direct_byte_buffer) {
  return JvmBytes(env, direct_byte_buffer, nullptr);
}

JvmBytes JvmBytes::FromByteArray(JNIEnv* env, jbyteArray byte_array) {
  return JvmBytes(env, nullptr, byte_array);
}

JvmBytes JvmBytes::FromEither(JNIEnv* env, jobject direct_byte_buffer,
                              jbyteArray byte_array) {
  return JvmBytes(env, direct_byte_buffer, byte_array);
}

JvmBytes::~JvmBytes() {
  if (byte_array_ != nullptr) {
    env_->ReleaseByteArrayElements(byte_array_, bytes_, JNI_ABORT);
  }
}

JvmBytes::JvmBytes(JNIEnv* env, absl_nullable jobject direct_byte_buffer,
                   absl_nullable jbyteArray byte_array)
    : env_(env),
      direct_byte_buffer_(direct_byte_buffer),
      byte_array_(byte_array) {
  if (direct_byte_buffer_ == nullptr) {
    ABSL_CHECK_NE(byte_array_, nullptr)
        << "Must provide either a direct byte buffer or a byte array.";
    // TODO(b/290213178): See how often this copies using the isCopy pointer
    //   argument, and consider using critical arrays to minimize copying.
    bytes_ = env_->GetByteArrayElements(byte_array_, nullptr);
  } else {
    ABSL_CHECK_EQ(byte_array_, nullptr)
        << "Cannot provide both a direct byte buffer and a byte array.";
    bytes_ = reinterpret_cast<jbyte*>(
        env_->GetDirectBufferAddress(direct_byte_buffer_));
  }
}

int8_t* JvmByteArrayNativeAlloc::Allocate(int size) {
  jbyteArray byte_array = env_->NewByteArray(size);
  ABSL_CHECK(byte_array != nullptr);
  int8_t* native_bytes = env_->GetByteArrayElements(byte_array, nullptr);
  ABSL_CHECK(native_bytes != nullptr);
  byte_arrays_.emplace(native_bytes, byte_array);
  return native_bytes;
}

jbyteArray JvmByteArrayNativeAlloc::Release(int8_t* native_bytes) {
  auto it = byte_arrays_.find(native_bytes);
  ABSL_CHECK(it != byte_arrays_.end())
      << "Native bytes not allocated by this allocator.";
  jbyteArray byte_array = it->second;
  byte_arrays_.erase(it);
  env_->ReleaseByteArrayElements(byte_array, native_bytes, 0);
  return byte_array;
}

JvmByteArrayNativeAlloc::JvmByteArrayNativeAlloc(JNIEnv* env) : env_(env) {}

JvmByteArrayNativeAlloc::~JvmByteArrayNativeAlloc() {
  ABSL_CHECK_EQ(byte_arrays_.size(), 0)
      << "JvmByteArrayNativeAlloc destroyed with un-released byte arrays.";
}

int8_t* JvmByteArrayNativeAllocCallback(void* allocator_pass_through,
                                        int size) {
  return reinterpret_cast<JvmByteArrayNativeAlloc*>(allocator_pass_through)
      ->Allocate(size);
}

namespace {

absl::Status ParseProtoFromBytes(const JvmBytes& jvm_bytes, jint offset,
                                 jint size, google::protobuf::MessageLite& dest) {
  bool success = dest.ParseFromArray(jvm_bytes.NativeBytes() + offset, size);
  if (!success) {
    return absl::InvalidArgumentError(
        absl::StrCat("Failed to parse ", dest.GetTypeName(), " proto."));
  }
  return absl::OkStatus();
}

}  // namespace

absl::Status ParseProtoFromByteArray(JNIEnv* env, jbyteArray serialized_proto,
                                     google::protobuf::MessageLite& dest) {
  return ParseProtoFromByteArray(env, serialized_proto, 0,
                                 env->GetArrayLength(serialized_proto), dest);
}

absl::Status ParseProtoFromByteArray(JNIEnv* env, jbyteArray serialized_proto,
                                     jint offset, jint size,
                                     google::protobuf::MessageLite& dest) {
  return ParseProtoFromBytes(JvmBytes::FromByteArray(env, serialized_proto),
                             offset, size, dest);
}

absl::Status ParseProtoFromBuffer(JNIEnv* env,
                                  jobject serialized_proto_direct_buffer,
                                  jint offset, jint size,
                                  google::protobuf::MessageLite& dest) {
  return ParseProtoFromBytes(
      JvmBytes::FromDirectByteBuffer(env, serialized_proto_direct_buffer),
      offset, size, dest);
}

absl::Status ParseProtoFromEither(JNIEnv* env,
                                  jobject serialized_proto_direct_buffer,
                                  jbyteArray serialized_proto_array,
                                  jint offset, jint size,
                                  google::protobuf::MessageLite& dest) {
  return ParseProtoFromBytes(
      JvmBytes::FromEither(env, serialized_proto_direct_buffer,
                           serialized_proto_array),
      offset, size, dest);
}

jbyteArray SerializeProto(JNIEnv* env, const google::protobuf::MessageLite& src) {
  size_t size = src.ByteSizeLong();
  ABSL_CHECK_LE(size, std::numeric_limits<jsize>::max());
  jbyteArray serialized_proto = env->NewByteArray(size);
  ABSL_CHECK(serialized_proto != nullptr);
  jbyte* bytes = env->GetByteArrayElements(serialized_proto, nullptr);
  ABSL_CHECK(bytes != nullptr);
  bool success = src.SerializeToArray(bytes, size);
  // Here the array elements must be copied back to the JVM.
  env->ReleaseByteArrayElements(serialized_proto, bytes, 0);
  ABSL_CHECK(success);
  return serialized_proto;
}

}  // namespace ink::jni
