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

#include "ink/storage/internal/jni/stroke_input_batch_serialization_native.h"

#include <cstdint>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/storage/internal/jni/byte_array_native_helper.h"
#include "ink/storage/proto/stroke_input_batch.pb.h"
#include "ink/storage/stroke_input_batch.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native_helper.h"

using ::ink::DecodeStrokeInputBatch;
using ::ink::StrokeInputBatch;
using ::ink::native::CastToStrokeInputBatch;
using ::ink::native::NewNativeByteArrayFromProto;
using ::ink::native::NewNativeStrokeInputBatch;
using ::ink::proto::CodedStrokeInputBatch;

extern "C" {

int64_t StrokeInputBatchSerializationNative_createFromProto(
    void* jni_env_pass_through, const int8_t* byte_array, int size,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  CodedStrokeInputBatch coded_input;
  if (byte_array != nullptr) {
    bool success = coded_input.ParseFromArray(byte_array, size);
    if (!success) {
      throw_from_status_callback(
          jni_env_pass_through,
          static_cast<int>(absl::StatusCode::kInvalidArgument),
          "Failed to parse ink.proto.CodedStrokeInputBatch.");
      return 0;
    }
  }
  absl::StatusOr<StrokeInputBatch> input = DecodeStrokeInputBatch(coded_input);
  if (!input.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(input.status().code()),
                               input.status().ToString().c_str());
    return 0;
  }
  return NewNativeStrokeInputBatch(*std::move(input));
}

int8_t* StrokeInputBatchSerializationNative_encode(
    void* alloc_native_array_pass_through, int64_t native_pointer,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size)) {
  CodedStrokeInputBatch coded_input;
  EncodeStrokeInputBatch(CastToStrokeInputBatch(native_pointer), coded_input);
  return NewNativeByteArrayFromProto(alloc_native_array_pass_through,
                                     coded_input, alloc_native_array_callback);
}

}  // extern "C"
