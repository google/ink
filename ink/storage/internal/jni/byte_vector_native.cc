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

#include "ink/storage/internal/jni/byte_vector_native.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include "ink/storage/internal/jni/byte_vector_native_helper.h"

using ::ink::native::CastToByteVector;
using ::ink::native::DeleteByteVector;

extern "C" {

int ByteVectorNative_getSize(int64_t byte_vector_native_pointer) {
  return CastToByteVector(byte_vector_native_pointer).size();
}

void ByteVectorNative_copyTo(int64_t byte_vector_native_pointer,
                             int8_t* byte_array) {
  const std::vector<int8_t>& byte_vector =
      CastToByteVector(byte_vector_native_pointer);
  memcpy(byte_array, byte_vector.data(), byte_vector.size());
}

void ByteVectorNative_free(int64_t byte_vector_native_pointer) {
  DeleteByteVector(byte_vector_native_pointer);
}

}  // extern "C"
