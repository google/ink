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

#include "ink/storage/internal/jni/partitioned_mesh_serialization_native.h"

#include <cstdint>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native_helper.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/storage/internal/jni/byte_array_native_helper.h"
#include "ink/storage/partitioned_mesh.h"
#include "ink/storage/proto/mesh.pb.h"

using ::ink::DecodePartitionedMesh;
using ::ink::EncodePartitionedMesh;
using ::ink::PartitionedMesh;
using ::ink::native::CastToPartitionedMesh;
using ::ink::native::NewNativeByteArrayFromProto;
using ::ink::native::NewNativePartitionedMesh;
using ::ink::proto::CodedModeledShape;

extern "C" {

int64_t PartitionedMeshSerializationNative_createFromProto(
    void* jni_env_pass_through, const int8_t* byte_array, int size,
    bool fill_missing_attributes,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  CodedModeledShape coded_shape;
  if (byte_array != nullptr) {
    bool success = coded_shape.ParseFromArray(byte_array, size);
    if (!success) {
      throw_from_status_callback(
          jni_env_pass_through,
          static_cast<int>(absl::StatusCode::kInvalidArgument),
          "Failed to parse ink.proto.CodedModeledShape.");
      return 0;
    }
  }
  absl::StatusOr<PartitionedMesh> shape =
      DecodePartitionedMesh(coded_shape, fill_missing_attributes);
  if (!shape.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(shape.status().code()),
                               shape.status().ToString().c_str());
    return 0;
  }
  return NewNativePartitionedMesh(*std::move(shape));
}

int8_t* PartitionedMeshSerializationNative_encode(
    void* alloc_native_array_pass_through, int64_t native_pointer,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size)) {
  CodedModeledShape coded_shape;
  EncodePartitionedMesh(CastToPartitionedMesh(native_pointer), coded_shape);
  return NewNativeByteArrayFromProto(alloc_native_array_pass_through,
                                     coded_shape, alloc_native_array_callback);
}

}  // extern "C"
