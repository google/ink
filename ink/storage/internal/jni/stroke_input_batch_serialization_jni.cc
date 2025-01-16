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

#include <utility>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/storage/proto/coded.pb.h"
#include "ink/storage/stroke_input_batch.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace {

using ::ink::StrokeInputBatch;
using ::ink::jni::ParseProto;
using ::ink::jni::ParseProtoFromBuffer;
using ::ink::jni::SerializeProto;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::proto::CodedStrokeInputBatch;

}  // namespace

extern "C" {

// Constructs a `StrokeInputBatch` from a serialized `CodedStrokeInputBatch`,
// which can be passed in as either a direct `ByteBuffer` or as an array of
// bytes. This returns the address of a heap-allocated `StrokeInputBatch`, which
// must later be freed by the caller.
JNI_METHOD(storage, StrokeInputBatchSerializationJni, jlong,
           nativeNewStrokeInputBatchFromProto)
(JNIEnv* env, jclass klass, jobject input_direct_byte_buffer,
 jbyteArray input_bytes, jint input_offset, jint input_length,
 jboolean throw_on_parse_error) {
  ink::proto::CodedStrokeInputBatch coded_input;
  if (input_direct_byte_buffer != nullptr) {
    ABSL_CHECK_OK(ParseProtoFromBuffer(env, input_direct_byte_buffer,
                                       input_offset, input_length,
                                       coded_input));
  } else {
    ABSL_CHECK(input_bytes);
    ABSL_CHECK_OK(
        ParseProto(env, input_bytes, input_offset, input_length, coded_input));
  }
  absl::StatusOr<StrokeInputBatch> input =
      ink::DecodeStrokeInputBatch(coded_input);
  if (!input.ok()) {
    if (throw_on_parse_error) ThrowExceptionFromStatus(env, input.status());
    return 0;
  }
  return reinterpret_cast<jlong>(new StrokeInputBatch(*std::move(input)));
}

JNI_METHOD(storage, StrokeInputBatchSerializationJni, jbyteArray,
           nativeSerializeStrokeInputBatch)
(JNIEnv* env, jclass klass, jlong stroke_input_batch_native_pointer) {
  const auto* input = reinterpret_cast<const StrokeInputBatch*>(
      stroke_input_batch_native_pointer);
  CodedStrokeInputBatch coded_input;
  ink::EncodeStrokeInputBatch(*input, coded_input);
  return SerializeProto(env, coded_input);
}

}  // extern "C"
