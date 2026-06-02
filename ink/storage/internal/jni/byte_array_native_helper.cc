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

#include "ink/storage/internal/jni/byte_array_native_helper.h"

#include <cstdint>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/message_lite.h"

namespace ink::native {

int8_t* NewNativeByteArrayFromProto(
    void* absl_nonnull alloc_native_array_pass_through,
    const google::protobuf::MessageLite& src,
    int8_t* absl_nullable (*absl_nonnull alloc_native_array_callback)(
        void* absl_nonnull pass_through, int size)) {
  int size = src.ByteSizeLong();
  int8_t* native_array =
      alloc_native_array_callback(alloc_native_array_pass_through, size);
  ABSL_CHECK(src.SerializeToArray(native_array, size));
  return native_array;
}

}  // namespace ink::native
